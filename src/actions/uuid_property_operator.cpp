// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/uuid_property_operator.h"
#include "commands/change_uuid_identifiable_object_property_command.h"
#include "utils/logger.h"

namespace zrythm::actions
{

void
UuidPropertyOperator::
  setValue (QObject * uuidObject, QString propertyName, QVariant value)
{
  if (uuidObject == nullptr)
    {
      z_warning ("uuidObject cannot be null");
      return;
    }

  setValueOnSubObject (uuidObject, uuidObject, propertyName, value);
}

void
UuidPropertyOperator::setValueOnSubObject (
  QObject * uuidLifecycleObject,
  QObject * target,
  QString   propertyName,
  QVariant  value)
{
  if (uuidLifecycleObject == nullptr)
    {
      z_warning ("uuidLifecycleObject cannot be null");
      return;
    }
  if (target == nullptr)
    {
      z_warning ("target cannot be null");
      return;
    }

  auto * uuid_obj =
    qobject_cast<utils::UuidIdentifiableObject *> (uuidLifecycleObject);
  if (uuid_obj == nullptr)
    {
      z_warning ("uuidLifecycleObject is not a UuidIdentifiableObject");
      return;
    }

  auto cmd = std::make_unique<
    commands::ChangeUuidIdentifiableObjectPropertyCommand> (
    *uuid_obj, *target, *registry_, propertyName, value,
    commands::Command::EngineRequirement::None);
  undo_stack_->push (std::move (cmd));
}

} // namespace zrythm::actions
