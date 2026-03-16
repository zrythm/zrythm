// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>
#include <string>

#include "utils/exceptions.h"
#include "utils/qt.h"
#include "utils/traits.h"
#include "utils/types.h"

#include <QUuid>

#include <au/au.hh>
#include <boost/unordered/concurrent_flat_map_fwd.hpp>
#include <nlohmann/json.hpp>

using zrythm::utils::exceptions::ZrythmException;

inline void
to_json (nlohmann::json &j, const QString &s)
{
  j = s.toStdString ();
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
static constexpr auto kVariantTypeKey = "type"sv;
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
  const auto index =
    j.at (zrythm::utils::serialization::kVariantTypeKey).get<int> ();

  auto creator = [&]<size_t... I> (std::index_sequence<I...>) {
    Variant result{};

    auto create_type_if_current_index = [&]<size_t N> () {
      using Type = std::variant_alternative_t<N, Variant>;
      using StrippedType = std::remove_pointer_t<Type>;
      if (N == index)
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
  // Create a copy of j without the "type" field for value deserialization
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

}; // namespace zrythm::utils::serialization

namespace nlohmann
{
/// pointer specialization
template <typename T> struct adl_serializer<T *>
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
template <StrongTypedef T> struct adl_serializer<T>
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
