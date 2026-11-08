// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "actions/parameter_operator.h"
#include "commands/change_parameter_value_command.h"
#include "utils/logger.h"

namespace zrythm::actions
{

ProcessorParameterOperator::ProcessorParameterOperator (
  undo::UndoStack        &undo_stack,
  utils::IObjectRegistry &registry,
  QObject *               parent)
    : QObject (parent), undo_stack_ (&undo_stack), registry_ (&registry)
{
}

void
ProcessorParameterOperator::setValue (
  dsp::ProcessorParameter * param,
  float                     value)
{
  z_return_if_fail (param != nullptr);
  // The command acquires a registry reference for the parameter, which
  // throws for UUIDs the registry does not know. Refuse such edits here
  // so the failure surfaces as an error log instead of an exception
  // escaping into QML.
  if (!registry_->contains (type_safe::get (param->get_uuid ())))
    {
      z_error (
        "Refusing edit of parameter '{}': it is not registered in the project registry",
        param->label ());
      return;
    }
  undo_stack_->push (
    std::make_unique<commands::ChangeParameterValueCommand> (
      *param, *registry_, value)
      .release ());
}

} // namespace zrythm::actions
