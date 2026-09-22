// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>

#include "structure/tracks/track_lane_list.h"

#include <QUndoCommand>

namespace zrythm::commands
{

/**
 * @brief Detaches a lane from its list and reattaches it on undo.
 *
 * Lane lists always end with an empty lane: removing the last lane is
 * only allowed when the remaining last lane is empty.
 *
 * The lane's reinsertion index is captured when the command first
 * executes. Undo reinserts in exact reverse of the executed removals,
 * which reconstructs the original order regardless of the order the
 * commands of one batch were constructed in.
 */
class DeleteLaneCommand : public QUndoCommand
{
public:
  /**
   * @brief Constructs a command detaching @p lane_ref from @p list.
   *
   * @throw std::invalid_argument if the lane is not in the list, is
   * the list's only lane, or removing it would leave the list without
   * a trailing empty lane.
   */
  DeleteLaneCommand (
    structure::tracks::TrackLaneList         &list,
    structure::tracks::TrackLaneUuidReference lane_ref);

  void undo () override;
  void redo () override;

private:
  structure::tracks::TrackLaneList &list_;

  structure::tracks::TrackLaneUuidReference lane_ref_;

  /** Reinsertion index, captured when the command first executes. */
  std::optional<size_t> original_index_;
};

} // namespace zrythm::commands
