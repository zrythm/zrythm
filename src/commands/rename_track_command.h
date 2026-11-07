// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "commands/command.h"
#include "structure/tracks/track.h"

namespace zrythm::commands
{
class RenameTrackCommand : public Command
{

public:
  RenameTrackCommand (structure::tracks::Track &track, QString name)
      : Command (QObject::tr ("Rename Track"), EngineRequirement::None),
        track_ (track), name_after_ (std::move (name))
  {
  }

  void undo () override { track_.setName (name_before_); }
  void redo () override
  {
    // note we are setting the previous track name here and not in the
    // constructor because the action may have been created but not performed
    // yet, and the track name may have changed meanwhile
    name_before_ = track_.name ();
    track_.setName (name_after_);
  }

private:
  structure::tracks::Track &track_;
  QString                   name_before_;
  QString                   name_after_;
};

} // namespace zrythm::commands
