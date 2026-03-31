// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "structure/tracks/track_collection.h"

#include <QUndoCommand>

namespace zrythm::commands
{

/**
 * @brief Command for moving tracks to a new position in the tracklist.
 *
 * Supports moving multiple tracks at once while maintaining their relative
 * order.
 */
class MoveTracksCommand : public QUndoCommand
{
public:
  /**
   * @brief Constructs a new MoveTracksCommand.
   *
   * @param collection Reference to the TrackCollection.
   * @param track_refs References to the tracks to move, in their current order.
   * @param target_position The target position for the first track.
   */
  MoveTracksCommand (
    structure::tracks::TrackCollection                &collection,
    std::vector<structure::tracks::TrackUuidReference> track_refs,
    int                                                target_position);

  void undo () override;
  void redo () override;
  int  id () const override;

private:
  structure::tracks::TrackCollection &collection_;

  /** Tracks being moved, in their original order. */
  std::vector<structure::tracks::TrackUuidReference> track_refs_;

  /** Original positions of the tracks (for undo). */
  std::vector<int> original_positions_;

  /** Target position for the first track. */
  int target_position_;
};

} // namespace zrythm::commands
