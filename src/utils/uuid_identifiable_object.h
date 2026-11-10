// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/icloneable.h"

#include <QObject>
#include <QUuid>
#include <QtQmlIntegration/qqmlintegration.h>

#include <boost/describe/class.hpp>
#include <fmt/format.h>
#include <nlohmann/json_fwd.hpp>
#include <type_safe/strong_typedef.hpp>

namespace zrythm::utils
{
/**
 * @brief QObject base for all UUID-identifiable objects.
 *
 * Every object referenced by UUID in the project inherits from this, either
 * directly (the family owner that holds the typed identity) or indirectly
 * through a typed base like Track. The QObject base lets the registry use
 * qobject_cast and QObject parent-child ownership, so derived types do not
 * need to inherit QObject separately.
 *
 * The class is deliberately non-template: it can carry Q_OBJECT and be
 * registered as an uncreatable QML type, which keeps the prototype chains
 * that qmllint and qmlls generate for derived types resolvable down to
 * QObject.
 *
 * Family owners add their typed UUID identity with DEFINE_UUID_IDENTITY(),
 * paired with a UUID type defined at namespace scope with DEFINE_UUID_TYPE():
 *
 *   DEFINE_UUID_TYPE (TrackUuid)
 *
 *   class Track : public utils::UuidIdentifiableObject
 *   {
 *   public:
 *     DEFINE_UUID_IDENTITY (TrackUuid)
 *   };
 *
 * Derived types (e.g., MidiTrack from Track) inherit the identity through
 * the base — they must not define their own, since each object has exactly
 * one UUID identity.
 */
class UuidIdentifiableObject : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
  Q_PROPERTY (QString uuidString READ uuidString CONSTANT)

public:
  /**
   * @brief Structural marker for the UuidIdentifiable concept.
   *
   * Inherited by every identifiable type, so the concept can verify
   * membership without duck-typing on unrelated nested types.
   */
  using uuid_base_type = void;

  explicit UuidIdentifiableObject (QObject * parent = nullptr)
      : QObject (parent), uuid_ (QUuid::createUuid ())
  {
  }
  explicit UuidIdentifiableObject (const QUuid &id, QObject * parent = nullptr)
      : QObject (parent), uuid_ (id)
  {
  }

  Q_DISABLE_COPY_MOVE (UuidIdentifiableObject)
  ~UuidIdentifiableObject () override = default;

  QUuid raw_uuid () const { return uuid_; }

  /**
   * @brief The UUID as a string without braces.
   */
  QString uuidString () const { return uuid_.toString (QUuid::WithoutBraces); }

  friend bool operator== (
    const UuidIdentifiableObject &lhs,
    const UuidIdentifiableObject &rhs)
  {
    return lhs.uuid_ == rhs.uuid_;
  }

  friend void to_json (nlohmann::json &j, const UuidIdentifiableObject &obj);
  friend void from_json (const nlohmann::json &j, UuidIdentifiableObject &obj);

  /** Assigns the source identity, or a fresh one for new-identity clones. */
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

protected:
  void set_raw_uuid (const QUuid &id) { uuid_ = id; }

private:
  QUuid uuid_;

  BOOST_DESCRIBE_CLASS (UuidIdentifiableObject, (), (), (), (uuid_))
};

/**
 * @brief Defines a distinct UUID strong type at namespace scope.
 *
 * Each identifiable family (Track, Port, ProcessorBase, ...) declares its
 * UUID type with this macro next to the family class. DEFINE_UUID_IDENTITY()
 * inside the class makes it available as Class::Uuid.
 */
#define DEFINE_UUID_TYPE(UuidType) \
  struct UuidType final \
      : type_safe::strong_typedef<UuidType, QUuid>, \
        type_safe::strong_typedef_op::equality_comparison<UuidType>, \
        type_safe::strong_typedef_op::relational_comparison<UuidType> \
  { \
    using UuidTag = void; /* used by the UUID fmt::formatter specializations */ \
    using type_safe::strong_typedef<UuidType, QUuid>::strong_typedef; \
    explicit UuidType () = default; \
    bool is_null () const { return type_safe::get (*this).isNull (); } \
    void set_null () { type_safe::get (*this) = QUuid (); } \
    friend std::size_t hash_value (const UuidType &p) { return p.hash (); } \
    std::size_t        hash () const { return qHash (type_safe::get (*this)); } \
  }; \
  static_assert (std::regular<UuidType>); \
  static_assert (sizeof (UuidType) == sizeof (QUuid));

/**
 * @brief Adds the family's typed UUID identity to an identifiable class.
 *
 * Defines the Class::Uuid alias and the typed get_uuid() accessor. The UUID
 * type must be defined with DEFINE_UUID_TYPE() at namespace scope.
 */
#define DEFINE_UUID_IDENTITY(UuidType) \
  using Uuid = UuidType; \
  UuidType get_uuid () const \
  { \
    return UuidType (raw_uuid ()); \
  }

/**
 * @brief Concept: T has a UUID identity from a UuidIdentifiableObject
 * hierarchy.
 *
 * Works for both family owners (e.g., Track) and derived types (e.g.,
 * MidiTrack) because uuid_base_type and the Uuid alias are inherited by both.
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

// Concept to detect UUID strong types defined with DEFINE_UUID_TYPE()
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
