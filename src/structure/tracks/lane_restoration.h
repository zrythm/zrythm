// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>

#include "structure/tracks/track.h"
#include "structure/tracks/track_lane.h"
#include "utils/typed_uuid_reference.h"

namespace zrythm::structure::tracks
{

/**
 * @brief Identifies a detached lane and the position it belongs at in
 * its track's lane list.
 *
 * Holds registry identities and a position only.
 */
struct LaneRestoration
{
  TrackUuidReference     track; ///< Track whose lane list owns the lane
  TrackLaneUuidReference lane;  ///< The lane to reinsert
  size_t                 index; ///< The lane's position in the track's list
};

/**
 * @brief Captures the position of @p lane_ref's lane in its track's
 * lane list.
 *
 * @return The capture, or no value when the lane is not attached to a
 * track's lane list.
 */
std::optional<LaneRestoration>
capture_lane_restoration (const std::optional<TrackLaneUuidReference> &lane_ref);

/**
 * @brief Reinserts @p restoration's lane into its track's lane list.
 *
 * No effect when @p restoration holds no data, its track or lane no
 * longer resolves, or the lane is still attached to a list. Failures
 * during reinsertion are logged and skipped.
 */
void
reattach_lane_if_detached (const std::optional<LaneRestoration> &restoration);

} // namespace zrythm::structure::tracks
