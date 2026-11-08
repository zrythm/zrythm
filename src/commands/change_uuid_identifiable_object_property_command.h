// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "commands/change_qobject_property_command.h"
#include "utils/uuid_identifiable.h"
#include "utils/uuid_reference.h"

namespace zrythm::commands
{

/**
 * @brief Property-change command that keeps the target alive via UUID reference.
 *
 * Unlike @ref ChangeQObjectPropertyCommand (which stores a raw QObject&), this
 * variant holds a @ref utils::UuidReference whose RAII refcount prevents the
 * UUID-identifiable object from being deleted while the command is on the undo
 * stack. This makes it safe for objects whose lifetime is tied to a mutating
 * model (e.g. automation points that can be removed by other undo commands).
 *
 * The base class's raw QObject& remains valid because the UUID reference
 * prevents the registry from deleting the object (and transitively, any
 * sub-objects it owns).
 *
 * Never merges with the plain @ref ChangeQObjectPropertyCommand: the base
 * only merges commands of its exact class, so the refcount safety of this
 * variant is never lost to a merge.
 */
class ChangeUuidIdentifiableObjectPropertyCommand
    : public ChangeQObjectPropertyCommand
{
public:
  /**
   * @brief Direct case: the UUID-identifiable object IS the target.
   *
   * @param uuid_object The UUID-identifiable object to modify and keep
   * alive.
   * @param registry The object registry that owns @p uuid_object.
   * @param property_name The Q_PROPERTY name to change.
   * @param value The new value.
   * @param engine_requirement Engine cooperation the change needs.
   */
  ChangeUuidIdentifiableObjectPropertyCommand (
    utils::UuidIdentifiableBase &uuid_object,
    utils::IObjectRegistry      &registry,
    QString                      property_name,
    QVariant                     value,
    EngineRequirement            engine_requirement)
      : ChangeQObjectPropertyCommand (
          uuid_object,
          std::move (property_name),
          std::move (value),
          engine_requirement),
        uuid_ref_ (uuid_object.raw_uuid (), registry)
  {
  }

  /**
   * @brief Sub-object case: the target is a child of a UUID-identifiable object.
   *
   * The UUID reference pins the parent alive, which transitively keeps the
   * target alive through QObject ownership.
   *
   * @param uuid_lifecycle_object The UUID-identifiable object whose lifetime
   *   guarantees @p target's lifetime.
   * @param target The actual QObject whose property is changed.
   * @param registry The object registry that owns @p uuid_lifecycle_object.
   * @param property_name The Q_PROPERTY name to change.
   * @param value The new value.
   * @param engine_requirement Engine cooperation the change needs.
   */
  ChangeUuidIdentifiableObjectPropertyCommand (
    utils::UuidIdentifiableBase &uuid_lifecycle_object,
    QObject                     &target,
    utils::IObjectRegistry      &registry,
    QString                      property_name,
    QVariant                     value,
    EngineRequirement            engine_requirement)
      : ChangeQObjectPropertyCommand (target, property_name, value, engine_requirement),
        uuid_ref_ (uuid_lifecycle_object.raw_uuid (), registry)
  {
  }

private:
  utils::UuidReference uuid_ref_;
};

} // namespace zrythm::commands
