// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/qobject_property_operator.h"
#include "commands/change_qobject_property_command.h"

namespace zrythm::actions
{
void
QObjectPropertyOperator::setValue (QString propertyName, QVariant value)
{
  auto cmd = std::make_unique<commands::ChangeQObjectPropertyCommand> (
    *current_object_, propertyName, value);
  undo_stack_->push (cmd.release ());
}

void
QObjectPropertyOperator::setValueAffectingTempoMap (
  QString  propertyName,
  QVariant value)
{
  auto cmd =
    std::make_unique<commands::ChangeTempoMapAffectingQObjectPropertyCommand> (
      *current_object_, propertyName, value);
  undo_stack_->push (cmd.release ());
}
}
