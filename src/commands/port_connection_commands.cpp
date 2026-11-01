// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/port_connection_commands.h"
#include "utils/logger.h"

namespace zrythm::commands
{

AddPortConnectionCommand::AddPortConnectionCommand (
  structure::project::PortConnectionsManager &manager,
  const dsp::PortUuid                         src,
  const dsp::PortUuid                         dest,
  dsp::Port                                  &destination_port)
    : QUndoCommand (QObject::tr ("Connect Ports")), manager_ (manager),
      src_ (src), dest_ (dest), destination_port_ (destination_port),
      attachment_follows_connections_ (
        destination_port.attachment_follows_connections ())
{
}

int
AddPortConnectionCommand::id () const
{
  return CommandId;
}

void
AddPortConnectionCommand::undo ()
{
  if (attachment_follows_connections_ && manager_.get_num_sources (dest_) == 1)
    destination_port_.set_detached (true);
  manager_.remove_connection (src_, dest_);
}

void
AddPortConnectionCommand::redo ()
{
  if (attachment_follows_connections_)
    destination_port_.set_detached (false);
  manager_.add_default_connection (src_, dest_, false);
}

RemovePortConnectionCommand::RemovePortConnectionCommand (
  structure::project::PortConnectionsManager &manager,
  const dsp::PortUuid                         src,
  const dsp::PortUuid                         dest,
  dsp::Port                                  &destination_port)
    : QUndoCommand (QObject::tr ("Disconnect Ports")), manager_ (manager),
      src_ (src), dest_ (dest), destination_port_ (destination_port),
      attachment_follows_connections_ (
        destination_port.attachment_follows_connections ())
{
}

int
RemovePortConnectionCommand::id () const
{
  return CommandId;
}

void
RemovePortConnectionCommand::undo ()
{
  if (attachment_follows_connections_)
    destination_port_.set_detached (false);
  manager_.add_connection (src_, dest_, multiplier_, locked_, enabled_);
  if (auto * conn = manager_.get_connection (src_, dest_))
    {
      conn->bipolar_ = bipolar_;
      conn->audio_bus_channel_routing_ = routing_;
    }
}

void
RemovePortConnectionCommand::redo ()
{
  if (const auto * conn = manager_.get_connection (src_, dest_))
    {
      multiplier_ = conn->multiplier_;
      locked_ = conn->locked_;
      enabled_ = conn->enabled_;
      bipolar_ = conn->bipolar_;
      routing_ = conn->audio_bus_channel_routing_;
      if (
        attachment_follows_connections_ && manager_.get_num_sources (dest_) == 1)
        destination_port_.set_detached (true);
      manager_.remove_connection (src_, dest_);
    }
}

UpdatePortConnectionCommand::UpdatePortConnectionCommand (
  structure::project::PortConnectionsManager &manager,
  const dsp::PortUuid                         src,
  const dsp::PortUuid                         dest,
  float                                       multiplier,
  bool                                        enabled,
  bool                                        merges)
    : QUndoCommand (QObject::tr ("Update Port Connection")), manager_ (manager),
      src_ (src), dest_ (dest), multiplier_ (multiplier), enabled_ (enabled),
      merges_ (merges)
{
  const auto * conn = manager.get_connection (src, dest);
  z_return_if_fail (conn != nullptr);
  before_multiplier_ = conn->multiplier_;
  before_enabled_ = conn->enabled_;
}

int
UpdatePortConnectionCommand::id () const
{
  return CommandId;
}

bool
UpdatePortConnectionCommand::mergeWith (const QUndoCommand * other)
{
  const auto * update =
    dynamic_cast<const UpdatePortConnectionCommand *> (other);
  if (update == nullptr || !merges_ || !update->merges_)
    return false;
  if (
    type_safe::get (update->src_) != type_safe::get (src_)
    || type_safe::get (update->dest_) != type_safe::get (dest_))
    return false;

  const auto cur_time = std::chrono::steady_clock::now ();
  const auto duration =
    std::chrono::duration_cast<std::chrono::milliseconds> (
      cur_time - last_redo_timestamp_)
      .count ();
  if (duration > 1'000)
    {
      return false;
    }

  last_redo_timestamp_ = cur_time;
  // keep the earliest before-state, apply the latest values
  multiplier_ = update->multiplier_;
  enabled_ = update->enabled_;
  return true;
}

void
UpdatePortConnectionCommand::undo ()
{
  const auto * conn = manager_.get_connection (src_, dest_);
  z_return_if_fail (conn != nullptr);
  manager_.update_connection (
    src_, dest_, before_multiplier_, conn->locked_, before_enabled_);
}

void
UpdatePortConnectionCommand::redo ()
{
  const auto * conn = manager_.get_connection (src_, dest_);
  z_return_if_fail (conn != nullptr);
  manager_.update_connection (src_, dest_, multiplier_, conn->locked_, enabled_);
  last_redo_timestamp_ = std::chrono::steady_clock::now ();
}

} // namespace zrythm::commands
