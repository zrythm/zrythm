/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
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
test_fill_when_region_starts_on_loop_end ()
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

  UndoableAction * ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_AUDIO, NULL, file,
      num_tracks_before, &TRANSPORT->loop_end_pos, 1);
  transport_request_pause (TRANSPORT);
  undo_manager_perform (UNDO_MANAGER, ua);
  /*transport_request_roll (TRANSPORT);*/
  TRANSPORT->play_state = PLAYSTATE_ROLLING;

  Track * track =
    TRACKLIST->tracks[num_tracks_before];
  StereoPorts * ports =
    stereo_ports_new_generic (
      false, "ports", PORT_OWNER_TYPE_TRACK, track);

  /* run until loop end and make sure sample is
   * never played */
  int nframes = 120;
  Position pos;
  position_set_to_bar (&pos, LOOP_BAR);
  position_add_frames (&pos, - nframes);
  track_fill_events (
    track, pos.frames, 0, (nframes_t) nframes,
    NULL, ports);
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
  track_fill_events (
    track, pos.frames, 0, (nframes_t) nframes,
    NULL, ports);
  for (int j = 0; j < nframes; j++)
    {
      g_assert_true (
        fabsf (ports->l->buf[j]) > 0.0000001f);
      g_assert_true (
        fabsf (ports->r->buf[j]) > 0.0000001f);
    }

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
