// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/midi_region.h"
#include "gui/dsp/region.h"
#include "gui/dsp/transport.h"

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

TEST_F (ZrythmFixture, ChangeSampleRate)
{
  Position pos;
  pos.set_to_bar (2);

  /* create audio track with region */
  FileDescriptor file (fs::path (TESTS_SRCDIR) / "test_start_with_signal.mp3");
  int            num_tracks_before = TRACKLIST->get_num_tracks ();
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &pos, num_tracks_before, 1, -1, nullptr);

  /*Track * track =*/
  /*TRACKLIST->tracks[num_tracks_before];*/

  /* save the project */
  PROJECT->save (PROJECT->dir_, false, false, false);
  auto prj_file = fs::path (PROJECT->dir_) / PROJECT_FILE;

  /* adjust the samplerate to be given at startup */
  zrythm_app->samplerate_ = (int) AUDIO_ENGINE->sample_rate_ * 2;

  AUDIO_ENGINE->activate (false);
  PROJECT.reset ();

  /* reload */
  test_project_reload (prj_file);

  /* stop engine to process manually */
  test_project_stop_dummy_engine ();

  pos.from_frames (301824);
  TRANSPORT->move_playhead (&pos, F_NO_PANIC, false, false);
  TRANSPORT->requestRoll (true);

  /* process manually */
  AUDIO_ENGINE->process (256);
}

TEST_F (ZrythmFixture, FillStereoPorts)
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
  ASSERT_EQ (r->track_name_hash_, track->get_name_hash ());
  ASSERT_EQ (r->get_track (), track);
  auto r_clip = r->get_clip ();

  StereoPorts ports (false, "ports", "ports");
  {
    AudioEngine * engine = nullptr;
    ports.set_owner (engine);
  }
  ports.allocate_bufs ();

  TRANSPORT->move_playhead (&pos, F_NO_PANIC, false, false);
  TRANSPORT->add_to_playhead (-20);
  const EngineProcessTimeInfo time_nfo = {
    .g_start_frame_ = (unsigned_frame_t) PLAYHEAD.frames_,
    .g_start_frame_w_offset_ = (unsigned_frame_t) PLAYHEAD.frames_,
    .local_offset_ = 0,
    .nframes_ = 100
  };
  r->fill_stereo_ports (time_nfo, ports);

  ASSERT_TRUE (audio_frames_empty (&ports.get_l ().buf_[0], 20));
  ASSERT_TRUE (audio_frames_empty (&ports.get_r ().buf_[0], 20));
  for (int i = 0; i < 80; i++)
    {
      float adj_multiplier =
        MIN (1.f, ((float) i / (float) AUDIO_REGION_BUILTIN_FADE_FRAMES));
      float adj_clip_frame_l =
        r_clip->ch_frames_.getSample (0, i) * adj_multiplier;
      float adj_clip_frame_r =
        r_clip->ch_frames_.getSample (1, i) * adj_multiplier;
      ASSERT_NEAR (adj_clip_frame_l, ports.get_l ().buf_[20 + i], 0.00001f);
      ASSERT_NEAR (adj_clip_frame_r, ports.get_l ().buf_[20 + i], 0.00001f);
    }
}

TEST_F (ZrythmFixture, LoadProjectWithSelectedAudioRegion)
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
  r->select (true, false, false);

  test_project_save_and_reload ();
}

class AudioRegionSampleRateTest
    : public ZrythmFixture,
      public ::testing::WithParamInterface<std::tuple<int, int>>
{
protected:
  AudioRegionSampleRateTest ()
      : ZrythmFixture (false, std::get<0> (GetParam ()), 0, false, true)
  {
  }
};

TEST_P (AudioRegionSampleRateTest, LoadProjectWithDifferentSampleRate)
{
  const int new_samplerate = std::get<1> (GetParam ());

  Position pos;
  pos.set_to_bar (2);

  /* create audio track with region */
  FileDescriptor file (fs::path (TESTS_SRCDIR) / "test_start_with_signal.mp3");
  int            num_tracks_before = TRACKLIST->get_num_tracks ();
  Track::create_with_action (
    Track::Type::Audio, nullptr, &file, &pos, num_tracks_before, 1, -1, nullptr);

  /* reload project @ 44100 Hz */
  zrythm_app->samplerate_ = new_samplerate;
  test_project_save_and_reload ();

  /* play the region */
  Position end;
  end.set_to_bar (4);
  TRANSPORT->requestRoll (true);
  while (PLAYHEAD < end)
    {
      AUDIO_ENGINE->wait_n_cycles (3);
    }
  TRANSPORT->requestPause (true);
  AUDIO_ENGINE->wait_n_cycles (1);
}

INSTANTIATE_TEST_SUITE_P (
  AudioRegion,
  AudioRegionSampleRateTest,
  ::testing::
    Values (std::make_tuple (48000, 44100), std::make_tuple (44100, 48000)));

TEST_F (ZrythmFixture, DetectBPM)
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
  r->select (true, false, false);

  std::vector<float> candidates;
  float              bpm = r->detect_bpm (candidates);
  ASSERT_NEAR (bpm, 186.233093f, 0.001f);
}
