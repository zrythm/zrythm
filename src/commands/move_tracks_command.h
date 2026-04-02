// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <unordered_set>
#include <vector>

#include "structure/tracks/track_collection.h"

#include <QUndoCommand>

namespace zrythm::commands
{

/**
 * @brief Command for moving tracks to a new position in the tracklist.
 *
 * Supports moving multiple tracks at once while maintaining their relative
 * order. Folder parent relationships that are internal to the moved set (parent
 * is also being moved) are preserved. External relationships are updated:
 * when @p target_folder is specified, moved tracks become children of that
 * folder; when nullopt, external folder parents are cleared.
 */
class MoveTracksCommand : public QUndoCommand
{
public:
  /**
   * @brief Constructs a new MoveTracksCommand.
   *
   * @param collection Reference to the TrackCollection.
   * @param track_refs References to the tracks to move, in their current order.
   * @param target_position The index in the **current** (pre-removal) list
   * where the first moved track should end up. For example, if the collection
   * is [A, B, C, D] and tracks [A, B] are moved to between C and D, pass
   *   target_position = 3 (the position of D in the current list).
   *   The command adjusts internally for the removal of moved tracks.
   * @param target_folder The folder to move tracks into (nullopt = no folder
   *   change / clear external folder parents).
   */
  MoveTracksCommand (
    structure::tracks::TrackCollection                &collection,
    std::vector<structure::tracks::TrackUuidReference> track_refs,
    int                                                target_position,
    std::optional<structure::tracks::Track::Uuid> target_folder = std::nullopt);

  void undo () override;
  void redo () override;
  int  id () const override;

private:
  /**
   * @brief Returns true if @p parent is a folder parent of another track in the
   * moved set (i.e., the relationship is internal to the moved group).
   */
  bool is_internal_parent (
    const std::optional<structure::tracks::Track::Uuid> &parent) const;

private:
  structure::tracks::TrackCollection &collection_;

  /** Tracks being moved, in their original order. */
  std::vector<structure::tracks::TrackUuidReference> track_refs_;

  /** Original positions of the tracks (for undo). */
  std::vector<int> original_positions_;

  /** Target position for the first track. */
  int target_position_;

  /** The folder to move tracks into (nullopt = no folder). */
  std::optional<structure::tracks::Track::Uuid> target_folder_;

  /** Original folder parents of the moved tracks (for undo). */
  std::vector<std::optional<structure::tracks::Track::Uuid>>
    original_folder_parents_;

  /** Whether the target folder was expanded before redo (for undo). */
  bool target_folder_was_expanded_{ false };

  /** Set of moved track UUIDs for quick lookup. */
  std::unordered_set<structure::tracks::Track::Uuid> moved_uuids_;
};

} // namespace zrythm::commands
