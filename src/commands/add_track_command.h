// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "commands/command.h"
#include "structure/tracks/track_collection.h"

namespace zrythm::commands
{
class AddEmptyTrackCommand : public Command
{
public:
  AddEmptyTrackCommand (
    structure::tracks::TrackCollection   &collection,
    structure::tracks::TrackUuidReference track_ref)
      : Command (QObject::tr ("Add Track"), EngineRequirement::RecalculateGraph),
        collection_ (collection), track_ref_ (std::move (track_ref))
  {
  }

  void undo () override { collection_.remove_track (track_ref_.id ()); }
  void redo () override { collection_.add_track (track_ref_); }

private:
  structure::tracks::TrackCollection   &collection_;
  structure::tracks::TrackUuidReference track_ref_;
};

} // namespace zrythm::commands
