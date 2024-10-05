// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

#include "common/dsp/audio_track.h"

constexpr int LOOP_BAR = 4;

TEST_F (ZrythmFixture, FillWhenRegionStartsOnLoopEnd)
{
  test_project_stop_dummy_engine ();

  TRANSPORT->play_state_ = Transport::PlayState::Rolling;

  /* prepare loop */
  TRANSPORT->set_loop (true, true);
  TRANSPORT->loop_end_pos_.set_to_bar (LOOP_BAR);
  TRANSPORT->loop_start_pos_.set_to_bar (LOOP_BAR);
  TRANSPORT->loop_start_pos_.add_frames (-31);

  /* create audio track with region */
  FileDescriptor file (fs::path (TESTS_SRCDIR) / "test_start_with_signal.mp3");
  int            num_tracks_before = TRACKLIST->get_num_tracks ();

  TRANSPORT->request_pause (true);
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &TRANSPORT->loop_end_pos_,
    num_tracks_before, 1, -1, nullptr);
  auto track = TRACKLIST->get_track<AudioTrack> (num_tracks_before);
  /*transport_request_roll (TRANSPORT);*/
  TRANSPORT->play_state_ = Transport::PlayState::Rolling;

  StereoPorts ports (false, "ports", "ports");
  ports.set_owner (dynamic_cast<Track *> (track));
  ports.allocate_bufs ();

  /* run until loop end and make sure sample is never played */
  int      nframes = 120;
  Position pos;
  pos.set_to_bar (LOOP_BAR);
  pos.add_frames (-nframes);
  EngineProcessTimeInfo time_nfo = {
    .g_start_frame_ = (unsigned_frame_t) pos.frames_,
    .g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_,
    .local_offset_ = 0,
    .nframes_ = (nframes_t) nframes,
  };
  track->fill_events (time_nfo, ports);
  for (int j = 0; j < nframes; j++)
    {
      ASSERT_FLOAT_EQ (ports.get_l ().buf_[j], 0.f);
      ASSERT_FLOAT_EQ (ports.get_r ().buf_[j], 0.f);
    }

  /* run after loop end and make sure sample is played */
  pos.set_to_bar (LOOP_BAR);
  time_nfo.g_start_frame_ = (unsigned_frame_t) pos.frames_;
  time_nfo.g_start_frame_w_offset_ = (unsigned_frame_t) pos.frames_;
  time_nfo.local_offset_ = 0;
  time_nfo.nframes_ = (nframes_t) nframes;
  track->fill_events (time_nfo, ports);
  for (int j = 0; j < nframes; j++)
    {
      /* take into account builtin fades */
      if (j == 0)
        {
          ASSERT_FLOAT_EQ (ports.get_l ().buf_[j], 0.f);
          ASSERT_FLOAT_EQ (ports.get_r ().buf_[j], 0.f);
        }
      else
        {
          ASSERT_GT (std::abs (ports.get_l ().buf_[j]), 1e-10f);
          ASSERT_GT (std::abs (ports.get_r ().buf_[j]), 1e-10f);
        }
    }
  for (int i = 0; i < 2; ++i)
    {
      const auto clip_frames =
        track->lanes_[0]->regions_[0]->get_clip ()->ch_frames_.getReadPointer (i);
      const auto port_frames = ports.get_l ().buf_.data ();
      const auto last_sample_index = nframes - 1;
      // check greater than 0
      ASSERT_GT (std::abs (clip_frames[last_sample_index]), 1e-7f);
      ASSERT_GT (std::abs (port_frames[last_sample_index]), 1e-7f);
      ASSERT_FLOAT_EQ (
        clip_frames[last_sample_index], port_frames[last_sample_index]);
    }
}