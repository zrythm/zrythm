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
  original_positions_ =
    track_refs_ | std::views::transform ([&] (const auto &ref) {
      return static_cast<int> (collection_.get_track_index (ref.id ()));
    })
    | std::ranges::to<std::vector> ();
}

void
MoveTracksCommand::undo ()
{
  // Same two-phase approach as redo(): remove all moved tracks, then insert
  // them at their original positions.
  auto current_positions =
    track_refs_ | std::views::transform ([&] (const auto &ref) {
      return static_cast<int> (collection_.get_track_index (ref.id ()));
    })
    | std::ranges::to<std::vector> ();

  // Remove in reverse current position order using zip to pair
  // indices with positions, then sort by position descending.
  auto indexed =
    std::views::zip (
      std::views::iota (size_t{ 0 }, track_refs_.size ()), current_positions)
    | std::ranges::to<std::vector> ();
  std::ranges::sort (indexed, std::greater{}, [] (const auto &p) {
    return std::get<1> (p);
  });
  for (auto [idx, _] : indexed)
    {
      collection_.detach_track (track_refs_[idx].id ());
    }

  // Insert at original positions in forward order
  auto orig_indexed =
    std::views::zip (
      std::views::iota (size_t{ 0 }, track_refs_.size ()), original_positions_)
    | std::ranges::to<std::vector> ();
  std::ranges::sort (orig_indexed, {}, [] (const auto &p) {
    return std::get<1> (p);
  });
  for (auto [idx, pos] : orig_indexed)
    {
      collection_.reattach_track (track_refs_[idx], pos);
    }
}

void
MoveTracksCommand::redo ()
{
  // Phase 1: Remove all tracks in reverse order of their current positions
  // to avoid index shifting during removal.
  auto current_positions =
    track_refs_ | std::views::transform ([&] (const auto &ref) {
      return static_cast<int> (collection_.get_track_index (ref.id ()));
    })
    | std::ranges::to<std::vector> ();

  auto indexed =
    std::views::zip (
      std::views::iota (size_t{ 0 }, track_refs_.size ()), current_positions)
    | std::ranges::to<std::vector> ();
  std::ranges::sort (indexed, std::greater{}, [] (const auto &p) {
    return std::get<1> (p);
  });
  for (auto [idx, _] : indexed)
    {
      collection_.detach_track (track_refs_[idx].id ());
    }

  // Phase 2: Insert all tracks at the target position in their original
  // relative order.
  int insert_pos = target_position_;
  for (const auto &track_ref : track_refs_)
    {
      collection_.reattach_track (track_ref, insert_pos);
      ++insert_pos;
    }
}

int
MoveTracksCommand::id () const
{
  return 1774496026;
}

} // namespace zrythm::commands
