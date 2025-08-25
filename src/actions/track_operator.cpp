// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/track_operator.h"
#include "commands/change_track_color_command.h"
#include "commands/rename_track_command.h"

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
}
