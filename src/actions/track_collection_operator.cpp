// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/track_collection_operator.h"
#include "commands/move_tracks_command.h"

namespace zrythm::actions
{

void
TrackCollectionOperator::moveTracks (
  const QList<structure::tracks::Track *> &tracks,
  int                                      targetPosition)
{
  if (!collection_ || !undo_stack_ || tracks.isEmpty ())
    return;

  // Convert Track* list to TrackUuidReference vector
  std::vector<structure::tracks::TrackUuidReference> track_refs;
  track_refs.reserve (tracks.size ());

  auto &registry = collection_->get_track_registry ();
  for (auto * track : tracks)
    {
      if (track != nullptr)
        {
          track_refs.emplace_back (track->get_uuid (), registry);
        }
    }

  if (track_refs.empty ())
    return;

  auto cmd = std::make_unique<commands::MoveTracksCommand> (
    *collection_, std::move (track_refs), targetPosition);
  undo_stack_->push (cmd.release ());
}

} // namespace zrythm::actions
