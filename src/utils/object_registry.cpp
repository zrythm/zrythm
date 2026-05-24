// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "utils/object_registry.h"

#include <boost/unordered/unordered_flat_map.hpp>

namespace zrythm::utils
{

struct ObjectRegistry::Impl
{
  boost::unordered::unordered_flat_map<QUuid, utils::UuidIdentifiableBase *>
                                                   objects_by_id_;
  boost::unordered::unordered_flat_map<QUuid, int> ref_counts_;
};

ObjectRegistry::ObjectRegistry (QObject * parent)
    : QObject (parent), impl_ (std::make_unique<Impl> ())
{
}

ObjectRegistry::~ObjectRegistry ()
{
  destroying_ = true;
}

void
ObjectRegistry::register_object_impl (utils::UuidIdentifiableBase &obj)
{
  const auto id = obj.raw_uuid ();
  if (impl_->objects_by_id_.contains (id))
    throw std::runtime_error (
      fmt::format ("duplicate UUID: {}", id.toString ()));
  obj.setParent (this);
  impl_->objects_by_id_.emplace (id, &obj);
}

void
ObjectRegistry::acquire_reference_impl (const QUuid &id)
{
  impl_->ref_counts_[id]++;
}

void
ObjectRegistry::release_reference_impl (const QUuid &id)
{
  if (destroying_)
    return;
  auto it = impl_->ref_counts_.find (id);
  if (it == impl_->ref_counts_.end () || it->second <= 0)
    {
      throw std::runtime_error (
        fmt::format (
          "release_reference: invalid ref count for UUID {}", id.toString ()));
    }
  if (--it->second <= 0)
    delete_object_by_id (id);
}

utils::UuidIdentifiableBase *
ObjectRegistry::find_by_raw_uuid_impl (const QUuid &id) const
{
  const auto it = impl_->objects_by_id_.find (id);
  return it != impl_->objects_by_id_.end () ? it->second : nullptr;
}

bool
ObjectRegistry::contains_impl (const QUuid &id) const
{
  return impl_->objects_by_id_.contains (id);
}

void
ObjectRegistry::for_each_matching_impl (
  const QMetaObject &meta_type,
  ObjectVisitor      visitor) const
{
  for (const auto &[id, obj] : impl_->objects_by_id_)
    {
      if (obj->metaObject ()->inherits (&meta_type))
        visitor (*obj);
    }
}

void
ObjectRegistry::delete_object_by_id (const QUuid &id)
{
  const auto it = impl_->objects_by_id_.find (id);
  if (it == impl_->objects_by_id_.end ())
    return;
  auto * obj = it->second;
  impl_->objects_by_id_.erase (it);
  impl_->ref_counts_.erase (id);
  delete obj;
}

int
ObjectRegistry::ref_count (const QUuid &id) const
{
  const auto it = impl_->ref_counts_.find (id);
  return it != impl_->ref_counts_.end () ? it->second : 0;
}

utils::UuidIdentifiableBase *
ObjectRegistry::find_by_raw_uuid_or_throw (const QUuid &id) const
{
  auto * obj = find_by_raw_uuid (id);
  if (obj == nullptr)
    {
      throw std::runtime_error (
        fmt::format ("Object with id {} not found", id.toString ()));
    }
  return obj;
}

} // namespace zrythm::utils
