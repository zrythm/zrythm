// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <utility>

#include "commands/command.h"
#include "dsp/port.h"
#include "structure/tracks/channel_send.h"

namespace zrythm::commands
{

/**
 * @brief Command that points a channel send at an optional destination
 * port, or clears its destination.
 *
 * A null destination clears the send. The destination port must be an
 * input port of the send's signal type; the send enforces this when the
 * command runs.
 */
class RouteChannelSendCommand : public Command
{
public:
  RouteChannelSendCommand (
    structure::tracks::ChannelSendUuidReference send,
    std::optional<dsp::PortUuid>                target_port_id)
      : Command (
          QObject::tr ("Route Channel Send"),
          EngineRequirement::RecalculateGraph),
        send_ref_ (std::move (send)), target_port_id_ (std::move (target_port_id))
  {
  }

  void undo () override { apply (); }
  void redo () override { apply (); }

private:
  /** Applies the stored destination and keeps the one it replaced. */
  void apply ()
  {
    auto * send = send_ref_.get ();
    if (send == nullptr)
      {
        return;
      }

    const auto next = target_port_id_;
    const auto prev = send->destination_port ();
    target_port_id_ =
      prev.has_value () ? std::make_optional (prev->id ()) : std::nullopt;

    if (next.has_value ())
      {
        send->set_destination_port (
          dsp::PortUuidReference{ *next, send->registry () });
      }
    else
      {
        send->clear_destination_port ();
      }
  }

  structure::tracks::ChannelSendUuidReference send_ref_;
  std::optional<dsp::PortUuid>                target_port_id_;
};

} // namespace zrythm::commands
