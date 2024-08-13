// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dsp/audio_track.h"
#include "project.h"
#include "zrythm.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("dsp/audio track");

constexpr int BUFFER_SIZE = 20;
constexpr int LARGE_BUFFER_SIZE = 2000;

constexpr int LOOP_BAR = 4;

TEST_CASE ("fill when region starts on loop end")
{
  test_helper_zrythm_init ();

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

  StereoPorts ports (
    false, "ports", "ports", PortIdentifier::OwnerType::Track, track);
  ports.allocate_bufs ();

  /* run until loop end and make sure sample is
   * never played */
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
      REQUIRE_FLOAT_NEAR (ports.get_l ().buf_[j], 0.f, 0.0000001f);
      REQUIRE_FLOAT_NEAR (ports.get_r ().buf_[j], 0.f, 0.0000001f);
    }

  /* run after loop end and make sure sample is
   * played */
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
          REQUIRE_FLOAT_EQ (ports.get_l ().buf_[j], 0.f);
          REQUIRE_FLOAT_EQ (ports.get_r ().buf_[j], 0.f);
        }
      else
        {
          REQUIRE_GT (std::abs (ports.get_l ().buf_[j]), 0.0000001f);
          REQUIRE_GT (std::abs (ports.get_r ().buf_[j]), 0.0000001f);
        }
    }

  test_helper_zrythm_cleanup ();
}

TEST_SUITE_END;