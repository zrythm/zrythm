// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "common/dsp/metronome.h"
#include "common/dsp/position.h"
#include "common/dsp/sample_processor.h"

#include "tests/helpers/zrythm_helper.h"

TEST_F (ZrythmFixture, FindAndQueueMetronome)
{
  /* this catches a bug where if the ticks were 0
   * but sub_tick was not, invalid offsets would
   * be generated */
  {
    Position play_pos;
    play_pos.set_to_bar (11);
    play_pos.add_ticks (0.26567493534093956);
    play_pos.update_frames_from_ticks (0.0);
    TRANSPORT->set_playhead_pos (play_pos);
    AUDIO_ENGINE->metronome_->queue_events (
      AUDIO_ENGINE.get (), 0, AUDIO_ENGINE->block_length_);

    ASSERT_EMPTY (SAMPLE_PROCESSOR->current_samples_);
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
    play_pos.set_to_bar (4);
    play_pos.add_beats (3);
    play_pos.add_sixteenths (3);
    play_pos.add_ticks (226.99682539682544302);
    play_pos.update_frames_from_ticks (0.0);
    TRANSPORT->set_playhead_pos (play_pos);
    AUDIO_ENGINE->metronome_->queue_events (
      AUDIO_ENGINE.get (), 0, AUDIO_ENGINE->block_length_);

    /* assert no sound is played for 5.1.1.0 */
    ASSERT_EMPTY (SAMPLE_PROCESSOR->current_samples_);

    /* go to the next round and verify that metronome was added */
    TRANSPORT->add_to_playhead (AUDIO_ENGINE->block_length_);
    AUDIO_ENGINE->metronome_->queue_events (AUDIO_ENGINE.get (), 0, 1);

    /* assert metronome is queued for 1.1.1.0 */
    ASSERT_SIZE_EQ (SAMPLE_PROCESSOR->current_samples_, 1);
  }

  {
    Position play_pos;
    play_pos.set_to_bar (16);
    play_pos.add_frames (-4);
    TRANSPORT->set_playhead_pos (play_pos);

    for (nframes_t i = 0; i < 200; i++)
      {
        SAMPLE_PROCESSOR->current_samples_.clear ();
        AUDIO_ENGINE->metronome_->queue_events (AUDIO_ENGINE.get (), 0, 1);
        ASSERT_SIZE_EQ (SAMPLE_PROCESSOR->current_samples_, i == 4);
        TRANSPORT->add_to_playhead (1);
      }

    play_pos.set_to_bar (16);
    play_pos.add_frames (-4);
    TRANSPORT->set_playhead_pos (play_pos);
    for (nframes_t i = 0; i < 200; i++)
      {
        SAMPLE_PROCESSOR->current_samples_.clear ();
        METRONOME->queue_events (AUDIO_ENGINE.get (), 0, 2);
        ASSERT_SIZE_EQ (SAMPLE_PROCESSOR->current_samples_, (i >= 3 && i <= 4));
        TRANSPORT->add_to_playhead (1);
      }
  }

  /*
   * Situation: playhead is exactly block_length frames before the loop end
   * position.
   */
  for (int i = 0; i < 2; i++)
    {
      Position play_pos;
      play_pos.set_to_bar (16);
      play_pos.add_frames (-(long) AUDIO_ENGINE->block_length_);
      if (i == 1)
        play_pos.add_ticks (-0.01);
      TRANSPORT->set_playhead_pos (play_pos);
      TRANSPORT->loop_end_pos_.set_to_bar (16);

      /* assert no playback from playhead to loop end */
      SAMPLE_PROCESSOR->current_samples_.clear ();
      METRONOME->queue_events (
        AUDIO_ENGINE.get (), 0, AUDIO_ENGINE->block_length_);
      ASSERT_EMPTY (SAMPLE_PROCESSOR->current_samples_);

      /* add remaining frames and assert playback at the first sample */
      TRANSPORT->add_to_playhead (AUDIO_ENGINE->block_length_);
      SAMPLE_PROCESSOR->current_samples_.clear ();
      METRONOME->queue_events (
        AUDIO_ENGINE.get (), 0, AUDIO_ENGINE->block_length_);
      ASSERT_SIZE_EQ (SAMPLE_PROCESSOR->current_samples_, 1);
      const auto &sp = SAMPLE_PROCESSOR->current_samples_[0];
      ASSERT_EQ (sp.start_offset_, 0);
    }
}
