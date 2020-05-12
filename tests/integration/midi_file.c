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

#include "audio/engine_dummy.h"
#include "audio/midi.h"
#include "audio/midi_track.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "zrythm.h"

#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

#include <glib.h>
#include <locale.h>

#define BUFFER_SIZE 20
#define LARGE_BUFFER_SIZE 2000

static void
test_midi_file_playback ()
{
  /* create a track for testing */
  Track * midi_track =
    track_new (
      TRACK_TYPE_MIDI,
      TRACKLIST->num_tracks,
      "Test MIDI Track 1",
      F_WITH_LANE);
  tracklist_append_track (
    TRACKLIST, midi_track, 0, 1);
  Port * port =
    port_new_with_type (
      TYPE_EVENT, FLOW_INPUT, "Test Port");
  MidiEvents * events =
    midi_events_new (port);

  char ** midi_files =
    io_get_files_in_dir_ending_in (
      MIDILIB_TEST_MIDI_FILES_PATH,
      F_RECURSIVE, ".MID");
  char * midi_file;
  int iter = 0;
  int max_files = 80;
  Position init_pos;
  position_set_to_bar (&init_pos, 1);
  while ((midi_file = midi_files[iter++]))
    {
      SupportedFile * file =
        supported_file_new_from_path (midi_file);
      UndoableAction * ua =
        create_tracks_action_new (
          TRACK_TYPE_MIDI, NULL, file,
          TRACKLIST->num_tracks, 1);
      undo_manager_perform (
        UNDO_MANAGER, ua);

      supported_file_free (file);

      if (iter == max_files)
        break;
    }
  g_strfreev (midi_files);

  test_project_save_and_reload ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/integration/midi_file/"

  g_test_add_func (
    TEST_PREFIX "test_midi_file_playback",
    (GTestFunc) test_midi_file_playback);

  return g_test_run ();
}

