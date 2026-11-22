// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdexcept>

#include "utils/format_qt.h"

#include "utils/composite_object_registry.h"

namespace zrythm::utils
{

CompositeObjectRegistry::CompositeObjectRegistry (
  IObjectRegistry &primary,
  IObjectRegistry &secondary)
    : primary_ (primary), secondary_ (secondary)
{
}

void
CompositeObjectRegistry::register_object_impl (UuidIdentifiableObject &obj)
{
  throw std::runtime_error (
    fmt::format (
      "CompositeObjectRegistry refuses registration of {}: register objects "
      "in the registry that owns them",
      obj.raw_uuid ().toString ()));
}

void
CompositeObjectRegistry::acquire_reference_impl (const QUuid &id)
{
  if (primary_.contains (id))
    {
      primary_.acquire_reference (id);
    }
  else if (secondary_.contains (id))
    {
      secondary_.acquire_reference (id);
    }
  else
    {
      throw std::runtime_error (
        fmt::format ("acquire_reference: unknown UUID {}", id.toString ()));
    }
}

void
CompositeObjectRegistry::release_reference_impl (const QUuid &id)
{
  if (primary_.contains (id))
    {
      primary_.release_reference (id);
    }
  else if (secondary_.contains (id))
    {
      secondary_.release_reference (id);
    }
  else
    {
      throw std::runtime_error (
        fmt::format ("release_reference: unknown UUID {}", id.toString ()));
    }
}

UuidIdentifiableObject *
CompositeObjectRegistry::find_by_raw_uuid_impl (const QUuid &id) const
{
  if (auto * obj = primary_.find_by_raw_uuid (id))
    {
      return obj;
    }
  return secondary_.find_by_raw_uuid (id);
}

bool
CompositeObjectRegistry::contains_impl (const QUuid &id) const
{
  return primary_.contains (id) || secondary_.contains (id);
}

void
CompositeObjectRegistry::for_each_matching_impl (
  const QMetaObject &meta_type,
  ObjectVisitor      visitor) const
{
  primary_.for_each_matching (meta_type, visitor);
  secondary_.for_each_matching (meta_type, visitor);
}

} // namespace zrythm::utils
