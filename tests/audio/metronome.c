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
#include "audio/sample_processor.h"
#include "utils/math.h"

#include "tests/helpers/zrythm.h"

static void
test_find_and_queue_metronome ()
{
  /* this catches a bug where if the ticks were 0
   * but sub_tick was not, invalid offsets would
   * be generated */
  {
    Position play_pos = {
      .bars = 11, .beats = 1, .sixteenths = 1,
      .ticks = 0, .sub_tick = 0.26567493534093956 };
    position_update_ticks_and_frames (&play_pos);
    transport_set_playhead_pos (
      TRANSPORT, &play_pos);
    metronome_queue_events (
      AUDIO_ENGINE, 0, AUDIO_ENGINE->block_length);

    g_assert_cmpint (
      SAMPLE_PROCESSOR->num_current_samples, ==, 0);
  }

  /*
   * from https://redmine.zrythm.org/issues/1824
   *
   * Cause: start_pos wraps remaining frames from
   * current loop to next loop. Every fourth loop,
   * it gets to 302,144 frames. Every fourth
   * measure, bar_pos = 302,400, so the difference
   * comes out to exactly 256, which is what causes
   * the failure. Here's the values at the end of
   * each measure and loop:
   *
   * Loop variable Measure 1 Measure 2 Measure 3
   * Measure 4
   * 1 bar_pos.frames 75600 151200 226800 302400
   * 1 start_pos.frames 75520 151040 226560 302336
   *
   * 2 bar_pos.frames 75600 151200 226800 302400
   * 2 start_pos.frames 75456 150976 226752 302272
   *
   * 3 bar_pos.frames 75600 151200 226800 302400
   * 3 start_pos.frames 75392 151168 226688 302208
   *
   * 4 bar_pos.frames 75600 151200 226800 302400
   * 4 start_pos.frames 75584 151104 226624 302144
   */
  {
    Position play_pos = {
      .bars = 4, .beats = 4, .sixteenths = 4,
      .ticks = 226,
      .sub_tick = 0.99682539682544302 };
    position_update_ticks_and_frames (&play_pos);
    transport_set_playhead_pos (
    TRANSPORT, &play_pos);
    metronome_queue_events (
    AUDIO_ENGINE, 0, AUDIO_ENGINE->block_length);

    /* assert no sound is played for 5.1.1.0 */
    g_assert_cmpint (
      SAMPLE_PROCESSOR->num_current_samples, ==, 0);

    /* go to the next round and verify that
     * metronome was added */
    transport_add_to_playhead (
      TRANSPORT, AUDIO_ENGINE->block_length);
    metronome_queue_events (
    AUDIO_ENGINE, 0, 1);

    /* assert metronome is queued for 1.1.1.0 */
    g_assert_cmpint (
      SAMPLE_PROCESSOR->num_current_samples, ==, 1);
  }
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
