// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/transport_controller.h"

namespace zrythm::controllers
{

void
TransportController::moveBackward ()
{
  const auto &playhead = transport_.playhead ()->playhead ();
  const auto &tempo_map = playhead.get_tempo_map ();

  auto pos_ticks = units::ticks (
    snap_grid_.prevSnapPoint (playhead.position_ticks ().in (units::ticks)));
  const auto pos_frames = tempo_map.tick_to_samples_rounded (pos_ticks);

  /* if prev snap point is exactly at the playhead or very close it, go back
   * more */
  const auto playhead_ticks = playhead.position_ticks ();
  const auto playhead_frames =
    tempo_map.tick_to_samples_rounded (playhead_ticks);

  if (
    pos_frames > units::samples (0)
    && (pos_frames == playhead_frames
        || (transport_.isRolling ()
            && (tempo_map.tick_to_seconds (playhead_ticks)
                  - tempo_map.tick_to_seconds (pos_ticks))
                 < REPEATED_BACKWARD_MS)))
    {
      pos_ticks = units::ticks (snap_grid_.prevSnapPoint (
        (pos_ticks - units::ticks (1.0)).in (units::ticks)));
    }

  transport_.move_playhead (pos_ticks, true);
}

void
TransportController::moveForward ()
{
  const auto &playhead = transport_.playhead ()->playhead ();
  double      pos_ticks =
    snap_grid_.nextSnapPoint (playhead.position_ticks ().in (units::ticks));
  transport_.move_playhead (units::ticks (pos_ticks), true);
}

} // namespace zrythm::controllers
