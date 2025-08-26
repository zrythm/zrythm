// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/parameter_operator.h"
#include "commands/change_parameter_value_command.h"

namespace zrythm::actions
{
void
ProcessorParameterOperator::setValue (float value)
{
  auto cmd =
    std::make_unique<commands::ChangeParameterValueCommand> (*param_, value);
  undo_stack_->push (cmd.release ());
}
}
