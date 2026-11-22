// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
#pragma once

#include "utils/iobject_registry.h"

namespace zrythm::utils
{

/**
 * @brief An IObjectRegistry resolving across two constituent registries.
 *
 * Lookups consult the primary registry first and the secondary second.
 * Reference counting routes to the constituent that owns the UUID.
 * Iteration is a union: each constituent's matches are visited in turn,
 * so an object registered in both constituents is visited twice.
 * Registration through the composite is refused; objects register in
 * the registry that owns them.
 */
class CompositeObjectRegistry : public IObjectRegistry
{
public:
  CompositeObjectRegistry (IObjectRegistry &primary, IObjectRegistry &secondary);

private:
  void register_object_impl (UuidIdentifiableObject &obj) override;
  void acquire_reference_impl (const QUuid &id) override;
  void release_reference_impl (const QUuid &id) override;
  [[gnu::hot]] UuidIdentifiableObject *
       find_by_raw_uuid_impl (const QUuid &id) const override;
  bool contains_impl (const QUuid &id) const override;
  void
  for_each_matching_impl (const QMetaObject &meta_type, ObjectVisitor visitor)
    const override;

  IObjectRegistry &primary_;
  IObjectRegistry &secondary_;
};

} // namespace zrythm::utils
