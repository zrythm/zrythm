// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/format_qt.h"

#include "commands/change_parameter_value_command.h"

#include "operations/operation_context.h"
#include "operations/operation_registry.h"
#include "operations/param_types.h"

namespace zrythm::operations
{

/**
 * Sets a parameter's base value. Edit; takes the parameter and the new
 * normalized value. Refuses parameters outside the project registry and
 * values outside the 0.0-1.0 range; setting the current value elides.
 * Consecutive edits of one parameter merge into a single undo step.
 */
inline const auto &
parameter_set_value_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit, "parameter.set_value", "Set a parameter's base value",
    std::array<ParamSpec, 2>{
      { { "parameter", "UUID of the processor parameter" },
       { "value", "New base value in the normalized 0.0-1.0 range" } }
  },
    [] (
      OperationContext &ctx, ProcessorParameterParam parameter,
      BoundedFloatParam<0.0, 1.0> value) -> OperationReport {
      auto &registry = ctx.environment ().object_registry ();
      // ChangeParameterValueCommand requires its parameter to be
      // registered in the project registry
      if (!registry.contains (type_safe::get (parameter.value ()->get_uuid ())))
        {
          return ctx.refused (
            utils::RefusalKind::NotFound,
            fmt::format (
              "'{}' is not registered in this project's registry",
              parameter.value ()->label ()));
        }
      // Mirrors ProcessorParameter::setBaseValue(), which fuzzy-compares
      // and stores nothing for equal values; skipping the command keeps
      // no-op edits off the undo stack
      if (
        qFuzzyCompare (
          parameter.value ()->baseValue (), static_cast<float> (value.value ())))
        {
          return ctx.applied ();
        }
      ctx.push_command (
        std::make_unique<commands::ChangeParameterValueCommand> (
          *parameter.value (), registry, static_cast<float> (value.value ())));
      ctx.note_affected (parameter.uuid_string (), "parameter");
      return ctx.applied ();
    });
  return operation;
}

/** The parameter.* operations. */
std::vector<Operation>
make_parameter_operations ();

} // namespace zrythm::operations
