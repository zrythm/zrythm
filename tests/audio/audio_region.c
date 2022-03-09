/*
 * Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/tracklist_selections.h"
#include "audio/midi_region.h"
#include "audio/region.h"
#include "audio/transport.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/io.h"
#include "zrythm.h"

#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

static void
test_fill_stereo_ports (void)
{
  test_helper_zrythm_init ();

  test_project_stop_dummy_engine ();

  Position pos;
  position_set_to_bar (&pos, 2);

  /* create audio track with region */
  char * filepath =
    g_build_filename (
      TESTS_SRCDIR,
      "test_start_with_signal.mp3", NULL);
  SupportedFile * file =
    supported_file_new_from_path (filepath);
  int num_tracks_before = TRACKLIST->num_tracks;
  track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, file, &pos,
    num_tracks_before, 1, NULL);

  Track * track =
    TRACKLIST->tracks[num_tracks_before];
  ZRegion * r = track->lanes[0]->regions[0];
  AudioClip * r_clip = audio_region_get_clip (r);

  StereoPorts * ports =
    stereo_ports_new_generic (
      false, "ports", "ports",
      PORT_OWNER_TYPE_AUDIO_ENGINE, NULL);
  port_allocate_bufs (ports->l);
  port_allocate_bufs (ports->r);

  transport_move_playhead (
    TRANSPORT, &pos, F_NO_PANIC, false,
    F_NO_PUBLISH_EVENTS);
  transport_add_to_playhead (
    TRANSPORT, -20);
  const EngineProcessTimeInfo time_nfo = {
    .g_start_frame =
      (unsigned_frame_t) PLAYHEAD->frames,
    .local_offset = 0, .nframes = 100 };
  audio_region_fill_stereo_ports (
    r, &time_nfo, ports);

  g_assert_true (
    audio_frames_empty (
      &ports->l->buf[0], 20));
  g_assert_true (
    audio_frames_empty (
      &ports->r->buf[0], 20));
  for (int i = 0; i < 80; i++)
    {
      float adj_multiplier =
        MIN (
          1.f,
          ((float) i /
           (float)
           AUDIO_REGION_BUILTIN_FADE_FRAMES));
      float adj_clip_frame_l =
        r_clip->ch_frames[0][i] * adj_multiplier;
      float adj_clip_frame_r =
        r_clip->ch_frames[1][i] * adj_multiplier;
      g_assert_true (
        math_floats_equal_epsilon (
          adj_clip_frame_l,
          ports->l->buf[20 + i], 0.00001f));
      g_assert_true (
        math_floats_equal_epsilon (
          adj_clip_frame_r,
          ports->l->buf[20 + i], 0.00001f));
    }

  object_free_w_func_and_null (
    stereo_ports_free, ports);

  test_helper_zrythm_cleanup ();
}

static void
test_change_samplerate (void)
{
  test_helper_zrythm_init ();

  Position pos;
  position_set_to_bar (&pos, 2);

  /* create audio track with region */
  char * filepath =
    g_build_filename (
      TESTS_SRCDIR,
      "test_start_with_signal.mp3", NULL);
  SupportedFile * file =
    supported_file_new_from_path (filepath);
  int num_tracks_before = TRACKLIST->num_tracks;
  track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, file, &pos,
    num_tracks_before, 1, NULL);

  /*Track * track =*/
    /*TRACKLIST->tracks[num_tracks_before];*/

  /* save the project */
  int ret =
    project_save (
      PROJECT, PROJECT->dir, 0, 0, F_NO_ASYNC);
  g_assert_cmpint (ret, ==, 0);
  char * prj_file =
    g_build_filename (
      PROJECT->dir, PROJECT_FILE, NULL);

  /* adjust the samplerate to be given at startup */
  zrythm_app->samplerate =
    (int)
    AUDIO_ENGINE->sample_rate * 2;

  object_free_w_func_and_null (
    project_free, PROJECT);

  /* reload */
  ret = project_load (prj_file, 0);

  /* stop engine to process manually */
  test_project_stop_dummy_engine ();

  position_from_frames (&pos, 301824);
  transport_move_playhead (
    TRANSPORT, &pos, F_NO_PANIC, false,
    F_NO_PUBLISH_EVENTS);
  transport_request_roll (TRANSPORT, true);

  /* process manually */
  engine_process (AUDIO_ENGINE, 256);

  test_helper_zrythm_cleanup ();
}

static void
test_load_project_with_selected_audio_region (void)
{
  test_helper_zrythm_init ();

  Position pos;
  position_set_to_bar (&pos, 2);

  /* create audio track with region */
  char * filepath =
    g_build_filename (
      TESTS_SRCDIR,
      "test_start_with_signal.mp3", NULL);
  SupportedFile * file =
    supported_file_new_from_path (filepath);
  int num_tracks_before = TRACKLIST->num_tracks;
  track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, file, &pos,
    num_tracks_before, 1, NULL);

  /* select region */
  Track * track =
    TRACKLIST->tracks[num_tracks_before];
  ZRegion * r =
    track->lanes[0]->regions[0];
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);

  test_project_save_and_reload ();

  test_helper_zrythm_cleanup ();
}

static void
test_load_project_with_different_sample_rate (void)
{
  int samplerates[] = { 48000, 44100 };

  for (int i = 0; i < 2; i++)
    {
      int samplerate_before = samplerates[i];

      /* create project @ 48000 Hz */
      _test_helper_zrythm_init (
        false, samplerate_before, 0);

      Position pos;
      position_set_to_bar (&pos, 2);

      /* create audio track with region */
      char * filepath =
        g_build_filename (
          TESTS_SRCDIR,
          "test_start_with_signal.mp3", NULL);
      SupportedFile * file =
        supported_file_new_from_path (filepath);
      int num_tracks_before = TRACKLIST->num_tracks;
      track_create_with_action (
        TRACK_TYPE_AUDIO, NULL, file, &pos,
        num_tracks_before, 1, NULL);

      Track * audio_track =
        TRACKLIST->tracks[num_tracks_before];
      arranger_object_print (
        (ArrangerObject *)
        audio_track->lanes[0]->regions[0]);

      /* reload project @ 44100 Hz */
      zrythm_app->samplerate =
        samplerates[i == 0 ? 1 : 0];
      test_project_save_and_reload ();

      /* play the region */
      Position end;
      position_set_to_bar (&end, 4);
      transport_request_roll (TRANSPORT, true);
      while (position_is_before (PLAYHEAD, &end))
        {
          engine_wait_n_cycles (AUDIO_ENGINE, 3);
        }
      transport_request_pause (TRANSPORT, true);
      engine_wait_n_cycles (AUDIO_ENGINE, 1);
    }

  test_helper_zrythm_cleanup ();
}

static void
test_detect_bpm (void)
{
  test_helper_zrythm_init ();

  Position pos;
  position_set_to_bar (&pos, 2);

  /* create audio track with region */
  char * filepath =
    g_build_filename (
      TESTS_SRCDIR,
      "test_start_with_signal.mp3", NULL);
  SupportedFile * file =
    supported_file_new_from_path (filepath);
  int num_tracks_before = TRACKLIST->num_tracks;
  Track * track =
    track_create_with_action (
      TRACK_TYPE_AUDIO, NULL, file, &pos,
      num_tracks_before, 1, NULL);
  supported_file_free (file);

  /* select region */
  ZRegion * r =
    track->lanes[0]->regions[0];
  arranger_object_select (
    (ArrangerObject *) r, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);

  float bpm = audio_region_detect_bpm (r, NULL);
  g_assert_cmpfloat_with_epsilon (
    bpm, 186.233093f, 0.001f);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/audio_region/"

  g_test_add_func (
    TEST_PREFIX "test load project with lower sample rate",
    (GTestFunc) test_load_project_with_different_sample_rate);
  g_test_add_func (
    TEST_PREFIX "test load project with selected audio region",
    (GTestFunc) test_load_project_with_selected_audio_region);
  g_test_add_func (
    TEST_PREFIX "test change samplerate",
    (GTestFunc) test_change_samplerate);
  g_test_add_func (
    TEST_PREFIX "test fill stereo ports",
    (GTestFunc) test_fill_stereo_ports);
  g_test_add_func (
    TEST_PREFIX "test detect bpm",
    (GTestFunc) test_detect_bpm);

  return g_test_run ();
}
