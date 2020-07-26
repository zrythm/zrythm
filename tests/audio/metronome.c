/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-test-config.h"

#include "audio/metronome.h"
#include "audio/position.h"
#include "utils/math.h"

#include "tests/helpers/zrythm.h"

static void
test_find_and_queue_metronome ()
{
  Position play_pos = {
    .bars = 11, .beats = 1, .sixteenths = 1,
    .ticks = 0, .sub_tick = 0.26567493534093956 };
  position_update_ticks_and_frames (&play_pos);
  transport_set_playhead_pos (
    TRANSPORT, &play_pos);
  metronome_queue_events (
    AUDIO_ENGINE, 0, AUDIO_ENGINE->block_length);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/audio/metronome/"

  g_test_add_func (
    TEST_PREFIX "test find and queue metronome",
    (GTestFunc) test_find_and_queue_metronome);

  return g_test_run ();
}
