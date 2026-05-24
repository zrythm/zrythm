// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
#pragma once

#include "utils/iobject_registry.h"

#include <QUuid>

namespace zrythm::utils
{

/**
 * @brief Untyped, reference-counted UUID reference into an IObjectRegistry.
 *
 * Holds a raw QUuid and a pointer to IObjectRegistry. Acquires/releases
 * reference counts on construction/destruction so the registry can manage
 * object lifetimes. Returns UuidIdentifiableBase* from get() — callers
 * that need a specific type should use TypedUuidReference<T> instead.
 *
 * Can be in an unengaged state (no id) for deferred initialization during
 * deserialization.
 */
class UuidReference
{
public:
  UuidReference () = delete;

  UuidReference (const QUuid &id, IObjectRegistry &registry)
      : id_ (id), registry_ (&registry)
  {
    acquire_ref ();
  }

  UuidReference (IObjectRegistry &registry) : registry_ (&registry) { }

  UuidReference (const UuidReference &other)
      : id_ (other.id_), registry_ (other.registry_)
  {
    if (id_.has_value () && registry_ != nullptr)
      {
        acquire_ref ();
      }
  }

  UuidReference &operator= (const UuidReference &other)
  {
    if (this != &other)
      {
        release_ref ();
        id_ = other.id_;
        registry_ = other.registry_;
        if (id_.has_value () && registry_ != nullptr)
          {
            acquire_ref ();
          }
      }
    return *this;
  }

  UuidReference (UuidReference &&other) noexcept
      : id_ (std::exchange (other.id_, std::nullopt)),
        registry_ (std::exchange (other.registry_, nullptr))
  {
  }

  UuidReference &operator= (UuidReference &&other) noexcept
  {
    if (this != &other)
      {
        release_ref ();
        id_ = std::exchange (other.id_, std::nullopt);
        registry_ = std::exchange (other.registry_, nullptr);
      }
    return *this;
  }

  ~UuidReference () { release_ref (); }

  QUuid id () const { return id_.value (); }

  void set_id (const QUuid &id)
  {
    if (id_.has_value ())
      {
        throw std::runtime_error (
          "Cannot set id of UuidReference that already has an id");
      }
    id_ = id;
    acquire_ref ();
  }

  UuidIdentifiableBase * get () const
  {
    if (!id_.has_value () || registry_ == nullptr)
      return nullptr;
    return registry_->find_by_raw_uuid (id_.value ());
  }

  UuidIdentifiableBase * get_or_throw () const
  {
    if (!id_.has_value () || registry_ == nullptr)
      {
        throw std::runtime_error ("UuidReference: no id or registry");
      }
    auto * obj = registry_->find_by_raw_uuid (id_.value ());
    if (obj == nullptr)
      {
        throw std::runtime_error (
          "UuidReference: object not found for id "
          + id_->toString ().toStdString ());
      }
    return obj;
  }

  bool has_value () const { return id_.has_value (); }

  IObjectRegistry &get_registry () const
  {
    assert (registry_ != nullptr);
    return *registry_;
  }

  friend void to_json (nlohmann::json &j, const UuidReference &ref);
  friend void from_json (const nlohmann::json &j, UuidReference &ref);

  friend bool operator== (const UuidReference &lhs, const UuidReference &rhs)
  {
    return lhs.id_ == rhs.id_;
  }

protected:
  void acquire_ref ()
  {
    if (id_.has_value () && registry_ != nullptr)
      {
        registry_->acquire_reference (id_.value ());
      }
  }

  void release_ref ()
  {
    if (id_.has_value () && registry_ != nullptr)
      {
        registry_->release_reference (id_.value ());
      }
  }

  std::optional<QUuid> id_;
  IObjectRegistry *    registry_ = nullptr;
};

} // namespace zrythm::utils
