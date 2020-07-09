/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/create_tracks_action.h"
#include "actions/undoable_action.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/master_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/project.h"

#include <glib.h>

static void
test_num_tracks_with_file (
  const char * filepath,
  const int    num_tracks)
{
  SupportedFile * file =
    supported_file_new_from_path (filepath);

  int num_tracks_before = TRACKLIST->num_tracks;

  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_MIDI, NULL, file,
      num_tracks_before, PLAYHEAD, 1);
  undo_manager_perform (
    UNDO_MANAGER, ua);

  g_assert_cmpint (
    TRACKLIST->num_tracks, ==,
    num_tracks_before + num_tracks);
}

static void
test_create_from_midi_file (void)
{
  char * midi_file;

  /* TODO this should pass in the future */
#if 0
  midi_file =
    g_build_filename (
      TESTS_SRCDIR,
      "1_empty_track_1_track_with_data.mid",
      NULL);
  test_num_tracks_with_file (midi_file, 2);
  g_free (midi_file);
#endif

  midi_file =
    g_build_filename (
      TESTS_SRCDIR,
      "1_track_with_data.mid",
      NULL);
  test_num_tracks_with_file (midi_file, 1);
  g_free (midi_file);

  g_assert_cmpint (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1]->
      lanes[0]->regions[0]->num_midi_notes, ==, 3);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/actions/arranger_selections/"

  g_test_add_func (
    TEST_PREFIX "test create from midi file",
    (GTestFunc) test_create_from_midi_file);

  return g_test_run ();
}
