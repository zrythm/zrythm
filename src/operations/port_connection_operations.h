// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "commands/port_connection_commands.h"

#include "operations/operation_context.h"
#include "operations/operation_registry.h"
#include "operations/param_types.h"

namespace zrythm::operations
{

namespace detail
{
/**
 * Returns why either port of the pair may not be edited, or no value
 * when both may.
 */
inline std::optional<utils::RefusalReason>
port_pair_edit_refusal (
  OperationContext &ctx,
  PortParam         source,
  PortParam         destination)
{
  for (const auto * port : { source.value (), destination.value () })
    {
      if (
        auto refusal = ctx.edit_target_refusal (
          type_safe::get (port->get_uuid ()), port->get_label ());
        refusal.has_value ())
        {
          return refusal;
        }
    }
  return std::nullopt;
}
} // namespace detail

/**
 * Connects two ports. Edit; takes the source and destination ports.
 * Refuses connections that duplicate an existing one, run against the
 * signal flow or would close a signal loop. Ports outside the project
 * registry are refused (the undo history is project-scoped).
 */
inline const auto &
port_connect_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit, "port.connect", "Connect two ports",
    std::array<ParamSpec, 2>{
      { { "source", "UUID of the source (output) port" },
       { "destination", "UUID of the destination (input) port" } }
  },
    [] (OperationContext &ctx, PortParam source, PortParam destination)
      -> OperationReport {
      if (
        const auto refusal =
          detail::port_pair_edit_refusal (ctx, source, destination);
        refusal.has_value ())
        {
          return ctx.refused (*refusal);
        }
      auto      &manager = ctx.environment ().port_connections_manager ();
      const auto refusal = manager.connect_refusal_reason (
        *source.value (), *destination.value (),
        [&ctx] (const dsp::Port &src, const dsp::Port &dest) {
          return ctx.environment ().port_wiring_refusal_reason (src, dest);
        });
      if (refusal.has_value ())
        {
          return ctx.refused (*refusal);
        }
      ctx.push_command (
        std::make_unique<commands::AddPortConnectionCommand> (
          manager, source.value ()->get_uuid (),
          destination.value ()->get_uuid (), *destination.value ()));
      ctx.note_affected (source.uuid_string (), "port");
      ctx.note_affected (destination.uuid_string (), "port");
      return ctx.applied ();
    });
  return operation;
}

/**
 * Removes a connection between two ports. Edit; takes the source and
 * destination ports. Refuses missing and locked connections. Ports
 * outside the project registry are refused (the undo history is
 * project-scoped).
 */
inline const auto &
port_disconnect_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit, "port.disconnect", "Remove a port connection",
    std::array<ParamSpec, 2>{
      { { "source", "UUID of the source port" },
       { "destination", "UUID of the destination port" } }
  },
    [] (OperationContext &ctx, PortParam source, PortParam destination)
      -> OperationReport {
      if (
        const auto refusal =
          detail::port_pair_edit_refusal (ctx, source, destination);
        refusal.has_value ())
        {
          return ctx.refused (*refusal);
        }
      auto      &manager = ctx.environment ().port_connections_manager ();
      const auto refusal =
        manager.edit_refusal_reason (*source.value (), *destination.value ());
      if (refusal.has_value ())
        {
          return ctx.refused (*refusal);
        }
      ctx.push_command (
        std::make_unique<commands::RemovePortConnectionCommand> (
          manager, source.value ()->get_uuid (),
          destination.value ()->get_uuid (), *destination.value ()));
      ctx.note_affected (source.uuid_string (), "port");
      ctx.note_affected (destination.uuid_string (), "port");
      return ctx.applied ();
    });
  return operation;
}

/**
 * Changes a connection's strength. Edit; takes the source and
 * destination ports and the new strength. Refuses MIDI connections
 * and strengths outside the destination's range; setting the current
 * strength elides. Consecutive changes to one connection merge into
 * a single undo step. Ports outside the project registry are refused
 * (the undo history is project-scoped).
 */
inline const auto &
port_set_connection_multiplier_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit, "port.set_connection_multiplier",
    "Change a port connection's strength",
    std::array<ParamSpec, 3>{
      { { "source", "UUID of the source port" },
       { "destination", "UUID of the destination port" },
       { "multiplier",
          "New strength (-2.0 to 2.0; CV destinations "
          "accept -1.0 to 1.0)" } }
  },
    [] (
      OperationContext &ctx, PortParam source, PortParam destination,
      BoundedFloatParam<-2.0, 2.0> multiplier) -> OperationReport {
      if (
        const auto refusal =
          detail::port_pair_edit_refusal (ctx, source, destination);
        refusal.has_value ())
        {
          return ctx.refused (*refusal);
        }
      auto      &manager = ctx.environment ().port_connections_manager ();
      const auto refusal = manager.multiplier_refusal_reason (
        *source.value (), *destination.value (), multiplier.value ());
      if (refusal.has_value ())
        {
          return ctx.refused (*refusal);
        }
      const auto   value = static_cast<float> (multiplier.value ());
      const auto * conn = manager.get_connection (
        source.value ()->get_uuid (), destination.value ()->get_uuid ());
      if (value == conn->multiplier_)
        {
          return ctx.applied ();
        }
      ctx.push_command (
        std::make_unique<commands::UpdatePortConnectionCommand> (
          manager, source.value ()->get_uuid (),
          destination.value ()->get_uuid (), value, conn->enabled_,
          /*merges=*/true));
      ctx.note_affected (source.uuid_string (), "port");
      ctx.note_affected (destination.uuid_string (), "port");
      return ctx.applied ();
    });
  return operation;
}

/**
 * Enables or disables a connection. Edit; takes the source and
 * destination ports and the new state. Refuses missing and locked
 * connections; setting the current state elides. Ports outside the
 * project registry are refused (the undo history is project-scoped).
 */
inline const auto &
port_set_connection_enabled_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Edit, "port.set_connection_enabled",
    "Enable or disable a port connection",
    std::array<ParamSpec, 3>{
      { { "source", "UUID of the source port" },
       { "destination", "UUID of the destination port" },
       { "enabled", "Whether the connection processes signal" } }
  },
    [] (
      OperationContext &ctx, PortParam source, PortParam destination,
      BoolParam enabled) -> OperationReport {
      if (
        const auto refusal =
          detail::port_pair_edit_refusal (ctx, source, destination);
        refusal.has_value ())
        {
          return ctx.refused (*refusal);
        }
      auto      &manager = ctx.environment ().port_connections_manager ();
      const auto refusal =
        manager.edit_refusal_reason (*source.value (), *destination.value ());
      if (refusal.has_value ())
        {
          return ctx.refused (*refusal);
        }
      const auto * conn = manager.get_connection (
        source.value ()->get_uuid (), destination.value ()->get_uuid ());
      if (conn->enabled_ == enabled.value ())
        {
          return ctx.applied ();
        }
      ctx.push_command (
        std::make_unique<commands::UpdatePortConnectionCommand> (
          manager, source.value ()->get_uuid (),
          destination.value ()->get_uuid (), conn->multiplier_,
          enabled.value (), /*merges=*/false));
      ctx.note_affected (source.uuid_string (), "port");
      ctx.note_affected (destination.uuid_string (), "port");
      return ctx.applied ();
    });
  return operation;
}

/** The port.* operations. */
std::vector<Operation>
make_port_connection_operations ();

} // namespace zrythm::operations
