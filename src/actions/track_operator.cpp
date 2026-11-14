// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "actions/track_operator.h"
#include "commands/change_timebase_override_command.h"
#include "commands/change_track_color_command.h"
#include "commands/change_track_comment_command.h"
#include "commands/change_uuid_identifiable_object_property_command.h"
#include "commands/channel_send_commands.h"
#include "commands/rename_track_command.h"
#include "commands/route_channel_send_command.h"
#include "commands/route_track_command.h"
#include "structure/project/project_graph_builder.h"
#include "utils/logger.h"
#include "utils/utf8_string.h"

#include "operations/operation_dispatcher.h"
#include "operations/track_operations.h"

namespace zrythm::actions
{
void
TrackOperator::rename (const QString &newName)
{
  if (track_ == nullptr || operation_dispatcher_ == nullptr)
    {
      z_warning ("Track rename needs a track and an operation dispatcher");
      return;
    }
  const auto report = operation_dispatcher_->invoke (
    operations::track_set_name_operation (), track_,
    utils::Utf8String::from_qstring (newName));
  if (!report.applied ())
    {
      z_warning ("Track rename refused: {}", report.message);
    }
}

void
TrackOperator::setColor (const QColor &color)
{
  if (!color.isValid ())
    {
      z_warning ("cannot set an invalid track color");
      return;
    }

  if (track_->useColor () && track_->color () == color)
    {
      return;
    }

  auto cmd = std::make_unique<commands::ChangeTrackColorCommand> (
    *track_, structure::tracks::Track::Color (color));
  undo_stack_->push (std::move (cmd));
}

void
TrackOperator::unsetColor ()
{
  if (!track_->useColor ())
    {
      return;
    }

  auto cmd =
    std::make_unique<commands::ChangeTrackColorCommand> (*track_, std::nullopt);
  undo_stack_->push (std::move (cmd));
}

void
TrackOperator::setComment (const QString &comment)
{
  auto cmd =
    std::make_unique<commands::ChangeTrackCommentCommand> (*track_, comment);
  undo_stack_->push (std::move (cmd));
}

void
TrackOperator::setOutputTrack (structure::tracks::Track * destination)
{
  if (!track_routing_ || !track_ || !undo_stack_)
    return;

  // Validate the routing before creating the command
  if (!track_routing_->canRouteTo (track_, destination))
    return;

  auto dest_id =
    destination ? std::make_optional (destination->get_uuid ()) : std::nullopt;

  auto cmd = std::make_unique<commands::RouteTrackCommand> (
    *track_routing_, track_->get_uuid (), dest_id);
  undo_stack_->push (std::move (cmd));
}

void
TrackOperator::setTimebase (dsp::Timebase timebase)
{
  if (track_ == nullptr || undo_stack_ == nullptr)
    return;

  auto cmd = std::make_unique<commands::ChangeTimebaseOverrideCommand> (
    *track_->timebaseProvider (), timebase);
  undo_stack_->push (std::move (cmd));
}

void
TrackOperator::addSend (bool isPrefader)
{
  if (
    track_ == nullptr || track_->channel () == nullptr || undo_stack_ == nullptr)
    return;

  undo_stack_->push (
    std::make_unique<commands::AddChannelSendCommand> (
      *track_->channel ()->sends (), isPrefader));
}

void
TrackOperator::removeSend (structure::tracks::ChannelSend * send)
{
  if (
    track_ == nullptr || track_->channel () == nullptr || send == nullptr
    || undo_stack_ == nullptr)
    return;

  undo_stack_->push (
    std::make_unique<commands::RemoveChannelSendCommand> (
      *track_->channel ()->sends (), send->get_uuid ()));
}

void
TrackOperator::setSendPrefader (
  structure::tracks::ChannelSend * send,
  bool                             isPrefader)
{
  if (send == nullptr || undo_stack_ == nullptr)
    return;

  if (send->isPrefader () == isPrefader)
    return;

  undo_stack_->push (
    std::make_unique<commands::ChangeUuidIdentifiableObjectPropertyCommand> (
      *send, send->registry (), QStringLiteral ("isPrefader"), isPrefader,
      commands::Command::EngineRequirement::RecalculateGraph));
}

void
TrackOperator::setSendTargetTrack (
  structure::tracks::ChannelSend * send,
  structure::tracks::Track *       destination)
{
  if (
    track_ == nullptr || channel_send_routing_ == nullptr
    || undo_stack_ == nullptr || send == nullptr)
    return;

  std::optional<dsp::PortUuid> target_port_id;
  if (destination != nullptr)
    {
      if (!channel_send_routing_->can_send_to_track (*track_, *destination))
        {
          z_warning (
            "cannot send from track '{}' to track '{}'", track_->name (),
            destination->name ());
          return;
        }
      const auto port =
        channel_send_routing_->destination_port_for_track (*destination);
      if (!port.has_value ())
        return;
      target_port_id = port->id ();

      if (!send_destination_is_connectable (*send, *port->get ()))
        return;
    }

  const auto send_ref = structure::tracks::ChannelSendUuidReference{
    send->get_uuid (), send->registry ()
  };

  undo_stack_->push (
    std::make_unique<commands::RouteChannelSendCommand> (
      send_ref, target_port_id));
}

void
TrackOperator::setSendTargetPort (
  structure::tracks::ChannelSend * send,
  dsp::AudioPort *                 destination)
{
  if (
    track_ == nullptr || channel_send_routing_ == nullptr
    || undo_stack_ == nullptr || send == nullptr)
    return;

  std::optional<dsp::PortUuid> target_port_id;
  if (destination != nullptr)
    {
      if (!channel_send_routing_->can_send_to_port (*track_, *destination))
        {
          z_warning (
            "cannot send from track '{}' to the given sidechain port",
            track_->name ());
          return;
        }
      target_port_id = destination->get_uuid ();

      if (!send_destination_is_connectable (*send, *destination))
        return;
    }

  const auto send_ref = structure::tracks::ChannelSendUuidReference{
    send->get_uuid (), send->registry ()
  };

  undo_stack_->push (
    std::make_unique<commands::RouteChannelSendCommand> (
      send_ref, target_port_id));
}

bool
TrackOperator::send_destination_is_connectable (
  const structure::tracks::ChannelSend &send,
  const dsp::Port                      &destination) const
{
  if (project_ == nullptr)
    {
      // Without a project there is no processing graph to validate against
      return true;
    }

  auto &out_port =
    send.is_audio ()
      ? static_cast<const dsp::Port &> (send.get_stereo_out_port ())
      : static_cast<const dsp::Port &> (send.get_midi_out_port ());
  if (!structure::project::ProjectGraphBuilder::can_ports_be_connected (
        *project_, out_port, destination))
    {
      z_warning (
        "refusing send routing that would close a signal loop or cannot be wired");
      return false;
    }

  return true;
}
}
