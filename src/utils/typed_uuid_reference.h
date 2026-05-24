// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
#pragma once

#include "utils/traits.h"
#include "utils/uuid_identifiable_object.h"
#include "utils/uuid_reference.h"

#include <type_safe/strong_typedef.hpp>

namespace zrythm::utils
{

/**
 * @brief Typed, reference-counted UUID reference into an IObjectRegistry.
 *
 * Wraps UuidReference and adds type safety. Holds the typed UUID
 * (typename T::Uuid) and resolves to T* via qobject_cast. Works for both
 * base types (e.g., TypedUuidReference<Track>) and concrete derived types
 * (e.g., TypedUuidReference<MidiTrack>) — the only difference is which
 * qobject_cast is performed on lookup.
 *
 * T must satisfy the UuidIdentifiable concept (i.e., inherit from a
 * UuidIdentifiableObject<U> for some U, and be a QObject).
 *
 * Use as_untyped() to access the underlying UuidReference when the typed
 * interface is not needed (e.g., for storage in heterogeneous containers).
 */
template <UuidIdentifiable T> class TypedUuidReference
{
public:
  using UuidType = typename T::Uuid;

  TypedUuidReference () = delete;

  TypedUuidReference (const UuidType &id, IObjectRegistry &registry)
      : ref_ (type_safe::get (id), registry)
  {
  }

  TypedUuidReference (IObjectRegistry &registry) : ref_ (registry) { }

  TypedUuidReference (const TypedUuidReference &other) = default;
  TypedUuidReference &operator= (const TypedUuidReference &other) = default;
  TypedUuidReference (TypedUuidReference &&other) noexcept = default;
  TypedUuidReference &operator= (TypedUuidReference &&other) noexcept = default;

  template <UuidIdentifiable U>
    requires std::derived_from<U, T>
  TypedUuidReference (const TypedUuidReference<U> &other)
      : ref_ (other.as_untyped ())
  {
  }

  UuidType id () const { return UuidType (ref_.id ()); }

  void set_id (const UuidType &id) { ref_.set_id (type_safe::get (id)); }

  T * get () const
  {
    auto * base = ref_.get ();
    return qobject_cast<T *> (base);
  }

  template <typename DerivedT>
    requires std::derived_from<DerivedT, T>
  DerivedT * get_object_as () const
  {
    return qobject_cast<DerivedT *> (get ());
  }

  UuidIdentifiableBase * get_object_base () const { return ref_.get (); }

  T * get_or_throw () const
  {
    auto * ptr = get ();
    if (ptr == nullptr)
      {
        throw std::runtime_error (
          fmt::format ("TypedUuidReference: object not found or wrong type"));
      }
    return ptr;
  }

  bool has_value () const { return ref_.has_value (); }

  IObjectRegistry &get_registry () const { return ref_.get_registry (); }

  const UuidReference &as_untyped () const { return ref_; }

  friend void to_json (nlohmann::json &j, const TypedUuidReference &ref)
  {
    to_json (j, ref.ref_);
  }
  friend void from_json (const nlohmann::json &j, TypedUuidReference &ref)
  {
    from_json (j, ref.ref_);
  }

  friend auto format_as (const TypedUuidReference &ref)
  {
    return type_safe::get (ref.id ());
  }

private:
  UuidReference ref_;
};

} // namespace zrythm::utils
