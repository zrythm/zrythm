// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "dsp/transport.h"

#include "operations/operation_context.h"
#include "operations/operation_registry.h"
#include "operations/param_types.h"

namespace zrythm::operations
{

/**
 * Sets the transport play state. Control; takes the requested state.
 */
inline const auto &
transport_set_play_state_operation ()
{
  static const auto operation = TypedOperation (
    OperationKind::Control, "transport.set_play_state",
    "Set the transport play state (rolling, pause requested, ...)",
    std::array<ParamSpec, 1>{ { { "state", "Requested play state" } } },
    [] (OperationContext &ctx, EnumParam<dsp::Transport::PlayState> state)
      -> OperationReport {
      auto * transport = ctx.environment ().transport ();
      if (transport == nullptr)
        {
          return ctx.refused ("this session has no transport");
        }
      transport->setPlayState (state.value ());
      return ctx.applied ();
    });
  return operation;
}

/** The transport.* operations. */
std::vector<Operation>
make_transport_operations ();

} // namespace zrythm::operations
