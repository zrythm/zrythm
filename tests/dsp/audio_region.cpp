// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dsp/midi_region.h"
#include "dsp/region.h"
#include "dsp/transport.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("dsp/audio region");

TEST_CASE_FIXTURE (ZrythmFixture, "fill stereo ports")
{
  test_project_stop_dummy_engine ();

  Position pos;
  pos.set_to_bar (2);

  /* create audio track with region */
  auto filepath = fs::path (TESTS_SRCDIR) / "test_start_with_signal.mp3";
  FileDescriptor file (filepath);
  int            num_tracks_before = TRACKLIST->get_num_tracks ();
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &pos, num_tracks_before, 1, -1, nullptr);

  auto  track = TRACKLIST->get_track<AudioTrack> (num_tracks_before);
  auto &r = track->lanes_[0]->regions_[0];
  REQUIRE_EQ (r->track_name_hash_, track->get_name_hash ());
  REQUIRE_EQ (r->get_track (), track);
  auto r_clip = r->get_clip ();

  StereoPorts ports (false, "ports", "ports");
  {
    AudioEngine * engine = nullptr;
    ports.set_owner (engine);
  }
  ports.allocate_bufs ();

  TRANSPORT->move_playhead (&pos, F_NO_PANIC, false, F_NO_PUBLISH_EVENTS);
  TRANSPORT->add_to_playhead (-20);
  const EngineProcessTimeInfo time_nfo = {
    .g_start_frame_ = (unsigned_frame_t) PLAYHEAD.frames_,
    .g_start_frame_w_offset_ = (unsigned_frame_t) PLAYHEAD.frames_,
    .local_offset_ = 0,
    .nframes_ = 100
  };
  r->fill_stereo_ports (time_nfo, ports);

  REQUIRE (audio_frames_empty (&ports.get_l ().buf_[0], 20));
  REQUIRE (audio_frames_empty (&ports.get_r ().buf_[0], 20));
  for (int i = 0; i < 80; i++)
    {
      float adj_multiplier =
        MIN (1.f, ((float) i / (float) AUDIO_REGION_BUILTIN_FADE_FRAMES));
      float adj_clip_frame_l =
        r_clip->ch_frames_.getSample (0, i) * adj_multiplier;
      float adj_clip_frame_r =
        r_clip->ch_frames_.getSample (1, i) * adj_multiplier;
      REQUIRE_FLOAT_NEAR (
        adj_clip_frame_l, ports.get_l ().buf_[20 + i], 0.00001f);
      REQUIRE_FLOAT_NEAR (
        adj_clip_frame_r, ports.get_l ().buf_[20 + i], 0.00001f);
    }
}

TEST_CASE_FIXTURE (ZrythmFixture, "change samplerate")
{
  Position pos;
  pos.set_to_bar (2);

  /* create audio track with region */
  char * filepath =
    g_build_filename (TESTS_SRCDIR, "test_start_with_signal.mp3", nullptr);
  FileDescriptor file = FileDescriptor (filepath);
  int            num_tracks_before = TRACKLIST->get_num_tracks ();
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &pos, num_tracks_before, 1, -1, nullptr);

  /*Track * track =*/
  /*TRACKLIST->tracks[num_tracks_before];*/

  /* save the project */
  PROJECT->save (PROJECT->dir_, 0, 0, F_NO_ASYNC);
  auto prj_file = fs::path (PROJECT->dir_) / PROJECT_FILE;

  /* adjust the samplerate to be given at startup */
  zrythm_app->samplerate = (int) AUDIO_ENGINE->sample_rate_ * 2;

  PROJECT.reset ();

  /* reload */
  test_project_reload (prj_file);

  /* stop engine to process manually */
  test_project_stop_dummy_engine ();

  pos.from_frames (301824);
  TRANSPORT->move_playhead (&pos, F_NO_PANIC, false, F_NO_PUBLISH_EVENTS);
  TRANSPORT->request_roll (true);

  /* process manually */
  AUDIO_ENGINE->process (256);
}

TEST_CASE_FIXTURE (ZrythmFixture, "load project with selected audio region")
{
  Position pos;
  pos.set_to_bar (2);

  /* create audio track with region */
  FileDescriptor file (fs::path (TESTS_SRCDIR) / "test_start_with_signal.mp3");
  int            num_tracks_before = TRACKLIST->get_num_tracks ();
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &pos, num_tracks_before, 1, -1, nullptr);

  /* select region */
  auto  track = TRACKLIST->get_track<AudioTrack> (num_tracks_before);
  auto &r = track->lanes_[0]->regions_[0];
  r->select (F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);

  test_project_save_and_reload ();
}

TEST_CASE ("load project with different sample rate")
{
  int samplerates[] = { 48000, 44100 };

  for (int i = 0; i < 2; i++)
    {
      int samplerate_before = samplerates[i];

      /* create project @ 48000 Hz */
      ZrythmFixture (false, samplerate_before, 0, false);

      Position pos;
      pos.set_to_bar (2);

      /* create audio track with region */
      char * filepath =
        g_build_filename (TESTS_SRCDIR, "test_start_with_signal.mp3", nullptr);
      FileDescriptor file = FileDescriptor (filepath);
      int            num_tracks_before = TRACKLIST->get_num_tracks ();
      Track::create_with_action (
        Track::Type::Audio, nullptr, &file, &pos, num_tracks_before, 1, -1,
        nullptr);

      /* reload project @ 44100 Hz */
      zrythm_app->samplerate = samplerates[i == 0 ? 1 : 0];
      test_project_save_and_reload ();

      /* play the region */
      Position end;
      end.set_to_bar (4);
      TRANSPORT->request_roll (true);
      while (PLAYHEAD < end)
        {
          AUDIO_ENGINE->wait_n_cycles (3);
        }
      TRANSPORT->request_pause (true);
      AUDIO_ENGINE->wait_n_cycles (1);
    }
}

TEST_CASE_FIXTURE (ZrythmFixture, "detect BPM")
{
  Position pos;
  pos.set_to_bar (2);

  /* create audio track with region */
  FileDescriptor file (fs::path (TESTS_SRCDIR) / "test_start_with_signal.mp3");
  int            num_tracks_before = TRACKLIST->get_num_tracks ();
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &pos, num_tracks_before, 1, -1, nullptr);
  auto track = TRACKLIST->get_last_track<AudioTrack> ();

  /* select region */
  auto &r = track->lanes_[0]->regions_[0];
  r->select (F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);

  std::vector<float> candidates;
  float              bpm = r->detect_bpm (candidates);
  REQUIRE_FLOAT_NEAR (bpm, 186.233093f, 0.001f);
}

TEST_SUITE_END;