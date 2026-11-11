// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <utility>

#include "commands/command.h"
#include "structure/tracks/channel_send_group.h"

namespace zrythm::commands
{

/**
 * @brief Command that appends a new send to a channel's send list.
 */
class AddChannelSendCommand : public Command
{
public:
  AddChannelSendCommand (
    structure::tracks::ChannelSendGroup &group,
    bool                                 is_prefader)
      : Command (
          QObject::tr ("Add Channel Send"),
          EngineRequirement::RecalculateGraph),
        group_ (group), is_prefader_ (is_prefader)
  {
  }

  void redo () override
  {
    if (send_ref_.has_value ())
      {
        // Reinsert the send kept alive by undo, preserving its identity
        group_.insert_send (*send_ref_);
      }
    else
      {
        send_ref_ = group_.append_send (is_prefader_);
      }
  }

  void undo () override { group_.remove_send (send_ref_->id ()); }

private:
  structure::tracks::ChannelSendGroup                       &group_;
  bool                                                       is_prefader_;
  std::optional<structure::tracks::ChannelSendUuidReference> send_ref_;
};

/**
 * @brief Command that removes a send from a channel's send list.
 *
 * The command holds a reference to the send, so it stays registered and
 * undo reinserts it at its original position with its identity (and any
 * destination) intact.
 */
class RemoveChannelSendCommand : public Command
{
public:
  RemoveChannelSendCommand (
    structure::tracks::ChannelSendGroup &group,
    structure::tracks::ChannelSend::Uuid send_id)
      : Command (
          QObject::tr ("Remove Channel Send"),
          EngineRequirement::RecalculateGraph),
        group_ (group), send_ref_ (send_id, group.registry ())
  {
  }

  void redo () override
  {
    const auto &sends = group_.get_sends ();
    index_ = std::ranges::distance (
      sends.begin (),
      std::ranges::find (
        sends, send_ref_.id (),
        &structure::tracks::ChannelSendUuidReference::id));
    group_.remove_send (send_ref_.id ());
  }

  void undo () override { group_.insert_send (send_ref_, index_); }

private:
  structure::tracks::ChannelSendGroup &group_;

  /** Keeps the send registered while it is removed from the group. */
  structure::tracks::ChannelSendUuidReference send_ref_;

  /** Position the send had before removal; restored on undo. */
  int index_ = -1;
};

} // namespace zrythm::commands
