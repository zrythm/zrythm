// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/transport.h"

#include "operations/operation_context.h"
#include "operations/param_types.h"
#include "operations/transport_operations.h"

namespace zrythm::operations
{

std::vector<Operation>
make_transport_operations ()
{
  return {
    TypedOperation (
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
      }),
  };
}

} // namespace zrythm::operations
