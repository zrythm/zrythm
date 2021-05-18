/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
  UndoableAction * ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_AUDIO, NULL, file,
      num_tracks_before, &pos, 1, -1);
  undo_manager_perform (UNDO_MANAGER, ua);

  Track * track =
    TRACKLIST->tracks[num_tracks_before];
  ZRegion * r = track->lanes[0]->regions[0];
  AudioClip * r_clip = audio_region_get_clip (r);

  StereoPorts * ports =
    stereo_ports_new_generic (
      false, "ports", PORT_OWNER_TYPE_BACKEND,
      NULL);

  transport_move_playhead (
    TRANSPORT, &pos, F_NO_PANIC, false,
    F_NO_PUBLISH_EVENTS);
  transport_add_to_playhead (
    TRANSPORT, -20);
  audio_region_fill_stereo_ports (
    r, PLAYHEAD->frames, 0, 100, ports);

  g_assert_true (
    audio_frames_empty (
      &ports->l->buf[0], 20));
  g_assert_true (
    audio_frames_empty (
      &ports->r->buf[0], 20));
  g_assert_true (
    audio_frames_equal (
      &r_clip->ch_frames[0][0],
      &ports->l->buf[20], 80));
  g_assert_true (
    audio_frames_equal (
      &r_clip->ch_frames[1][0],
      &ports->r->buf[20], 80));

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/audio_region/"

  g_test_add_func (
    TEST_PREFIX "test fill stereo ports",
    (GTestFunc) test_fill_stereo_ports);

  return g_test_run ();
}
