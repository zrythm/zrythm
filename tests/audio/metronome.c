// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "audio/metronome.h"
#include "audio/position.h"
#include "audio/sample_processor.h"
#include "utils/math.h"

#include "tests/helpers/zrythm.h"

static void
test_find_and_queue_metronome (void)
{
  test_helper_zrythm_init ();

  /* this catches a bug where if the ticks were 0
   * but sub_tick was not, invalid offsets would
   * be generated */
  {
    Position play_pos;
    position_set_to_bar (&play_pos, 11);
    position_add_ticks (&play_pos, 0.26567493534093956);
    position_update_frames_from_ticks (&play_pos);
    transport_set_playhead_pos (TRANSPORT, &play_pos);
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
    Position play_pos;
    position_set_to_bar (&play_pos, 4);
    position_add_beats (&play_pos, 3);
    position_add_sixteenths (&play_pos, 3);
    position_add_ticks (&play_pos, 226.99682539682544302);
    position_update_frames_from_ticks (&play_pos);
    transport_set_playhead_pos (TRANSPORT, &play_pos);
    metronome_queue_events (
      AUDIO_ENGINE, 0, AUDIO_ENGINE->block_length);

    /* assert no sound is played for 5.1.1.0 */
    g_assert_cmpint (
      SAMPLE_PROCESSOR->num_current_samples, ==, 0);

    /* go to the next round and verify that
     * metronome was added */
    transport_add_to_playhead (
      TRANSPORT, AUDIO_ENGINE->block_length);
    metronome_queue_events (AUDIO_ENGINE, 0, 1);

    /* assert metronome is queued for 1.1.1.0 */
    g_assert_cmpint (
      SAMPLE_PROCESSOR->num_current_samples, ==, 1);
  }

  {
    Position play_pos;
    position_set_to_bar (&play_pos, 16);
    position_add_frames (&play_pos, -4);
    transport_set_playhead_pos (TRANSPORT, &play_pos);

    for (nframes_t i = 0; i < 200; i++)
      {
        g_message ("%u", i);
        SAMPLE_PROCESSOR->num_current_samples = 0;
        metronome_queue_events (AUDIO_ENGINE, 0, 1);
        g_assert_cmpint (
          SAMPLE_PROCESSOR->num_current_samples, ==, i == 4);
        transport_add_to_playhead (TRANSPORT, 1);
      }

    position_set_to_bar (&play_pos, 16);
    position_add_frames (&play_pos, -4);
    transport_set_playhead_pos (TRANSPORT, &play_pos);
    for (nframes_t i = 0; i < 200; i++)
      {
        g_message ("%u", i);
        SAMPLE_PROCESSOR->num_current_samples = 0;
        metronome_queue_events (AUDIO_ENGINE, 0, 2);
        g_assert_cmpint (
          SAMPLE_PROCESSOR->num_current_samples, ==,
          (i >= 3 && i <= 4));
        transport_add_to_playhead (TRANSPORT, 1);
      }
  }

  /*
   * Situation: playhead is exactly block_length
   * frames before the loop end position.
   */
  for (int i = 0; i < 2; i++)
    {
      Position play_pos;
      position_set_to_bar (&play_pos, 16);
      position_add_frames (
        &play_pos, -(long) AUDIO_ENGINE->block_length);
      if (i == 1)
        position_add_ticks (&play_pos, -0.01);
      transport_set_playhead_pos (TRANSPORT, &play_pos);
      position_set_to_bar (&TRANSPORT->loop_end_pos, 16);

      /* assert no playback from playhead to loop
       * end */
      SAMPLE_PROCESSOR->num_current_samples = 0;
      metronome_queue_events (
        AUDIO_ENGINE, 0, AUDIO_ENGINE->block_length);
      g_assert_cmpint (
        SAMPLE_PROCESSOR->num_current_samples, ==, 0);

      /* add remaining frames and assert playback
       * at the first sample */
      transport_add_to_playhead (
        TRANSPORT, AUDIO_ENGINE->block_length);
      SAMPLE_PROCESSOR->num_current_samples = 0;
      metronome_queue_events (
        AUDIO_ENGINE, 0, AUDIO_ENGINE->block_length);
      g_assert_cmpint (
        SAMPLE_PROCESSOR->num_current_samples, ==, 1);
      SamplePlayback * sp =
        &SAMPLE_PROCESSOR->current_samples[0];
      g_assert_cmpuint (sp->start_offset, ==, 0);
    }

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/metronome/"

  g_test_add_func (
    TEST_PREFIX "test find and queue metronome",
    (GTestFunc) test_find_and_queue_metronome);

  return g_test_run ();
}
