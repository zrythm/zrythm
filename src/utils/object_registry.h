// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
#pragma once

#include <memory>

#include "utils/iobject_registry.h"

namespace zrythm::utils
{

/**
 * @brief Concrete IObjectRegistry with QObject parent-child ownership and
 * reference counting.
 *
 * Stores UUID-identifiable objects in a flat map. When an object's ref count
 * drops to zero (via release_reference), the object is deleted.
 */
class ObjectRegistry : public QObject, public utils::IObjectRegistry
{
  Q_OBJECT

public:
  ObjectRegistry (QObject * parent = nullptr);
  ~ObjectRegistry () override;

  [[gnu::hot]] utils::UuidIdentifiableBase *
  find_by_raw_uuid_or_throw (const QUuid &id) const;

  int ref_count (const QUuid &id) const;

protected:
  void delete_object_by_id (const QUuid &id);

private:
  using ObjectVisitor = utils::IObjectRegistry::ObjectVisitor;

  void register_object_impl (utils::UuidIdentifiableBase &obj) override;
  void acquire_reference_impl (const QUuid &id) override;
  void release_reference_impl (const QUuid &id) override;

  [[gnu::hot]] utils::UuidIdentifiableBase *
  find_by_raw_uuid_impl (const QUuid &id) const override;

  bool contains_impl (const QUuid &id) const override;

  void
  for_each_matching_impl (const QMetaObject &meta_type, ObjectVisitor visitor)
    const override;

  struct Impl;
  bool                  destroying_ = false;
  std::unique_ptr<Impl> impl_;
};

} // namespace zrythm::utils
