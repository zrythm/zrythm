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
 * Sets a parameter's base value as live state. Control; takes the
 * parameter and the new normalized value. The change applies
 * immediately and is never undone, suiting runtime control rather
 * than document edits.
 */
inline const auto &
parameter_set_value_live_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Control,
    OperationId{
      "parameter", "set_value_live"
  },
    "Set a parameter's base value as live, non-undoable state",
    std::array<ParamSpec, 2>{
      { { "parameter", "UUID of the processor parameter" },
        { "value", "New base value in the normalized 0.0-1.0 range" } } },
    [] (
      OperationContext &ctx, ProcessorParameterParam parameter,
      BoundedFloatParam<0.0, 1.0> value) -> OperationReport {
      // Only parameters resolvable through the session's registry are
      // controllable through it
      if (!ctx.environment ().object_registry ().contains (
            type_safe::get (parameter.value ()->get_uuid ())))
        {
          return ctx.refused (
            utils::RefusalKind::NotFound,
            fmt::format (
              "'{}' is not registered in this session's registry",
              parameter.value ()->label ()));
        }
      // Mirrors ProcessorParameter::setBaseValue(), which fuzzy-compares
      // and stores nothing for equal values; skipping the write keeps
      // the affected list to actual changes
      if (
        qFuzzyCompare (
          parameter.value ()->baseValue (), static_cast<float> (value.value ())))
        {
          return ctx.applied ();
        }
      parameter.value ()->setBaseValueByUser (
        static_cast<float> (value.value ()));
      ctx.note_affected (parameter.uuid_string (), "parameter");
      return ctx.applied ();
    });
  return operation;
}

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
    OperationKind::Edit,
    OperationId{
      "parameter", "set_value"
  },
    "Set a parameter's base value",
    std::array<ParamSpec, 2>{
      { { "parameter", "UUID of the processor parameter" },
        { "value", "New base value in the normalized 0.0-1.0 range" } } },
    [] (
      OperationContext &ctx, ProcessorParameterParam parameter,
      BoundedFloatParam<0.0, 1.0> value) -> OperationReport {
      // the undo history is project-scoped: edits refuse application
      // state, which only live control accepts
      if (
        const auto refusal = ctx.edit_target_refusal (
          type_safe::get (parameter.value ()->get_uuid ()),
          utils::Utf8String::from_qstring (parameter.value ()->label ()),
          parameter_set_value_live_operation ().id ());
        refusal.has_value ())
        {
          return ctx.refused (*refusal);
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
          *parameter.value (), ctx.environment ().project_object_registry (),
          static_cast<float> (value.value ())));
      ctx.note_affected (parameter.uuid_string (), "parameter");
      return ctx.applied ();
    });
  return operation;
}

/** The parameter.* operations. */
std::vector<Operation>
make_parameter_operations ();

} // namespace zrythm::operations
