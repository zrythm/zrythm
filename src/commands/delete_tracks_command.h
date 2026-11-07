// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <unordered_set>
#include <vector>

#include "commands/command.h"
#include "structure/tracks/track_collection.h"

namespace zrythm::commands
{

class DeleteTracksCommand : public Command
{
public:
  DeleteTracksCommand (
    structure::tracks::TrackCollection                &collection,
    std::vector<structure::tracks::TrackUuidReference> track_refs);

  void undo () override;
  void redo () override;

private:
  struct TrackInfo
  {
    structure::tracks::TrackUuidReference         ref;
    int                                           original_position;
    std::optional<structure::tracks::Track::Uuid> original_folder_parent;
    bool                                          original_expanded_state;
  };

  structure::tracks::TrackCollection &collection_;

  std::vector<TrackInfo>                             tracks_;
  std::unordered_set<structure::tracks::Track::Uuid> deleted_uuids_;
};

} // namespace zrythm::commands
