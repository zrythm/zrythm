// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/track_operator.h"
#include "commands/change_timebase_override_command.h"
#include "commands/change_track_color_command.h"
#include "commands/change_track_comment_command.h"
#include "commands/rename_track_command.h"
#include "commands/route_track_command.h"
#include "utils/logger.h"

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
  undo_stack_->push (cmd.release ());
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

void
TrackOperator::setTimebase (dsp::Timebase timebase)
{
  if (track_ == nullptr || undo_stack_ == nullptr)
    return;

  auto cmd = std::make_unique<commands::ChangeTimebaseOverrideCommand> (
    *track_->timebaseProvider (), timebase);
  undo_stack_->push (cmd.release ());
}
}
