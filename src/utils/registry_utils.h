// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
#pragma once

#include "utils/iobject_registry.h"
#include "utils/qt.h"
#include "utils/typed_uuid_reference.h"
#include "utils/uuid_identifiable_object.h"

namespace zrythm::utils
{

template <UuidIdentifiable T, typename... Args>
[[nodiscard]] auto
create_object (IObjectRegistry &registry, Args &&... args)
{
  auto obj = make_qobject_unique<T> (std::forward<Args> (args)...);
  auto uuid = obj->get_uuid ();
  registry.register_object (*obj);
  obj.release ();
  return TypedUuidReference<T> (uuid, registry);
}

template <UuidIdentifiable T>
[[nodiscard]] auto
clone_object (
  const T         &source,
  IObjectRegistry &registry,
  ObjectCloneType  clone_type,
  auto &&... extra_args)
{
  auto cloned =
    std::unique_ptr<T> (clone_raw_ptr (source, clone_type, extra_args...));
  auto uuid = cloned->get_uuid ();
  registry.register_object (*cloned);
  cloned.release ();
  return TypedUuidReference<T> (uuid, registry);
}

template <UuidIdentifiable T>
T &
get_typed (
  IObjectRegistry                                &registry,
  const typename UuidIdentifiableObject<T>::Uuid &id)
{
  auto * typed =
    qobject_cast<T *> (registry.find_by_raw_uuid (type_safe::get (id)));
  assert (typed != nullptr);
  return *typed;
}

template <UuidType T>
bool
contains (IObjectRegistry &registry, const T &id)
{
  return registry.contains (type_safe::get (id));
}

} // namespace zrythm::utils
