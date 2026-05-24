// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/icloneable.h"
#include "utils/uuid_identifiable.h"

#include <QUuid>

#include <boost/describe/class.hpp>
#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>
#include <type_safe/strong_typedef.hpp>

namespace zrythm::utils
{
/**
 * @brief CRTP base that adds a typed UUID strong-typedef to a class hierarchy.
 *
 * Each distinct "family" of objects (e.g., Track, ArrangerObject, Plugin)
 * should have ONE class that inherits from UuidIdentifiableObject<ThatClass>.
 * Derived types (e.g., MidiTrack from Track) inherit the UUID through the
 * base — they must NOT inherit from UuidIdentifiableObject again, since each
 * object has exactly one UUID identity.
 *
 * The `uuid_base_type` alias exposes the CRTP parameter so the UuidIdentifiable
 * concept can verify the inheritance chain structurally (no duck-typing).
 *
 * Inherits UuidIdentifiableBase (which is a QObject), so derived types do NOT
 * need to inherit QObject separately.
 *
 * Example:
 *   class Track : public UuidIdentifiableObject<Track> { ... };
 *   class MidiTrack : public Track { ... };  // inherits Track's UUID
 *
 * @tparam Derived The class at the top of the hierarchy that "owns" the UUID
 * type.
 */
template <typename Derived>
class UuidIdentifiableObject : public UuidIdentifiableBase
{
public:
  /** Exposes the CRTP parameter for concept checking. */
  using uuid_base_type = Derived;

  struct Uuid final
      : type_safe::strong_typedef<Uuid, QUuid>,
        type_safe::strong_typedef_op::equality_comparison<Uuid>,
        type_safe::strong_typedef_op::relational_comparison<Uuid>
  {
    using UuidTag = void; // used by the fmt formatter below
    using type_safe::strong_typedef<Uuid, QUuid>::strong_typedef;

    explicit Uuid () = default;

    static_assert (utils::StrongTypedef<Uuid>);
    // static_assert (type_safe::is_strong_typedef<Uuid>::value);
    // static_assert (std::is_same_v<type_safe::underlying_type<Uuid>, QUuid>);

    /**
     * @brief Checks if the UUID is null.
     */
    bool is_null () const { return type_safe::get (*this).isNull (); }

    void set_null () { type_safe::get (*this) = QUuid (); }

    // Used by boost hash
    friend std::size_t hash_value (const Uuid &p) { return p.hash (); }

    std::size_t hash () const { return qHash (type_safe::get (*this)); }
  };
  static_assert (std::regular<Uuid>);
  static_assert (sizeof (Uuid) == sizeof (QUuid));

  explicit UuidIdentifiableObject (QObject * parent = nullptr)
      : UuidIdentifiableBase (QUuid::createUuid (), parent)
  {
  }
  UuidIdentifiableObject (const Uuid &id, QObject * parent = nullptr)
      : UuidIdentifiableBase (type_safe::get (id), parent)
  {
  }

  Q_DISABLE_COPY_MOVE (UuidIdentifiableObject)
  ~UuidIdentifiableObject () override = default;

  auto get_uuid () const { return Uuid (raw_uuid ()); }

  friend void init_from (
    UuidIdentifiableObject       &obj,
    const UuidIdentifiableObject &other,
    utils::ObjectCloneType        clone_type)
  {
    if (clone_type == utils::ObjectCloneType::NewIdentity)
      {
        obj.set_raw_uuid (QUuid::createUuid ());
      }
    else
      {
        obj.set_raw_uuid (other.raw_uuid ());
      }
  }

  friend void to_json (nlohmann::json &j, const UuidIdentifiableObject &obj)
  {
    to_json (j, static_cast<const UuidIdentifiableBase &> (obj));
  }
  friend void from_json (const nlohmann::json &j, UuidIdentifiableObject &obj)
  {
    from_json (j, static_cast<UuidIdentifiableBase &> (obj));
  }

  friend bool operator== (
    const UuidIdentifiableObject &lhs,
    const UuidIdentifiableObject &rhs)
  {
    return static_cast<const UuidIdentifiableBase &> (lhs)
           == static_cast<const UuidIdentifiableBase &> (rhs);
  }

  BOOST_DESCRIBE_CLASS (UuidIdentifiableObject, (UuidIdentifiableBase), (), (), ())
};

/**
 * @brief Concept: T has a UUID identity from a UuidIdentifiableObject hierarchy.
 *
 * Works for both base types (e.g., Track : UuidIdentifiableObject<Track>) and
 * derived types (e.g., MidiTrack : Track) because uuid_base_type is inherited
 * and resolves to the correct CRTP parameter in both cases.
 *
 * QObject is now implicit via UuidIdentifiableBase, so
 * TypedUuidReference<T>::get() can use qobject_cast without a separate check.
 *
 * Uses structural checks instead of std::derived_from so that the concept
 * works with incomplete types (e.g., TypedUuidReference<Track> as a member
 * of Track itself).
 */
template <typename T>
concept UuidIdentifiable = requires {
  typename T::uuid_base_type;
  typename T::Uuid;
};

// specialization for std::hash and boost::hash
#define DEFINE_UUID_HASH_SPECIALIZATION(UuidType) \
  namespace std \
  { \
  template <> struct hash<UuidType> \
  { \
    size_t operator() (const UuidType &uuid) const { return uuid.hash (); } \
  }; \
  }

} // namespace zrythm::utils

// Concept to detect UuidIdentifiableObject::Uuid types
template <typename T>
concept UuidType = requires (T t) {
  typename T::UuidTag;
  { type_safe::get (t) } -> std::convertible_to<QUuid>;
};

// Formatter for any UUID type from UuidIdentifiableObject
template <UuidType T>
struct fmt::formatter<T> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const T &uuid, FormatContext &ctx) const
  {
    return fmt::formatter<std::string_view>{}.format (
      type_safe::get (uuid).toString (QUuid::WithoutBraces).toUtf8 (), ctx);
  }
};
