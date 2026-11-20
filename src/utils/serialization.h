// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <concepts>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>

#include "utils/enum_json.h"
#include "utils/exceptions.h"
#include "utils/qt.h"
#include "utils/traits.h"

#include <QUuid>

#include <au/au.hh>
#include <boost/unordered/concurrent_flat_map_fwd.hpp>
#include <nlohmann/json.hpp>

/* magic_enum's templates trip -Wshadow under the project's warning flags */
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wshadow"
#endif
#include <magic_enum/magic_enum.hpp>
#if defined(__GNUC__) || defined(__clang__)
#  pragma GCC diagnostic pop
#endif

using zrythm::utils::ZrythmException;
using namespace std::string_view_literals;

inline void
to_json (nlohmann::json &j, const QString &s)
{
  j = zrythm::utils::to_std_string (s);
}
inline void
from_json (const nlohmann::json &j, QString &s)
{
  s = QString::fromUtf8 (j.get<std::string> ());
}

inline void
to_json (nlohmann::json &j, const QUuid &uuid)
{
  j = uuid.toString (QUuid::WithoutBraces);
}
inline void
from_json (const nlohmann::json &j, QUuid &uuid)
{
  uuid = QUuid::fromString (j.get<QString> ());
}

namespace zrythm::utils::serialization
{

static constexpr auto kSchemaVersionKey = "schemaVersion"sv;
static constexpr auto kAppVersionKey = "appVersion"sv;
static constexpr auto kDocumentTypeKey = "documentType"sv;
static constexpr auto kVariantTypeKey = "variantType"sv;
static constexpr auto kVariantNonObjectValueKey = "nonObjectValue"sv;

/**
 * @brief Creates an object from JSON without deserializing its data.
 *
 * This is the first phase of two-phase deserialization. It creates the object
 * using the builder but does not deserialize any data. This allows all objects
 * to be registered before any inter-object references are resolved during data
 * deserialization.
 *
 * @tparam Variant The variant of pointers to create.
 * @tparam BuilderT The builder type.
 * @param j The JSON value containing the object data.
 * @param var The variant to store the created object.
 * @param builder The builder to use to create the object.
 */
template <StdVariant Variant, ObjectBuilder BuilderT>
inline void
variant_create_object_only (
  const nlohmann::json &j,
  Variant              &var,
  const BuilderT       &builder)
{
  std::int64_t index = 0;
  try
    {
      index =
        j.at (zrythm::utils::serialization::kVariantTypeKey).get<std::int64_t> ();
    }
  catch (const nlohmann::json::exception &e)
    {
      throw ZrythmException (
        "Failed to read variant type index: " + std::string (e.what ()));
    }

  if (
    index < 0
    || index >= static_cast<std::int64_t> (std::variant_size_v<Variant>))
    {
      throw ZrythmException (
        "Invalid variant type index " + std::to_string (index) + " (expected 0.."
        + std::to_string (std::variant_size_v<Variant> - 1) + ")");
    }

  const auto unsigned_index = static_cast<std::size_t> (index);

  auto creator = [&]<size_t... I> (std::index_sequence<I...>) {
    Variant result{};

    auto create_type_if_current_index = [&]<size_t N> () {
      using Type = std::variant_alternative_t<N, Variant>;
      using StrippedType = std::remove_pointer_t<Type>;
      if (N == unsigned_index)
        {
          auto object_ptr = builder.template build<StrippedType> ();

          if constexpr (std::is_pointer_v<Type>)
            result = object_ptr.release ();
          else if constexpr (std::is_copy_constructible_v<Type>)
            result = *object_ptr;
          else if constexpr (is_derived_from_template_v<QObjectUniquePtr, Type>)
            result = std::move (*object_ptr);
          else
            {
              DEBUG_TEMPLATE_PARAM (Type)
              static_assert (
                false,
                "Only variant of pointers or copy-constructible types is currently supported");
            }
        }
    };

    (create_type_if_current_index.template operator()<I> (), ...);

    return result;
  };

  var = creator (std::make_index_sequence<std::variant_size_v<Variant>>{});
}

/**
 * @brief Deserializes data into an existing variant object.
 *
 * This is the second phase of two-phase deserialization. It deserializes all
 * data (except the type field) into the object stored in the variant.
 *
 * @tparam Variant The variant containing the object to deserialize into.
 * @param j The JSON value containing the object data.
 * @param var The variant containing the object to deserialize into.
 */
template <StdVariant Variant>
inline void
variant_deserialize_data (const nlohmann::json &j, Variant &var)
{
  // Create a copy of j without the "variantType" field for value
  // deserialization
  nlohmann::json value_json = j;
  value_json.erase (zrythm::utils::serialization::kVariantTypeKey);

  std::visit (
    [&] (auto &&obj) {
      using ObjType = std::decay_t<decltype (obj)>;
      if constexpr (std::is_pointer_v<ObjType>)
        {
          if (value_json.contains (kVariantNonObjectValueKey))
            value_json[kVariantNonObjectValueKey].get_to (*obj);
          else
            value_json.get_to (*obj);
        }
      else if constexpr (is_derived_from_template_v<QObjectUniquePtr, ObjType>)
        {
          if (value_json.contains (kVariantNonObjectValueKey))
            value_json[kVariantNonObjectValueKey].get_to (*obj);
          else
            value_json.get_to (*obj);
        }
      else
        {
          if (value_json.contains (kVariantNonObjectValueKey))
            value_json[kVariantNonObjectValueKey].get_to (obj);
          else
            value_json.get_to (obj);
        }
    },
    var);
}

/**
 * @brief Deserializer for a variant of pointers buildable with a builder.
 *
 * This is a convenience function that combines object creation and
 * deserialization in a single step. For cases where two-phase deserialization
 * is needed (e.g., when objects reference each other), use
 * variant_create_object_only() and variant_deserialize_data() separately.
 *
 * @tparam Variant The variant of pointers to deserialize.
 * @tparam BuilderT The builder type.
 * @param j The JSON value to deserialize.
 * @param var The variant to deserialize into.
 * @param builder The builder to use to create the objects.
 */
template <StdVariant Variant, ObjectBuilder BuilderT>
inline void
variant_from_json_with_builder (
  const nlohmann::json &j,
  Variant              &var,
  const BuilderT       &builder)
{
  variant_create_object_only (j, var, builder);
  variant_deserialize_data (j, var);
}

/**
 * @brief True for enums marked as flag sets with ENUM_ENABLE_BITSET().
 *
 * Flag enums serialize as an array of the names of their set bits.
 */
template <typename T>
concept FlagEnum =
  EnumType<T>
  && std::same_as<
    bool,
    std::remove_cvref_t<decltype (magic_enum::customize::enum_range<T>::is_flags)>>;

/**
 * @brief True for enums that the central enum serializer handles.
 *
 * This is every enum except flag enums (ENUM_ENABLE_BITSET()), enums
 * marked with ENUM_CUSTOM_JSON() (see utils/enum_json.h), and nlohmann's
 * own enums.
 */
/**
 * @brief True for enums declared by the nlohmann library itself.
 *
 * nlohmann::json serializes its own enums through its default serializer.
 * The enum specializations below must not match them: nlohmann's own headers
 * instantiate adl_serializer for these enums while being included, and a
 * matching partial specialization declared afterwards is ill-formed.
 */
template <typename T>
concept NlohmannInternalEnum =
  std::same_as<T, nlohmann::detail::value_t>
  || std::same_as<T, nlohmann::detail::error_handler_t>
  || std::same_as<T, nlohmann::detail::input_format_t>
  || std::same_as<T, nlohmann::detail::cbor_tag_handler_t>;

template <typename T>
concept NameSerializableEnum =
  EnumType<T> && !FlagEnum<T> && !NlohmannInternalEnum<T>
  && !zrythm::utils::serialization::use_custom_enum_json<T>::value;

/**
 * @brief True for flag enums the central flag serializer handles.
 *
 * This is every enum marked with ENUM_ENABLE_BITSET() except enums marked
 * with ENUM_CUSTOM_JSON() (see utils/enum_json.h).
 */
template <typename T>
concept FlagSerializableEnum =
  FlagEnum<T> && !NlohmannInternalEnum<T>
  && !zrythm::utils::serialization::use_custom_enum_json<T>::value;

/**
 * @brief Error message for a JSON value that matches no enumerator of @p E,
 * listing the accepted names.
 */
template <EnumType E>
std::string
invalid_enum_value_message (const nlohmann::json &json_value)
{
  std::string allowed;
  for (const auto name : magic_enum::enum_names<E> ())
    {
      if (!allowed.empty ())
        {
          allowed += ", ";
        }
      allowed += std::string (name);
    }
  return "invalid " + std::string (magic_enum::enum_type_name<E> ()) + " value "
         + json_value.dump () + ": expected one of: " + allowed;
}

}; // namespace zrythm::utils::serialization

namespace nlohmann
{
/**
 * @brief Pointer specialization: serializes the pointee, or null for a null
 * pointer.
 *
 * Char pointers are excluded: they are C strings and serialize as strings
 * through the default serializer.
 */
template <typename T>
  requires (!std::same_as<std::remove_cv_t<T>, char>)
struct adl_serializer<T *>
{
  static void to_json (json &j, const T * ptr)
  {
    if (ptr)
      j = *ptr;
    else
      j = nullptr;
  }
  // from_json would be unsafe due to raw memory allocation, so handle each case
  // manually in each type
};

/// std::variant specialization (serialization only)
template <typename... Args> struct adl_serializer<std::variant<Args...>>
{
  static void to_json (json &j, std::variant<Args...> const &v)
  {
    std::visit (
      [&] (auto &&value) {
        j[zrythm::utils::serialization::kVariantTypeKey] = v.index ();
        // Merge the value's JSON fields directly into j
        json value_json = std::forward<decltype (value)> (value);
        if (value_json.is_object ())
          {
            j.update (value_json);
          }
        else
          {
            j[zrythm::utils::serialization::kVariantNonObjectValueKey] =
              value_json;
          }
      },
      v);
  }
};

/// std::unique_ptr specialization
template <typename T> struct adl_serializer<std::unique_ptr<T>>
{
  static void to_json (json &json_value, const std::unique_ptr<T> &ptr)
  {
    if (ptr.get ())
      {
        json_value = *ptr;
      }
    else
      {
        json_value = nullptr;
      }
  }
  static void from_json (const json &json_value, std::unique_ptr<T> &ptr)
    requires std::is_default_constructible_v<T>
  {
    ptr = std::make_unique<T> ();
    json_value.get_to (*ptr);
  }
};

/// std::shared_ptr specialization
template <typename T> struct adl_serializer<std::shared_ptr<T>>
{
  static void to_json (json &json_value, const std::shared_ptr<T> &ptr)
  {
    if (ptr.get ())
      {
        json_value = *ptr;
      }
    else
      {
        json_value = nullptr;
      }
  }
  static void from_json (const json &json_value, std::shared_ptr<T> &ptr)
    requires std::is_default_constructible_v<T>
  {
    ptr = std::make_shared<T> ();
    json_value.get_to (*ptr);
  }
};

/// QObjectUniquePtr specialization
template <typename T> struct adl_serializer<zrythm::utils::QObjectUniquePtr<T>>
{
  static void
  to_json (json &json_value, const zrythm::utils::QObjectUniquePtr<T> &ptr)
  {
    if (ptr.get ())
      {
        json_value = *ptr;
      }
    else
      {
        json_value = nullptr;
      }
  }
  static void
  from_json (const json &json_value, zrythm::utils::QObjectUniquePtr<T> &ptr)
    requires std::is_default_constructible_v<T>
  {
    ptr = new T ();
    json_value.get_to (*ptr);
  }
};

/// StrongTypedef specialization
template <zrythm::utils::StrongTypedef T> struct adl_serializer<T>
{
  static void to_json (json &json_value, const T &strong_type)
  {
    json_value = type_safe::get (strong_type);
  }
  static void from_json (const json &json_value, T &strong_type)
  {
    json_value.get_to (type_safe::get (strong_type));
  }
};

/**
 * @brief Enum specialization: enums serialize as their enumerator names.
 *
 * Values that match no enumerator name throw when written or read, with an
 * error naming the accepted names.
 *
 * Enums marked as flags with ENUM_ENABLE_BITSET() serialize through the
 * flag serializer below; enums marked with ENUM_CUSTOM_JSON() serialize
 * through their own to_json/from_json.
 */
template <zrythm::utils::serialization::NameSerializableEnum E>
struct adl_serializer<E>
{
  static void to_json (json &json_value, E value)
  {
    const auto name = magic_enum::enum_name (value);
    if (name.empty ())
      {
        throw json::out_of_range::create (
          401,
          std::string (magic_enum::enum_type_name<E> ()) + " value "
            + std::to_string (magic_enum::enum_integer (value))
            + " matches no enumerator name",
          nullptr);
      }
    json_value = std::string (name);
  }

  static void from_json (const json &json_value, E &value)
  {
    if (!json_value.is_string ())
      {
        throw json::type_error::create (
          302,
          zrythm::utils::serialization::invalid_enum_value_message<E> (
            json_value),
          nullptr);
      }
    const auto parsed =
      magic_enum::enum_cast<E> (json_value.get<std::string> ());
    if (!parsed.has_value ())
      {
        throw json::out_of_range::create (
          401,
          zrythm::utils::serialization::invalid_enum_value_message<E> (
            json_value),
          nullptr);
      }
    value = *parsed;
  }
};

/**
 * @brief Flag enum specialization: enums marked with ENUM_ENABLE_BITSET()
 * serialize as an array of the names of their set bits.
 *
 * Writing throws when a set bit matches no enumerator name. Reading accepts
 * an array of flag names and throws on any unknown name.
 */
template <zrythm::utils::serialization::FlagSerializableEnum E>
struct adl_serializer<E>
{
  using Unsigned = std::make_unsigned_t<std::underlying_type_t<E>>;

  static void to_json (json &json_value, E value)
  {
    auto remaining = static_cast<Unsigned> (magic_enum::enum_underlying (value));
    json_value = json::array ();
    for (const auto flag : magic_enum::enum_values<E> ())
      {
        const auto bits =
          static_cast<Unsigned> (magic_enum::enum_underlying (flag));
        if ((remaining & bits) != 0)
          {
            json_value.push_back (std::string (magic_enum::enum_name (flag)));
            remaining &= static_cast<Unsigned> (~bits);
          }
      }
    if (remaining != 0)
      {
        throw json::out_of_range::create (
          401,
          std::string (magic_enum::enum_type_name<E> ()) + " value "
            + std::to_string (magic_enum::enum_underlying (value))
            + " contains bits matching no enumerator name",
          nullptr);
      }
  }

  static void from_json (const json &json_value, E &value)
  {
    if (!json_value.is_array ())
      {
        throw json::type_error::create (
          302,
          zrythm::utils::serialization::invalid_enum_value_message<E> (
            json_value),
          nullptr);
      }
    auto bits = Unsigned{ 0 };
    for (const auto &item : json_value)
      {
        if (!item.is_string ())
          {
            throw json::type_error::create (
              302,
              zrythm::utils::serialization::invalid_enum_value_message<E> (item),
              nullptr);
          }
        const auto parsed = magic_enum::enum_cast<E> (item.get<std::string> ());
        if (!parsed.has_value ())
          {
            throw json::out_of_range::create (
              401,
              zrythm::utils::serialization::invalid_enum_value_message<E> (item),
              nullptr);
          }
        bits |= static_cast<Unsigned> (magic_enum::enum_underlying (*parsed));
      }
    value = static_cast<E> (bits);
  }
};

template <typename Key, typename T>
struct adl_serializer<boost::unordered::concurrent_flat_map<Key, T>>
{
  using ConcurrentHashMap = boost::unordered::concurrent_flat_map<Key, T>;
  // convert to std::map since nlohmann::json knows how to serialize it
  static void to_json (json &j, const ConcurrentHashMap &map)
  {
    std::map<Key, T> temp;
    map.visit_all ([&temp] (const auto &kv) {
      temp.emplace (kv.first, kv.second);
    });
    j = temp;
  }

  static void from_json (const json &j, ConcurrentHashMap &map)
  {
    auto temp = j.get<std::map<Key, T>> (); // Deserialize to std::map first
    map.clear ();
    for (const auto &kv : temp)
      {
        map.emplace (kv.first, kv.second);
      }
  }
};

/// au::Quantity specialization
template <typename Unit, typename Rep>
struct adl_serializer<au::Quantity<Unit, Rep>>
{
  static void to_json (json &j, const au::Quantity<Unit, Rep> &quantity)
  {
    j = quantity.in (Unit{});
  }

  static void from_json (const json &j, au::Quantity<Unit, Rep> &quantity)
  {
    Rep raw_value;
    j.get_to (raw_value);
    quantity = au::QuantityMaker<Unit>{}(raw_value);
  }
};

} // namespace nlohmann
