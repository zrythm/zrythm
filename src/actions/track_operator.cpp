// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/track_operator.h"
#include "commands/change_track_color_command.h"
#include "commands/change_track_comment_command.h"
#include "commands/rename_track_command.h"
#include "commands/route_track_command.h"

namespace zrythm::actions
{
void
TrackOperator::rename (const QString &newName)
{
  auto cmd = std::make_unique<commands::RenameTrackCommand> (*track_, newName);
  undo_stack_->push (cmd.release ());
}

void
TrackOperator::setColor (const QColor &color)
{
  auto cmd =
    std::make_unique<commands::ChangeTrackColorCommand> (*track_, color);
  undo_stack_->push (cmd.release ());
}

void
TrackOperator::setComment (const QString &comment)
{
  auto cmd =
    std::make_unique<commands::ChangeTrackCommentCommand> (*track_, comment);
  undo_stack_->push (cmd.release ());
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
  undo_stack_->push (cmd.release ());
}
}
