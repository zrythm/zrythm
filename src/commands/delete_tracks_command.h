// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <unordered_set>
#include <vector>

#include "structure/tracks/track_collection.h"

#include <QUndoCommand>

namespace zrythm::commands
{

class DeleteTracksCommand : public QUndoCommand
{
public:
  static constexpr auto CommandId = 1776134295;

  DeleteTracksCommand (
    structure::tracks::TrackCollection                &collection,
    std::vector<structure::tracks::TrackUuidReference> track_refs);

  void undo () override;
  void redo () override;
  int  id () const override { return CommandId; }

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
