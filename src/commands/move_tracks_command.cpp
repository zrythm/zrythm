// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/move_tracks_command.h"
#include "utils/views.h"

namespace zrythm::commands
{

MoveTracksCommand::MoveTracksCommand (
  structure::tracks::TrackCollection                &collection,
  std::vector<structure::tracks::TrackUuidReference> track_refs,
  int                                                target_position)
    : QUndoCommand (QObject::tr ("Move Tracks")), collection_ (collection),
      track_refs_ (std::move (track_refs)), target_position_ (target_position)
{
  // Store original positions for undo
  original_positions_.reserve (track_refs_.size ());
  for (const auto &track_ref : track_refs_)
    {
      original_positions_.push_back (
        static_cast<int> (collection_.get_track_index (track_ref.id ())));
    }
}

void
MoveTracksCommand::undo ()
{
  // Move tracks back to their original positions.
  // Process in forward order - same as redo, using the stored original positions.
  for (
    const auto &[track_ref, original_pos] :
    std::views::zip (track_refs_, original_positions_))
    {
      collection_.move_track (track_ref.id (), original_pos);
    }
}

void
MoveTracksCommand::redo ()
{
  // target_position_ is where the first moved track should end up.
  // Subsequent tracks go after it, maintaining their relative order.
  // Move tracks in reverse order (highest target position first) to avoid
  // interfering with positions of tracks that haven't been moved yet.
  for (
    const auto &[i, track_ref] :
    utils::views::enumerate (track_refs_) | std::views::reverse)
    {
      // Calculate where this track should end up
      int target_for_track = target_position_ + static_cast<int> (i);
      collection_.move_track (track_ref.id (), target_for_track);
    }
}

int
MoveTracksCommand::id () const
{
  return 1774496026;
}

} // namespace zrythm::commands
