// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>

#include "commands/move_tracks_command.h"

namespace zrythm::commands
{

MoveTracksCommand::MoveTracksCommand (
  structure::tracks::TrackCollection                &collection,
  std::vector<structure::tracks::TrackUuidReference> track_refs,
  int                                                target_position,
  std::optional<structure::tracks::Track::Uuid>      target_folder)
    : QUndoCommand (QObject::tr ("Move Tracks")), collection_ (collection),
      track_refs_ (std::move (track_refs)), target_position_ (target_position),
      target_folder_ (target_folder),
      moved_uuids_ (
        this->track_refs_
        | std::views::transform ([] (const auto &ref) { return ref.id (); })
        | std::ranges::to<std::unordered_set> ())
{
  // Store original positions for undo
  original_positions_ =
    track_refs_ | std::views::transform ([&] (const auto &ref) {
      return static_cast<int> (collection_.get_track_index (ref.id ()));
    })
    | std::ranges::to<std::vector> ();

  // Store original folder parents for undo
  original_folder_parents_ =
    track_refs_ | std::views::transform ([&] (const auto &ref) {
      return collection_.get_folder_parent (ref.id ());
    })
    | std::ranges::to<std::vector> ();

  // Store target folder's original expanded state for undo
  if (target_folder_.has_value ())
    {
      target_folder_was_expanded_ =
        collection_.get_track_expanded (target_folder_.value ());
    }
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

  // Restore original folder parents (only for tracks with external parents).
  // Internal relationships were never changed by redo().
  auto external_indices =
    std::views::iota (size_t{ 0 }, track_refs_.size ())
    | std::views::filter ([&] (size_t i) {
        return !is_internal_parent (original_folder_parents_[i]);
      });

  for (const auto i : external_indices)
    collection_.remove_folder_parent (track_refs_[i].id ());

  for (const auto i : external_indices)
    {
      if (original_folder_parents_[i].has_value ())
        {
          collection_.set_folder_parent (
            track_refs_[i].id (), original_folder_parents_[i].value ());
        }
    }

  // Restore target folder's original expanded state
  if (target_folder_.has_value ())
    {
      collection_.set_track_expanded (
        target_folder_.value (), target_folder_was_expanded_);
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
  // relative order. target_position_ is a pre-removal index, so subtract
  // the number of moved tracks that were above it.
  const auto above_count = std::ranges::count_if (
    current_positions, [this] (int pos) { return pos < target_position_; });
  int insert_pos = target_position_ - static_cast<int> (above_count);
  for (const auto &track_ref : track_refs_)
    {
      collection_.reattach_track (track_ref, insert_pos);
      ++insert_pos;
    }

  // Phase 3: Update folder parent relationships.
  // Check for circular nesting: skip if target folder is a descendant
  // of any moved track.
  const bool circular =
    target_folder_.has_value ()
    && std::ranges::any_of (track_refs_, [&] (const auto &ref) {
         return collection_.is_ancestor_of (ref.id (), target_folder_.value ());
       });

  if (!circular)
    {
      // Only modify tracks whose folder parent is NOT in the moved set
      // (external relationships). Internal relationships (parent is also
      // being moved) are preserved.
      for (const auto &ref : track_refs_)
        {
          if (!is_internal_parent (collection_.get_folder_parent (ref.id ())))
            collection_.remove_folder_parent (ref.id ());
        }

      // Set new folder parent for tracks with external parents
      if (target_folder_.has_value ())
        {
          for (const auto &ref : track_refs_)
            {
              if (
                !is_internal_parent (collection_.get_folder_parent (ref.id ())))
                {
                  collection_.set_folder_parent (
                    ref.id (), target_folder_.value ());
                }
            }

          // Auto-expand collapsed folder
          if (!collection_.get_track_expanded (target_folder_.value ()))
            {
              collection_.set_track_expanded (target_folder_.value (), true);
            }
        }
    }
}

int
MoveTracksCommand::id () const
{
  return 1774496026;
}

bool
MoveTracksCommand::is_internal_parent (
  const std::optional<structure::tracks::Track::Uuid> &parent) const
{
  return parent.has_value () && moved_uuids_.contains (parent.value ());
}

} // namespace zrythm::commands
