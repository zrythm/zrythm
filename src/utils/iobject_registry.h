// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
#pragma once

#include <functional>

#include "utils/rt_thread_id.h"
#include "utils/uuid_identifiable.h"

#include <QMetaObject>
#include <QUuid>

inline std::size_t
hash_value (const QUuid &id)
{
  return qHash (id);
}

namespace zrythm::utils
{

/**
 * @brief Abstract interface for a UUID-keyed object registry.
 *
 * Decouples lower layers (arrangement, tracks) from the concrete registry
 * implementation (ProjectRegistry). Operates on the non-template
 * UuidIdentifiableBase, so it has no knowledge of variants or specific types.
 *
 * Reference counting: acquire_reference()/release_reference() manage
 * reference counts. When the count drops to zero, the registry may delete
 * the object. UuidReference and TypedUuidReference handle this automatically
 * via RAII.
 */
class IObjectRegistry
{
public:
  virtual ~IObjectRegistry () = default;

  void register_object (UuidIdentifiableBase &obj)
  {
    assert_main_thread ();
    register_object_impl (obj);
  }

  void acquire_reference (const QUuid &id)
  {
    assert_main_thread ();
    acquire_reference_impl (id);
  }

  void release_reference (const QUuid &id)
  {
    assert_main_thread ();
    release_reference_impl (id);
  }

  [[gnu::hot]] UuidIdentifiableBase * find_by_raw_uuid (const QUuid &id) const
  {
    assert_main_thread ();
    return find_by_raw_uuid_impl (id);
  }

  bool contains (const QUuid &id) const
  {
    assert_main_thread ();
    return contains_impl (id);
  }

  // ============================================================================
  // Type-filtered iteration (NVI pattern)
  //
  // Public templated API delegates to private virtual _impl methods.
  // ProjectRegistry optimizes via internal type buckets.
  // ObjectRegistry falls back to iterating all objects with a type check.
  // ============================================================================

  template <typename T>
  void for_each_matching (std::function<void (T &)> visitor) const
  {
    for_each_matching_impl (T::staticMetaObject, [&] (UuidIdentifiableBase &obj) {
      auto * casted = qobject_cast<T *> (&obj);
      assert (casted != nullptr);
      visitor (*casted);
    });
  }

  template <typename T> [[nodiscard]] size_t count_matching () const
  {
    size_t count = 0;
    for_each_matching_impl (T::staticMetaObject, [&] (UuidIdentifiableBase &obj) {
      assert (qobject_cast<T *> (&obj) != nullptr);
      ++count;
    });
    return count;
  }

protected:
  IObjectRegistry () : creation_thread_id_ (current_thread_id) { }

private:
  void assert_main_thread () const
  {
    assert (creation_thread_id_ == current_thread_id);
  }

protected:
  virtual void register_object_impl (UuidIdentifiableBase &obj) = 0;
  virtual void acquire_reference_impl (const QUuid &id) = 0;
  virtual void release_reference_impl (const QUuid &id) = 0;
  [[gnu::hot]] virtual UuidIdentifiableBase *
               find_by_raw_uuid_impl (const QUuid &id) const = 0;
  virtual bool contains_impl (const QUuid &id) const = 0;

  using ObjectVisitor = std::function<void (UuidIdentifiableBase &)>;
  virtual void
  for_each_matching_impl (const QMetaObject &meta_type, ObjectVisitor visitor)
    const = 0;

private:
  RTThreadId creation_thread_id_;
};

} // namespace zrythm::utils
