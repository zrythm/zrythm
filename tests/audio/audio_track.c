/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include <math.h>

#include "audio/engine_dummy.h"
#include "audio/audio_track.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

#include <glib.h>
#include <locale.h>

#define BUFFER_SIZE 20
#define LARGE_BUFFER_SIZE 2000

#define LOOP_BAR 4

static void
test_fill_when_region_starts_on_loop_end (void)
{
  test_helper_zrythm_init ();

  test_project_stop_dummy_engine ();

  TRANSPORT->play_state = PLAYSTATE_ROLLING;

  /* prepare loop */
  transport_set_loop (TRANSPORT, true);
  position_set_to_bar (
    &TRANSPORT->loop_end_pos, LOOP_BAR);
  position_set_to_bar (
    &TRANSPORT->loop_start_pos, LOOP_BAR);
  position_add_frames (
    &TRANSPORT->loop_start_pos, - 31);

  /* create audio track with region */
  char * filepath =
    g_build_filename (
      TESTS_SRCDIR,
      "test_start_with_signal.mp3", NULL);
  SupportedFile * file =
    supported_file_new_from_path (filepath);
  int num_tracks_before = TRACKLIST->num_tracks;

  transport_request_pause (TRANSPORT);
  Track * track =
    track_create_with_action (
      TRACK_TYPE_AUDIO, NULL, file,
      &TRANSPORT->loop_end_pos,
      num_tracks_before, 1, NULL);
  /*transport_request_roll (TRANSPORT);*/
  TRANSPORT->play_state = PLAYSTATE_ROLLING;

  StereoPorts * ports =
    stereo_ports_new_generic (
      false, "ports", "ports",
      PORT_OWNER_TYPE_TRACK, track);
  port_allocate_bufs (ports->l);
  port_allocate_bufs (ports->r);

  /* run until loop end and make sure sample is
   * never played */
  int nframes = 120;
  Position pos;
  position_set_to_bar (&pos, LOOP_BAR);
  position_add_frames (&pos, - nframes);
  EngineProcessTimeInfo time_nfo = {
    .g_start_frame =
      (unsigned_frame_t) pos.frames,
    .local_offset = 0,
    .nframes = (nframes_t) nframes, };
  track_fill_events (
    track, &time_nfo, NULL, ports);
  for (int j = 0; j < nframes; j++)
    {
      g_assert_cmpfloat_with_epsilon (
        ports->l->buf[j], 0.f, 0.0000001f);
      g_assert_cmpfloat_with_epsilon (
        ports->r->buf[j], 0.f, 0.0000001f);
    }

  /* run after loop end and make sure sample is
   * played */
  position_set_to_bar (&pos, LOOP_BAR);
  time_nfo.g_start_frame =
    (unsigned_frame_t) pos.frames;
  time_nfo.local_offset = 0;
  time_nfo.nframes = (nframes_t) nframes;
  track_fill_events (
    track, &time_nfo, NULL, ports);
  for (int j = 0; j < nframes; j++)
    {
      g_assert_true (
        fabsf (ports->l->buf[j]) > 0.0000001f);
      g_assert_true (
        fabsf (ports->r->buf[j]) > 0.0000001f);
    }

  object_free_w_func_and_null (
    stereo_ports_free, ports);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/audio_track/"

  g_test_add_func (
    TEST_PREFIX "test fill when region starts on loop end",
    (GTestFunc) test_fill_when_region_starts_on_loop_end);

  return g_test_run ();
}
