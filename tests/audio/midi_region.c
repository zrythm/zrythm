/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "tests/helpers/zrythm.h"

static void
test_export (void)
{
  const int max_files = 20;

  char ** midi_files =
    io_get_files_in_dir_ending_in (
      MIDILIB_TEST_MIDI_FILES_PATH,
      F_RECURSIVE, ".MID", false);
  g_assert_nonnull (midi_files);
  char * export_dir =
    g_dir_make_tmp ("test_midi_export_XXXXXX", NULL);
  char * midi_file;
  int iter = 0;
  while ((midi_file = midi_files[iter++]))
    {
      g_message ("testing %s", midi_file);

      test_helper_zrythm_init ();

      SupportedFile * file =
        supported_file_new_from_path (midi_file);
      track_create_with_action (
        TRACK_TYPE_MIDI, NULL, file, PLAYHEAD,
        TRACKLIST->num_tracks, 1, NULL);
      supported_file_free (file);

      Track * track =
        tracklist_get_last_track (
          TRACKLIST, TRACKLIST_PIN_OPTION_BOTH,
          true);

      g_assert_cmpint (track->num_lanes, >, 0);
      g_assert_cmpint (
        track->lanes[0]->num_regions, >, 0);
      ZRegion * region =
        track->lanes[0]->regions[0];
      g_assert_true (IS_REGION (region));

      char * basename =
        g_path_get_basename (midi_file);
      char * export_filepath =
        g_build_filename (
          export_dir, basename, NULL);

      /* export the region again */
      midi_region_export_to_midi_file (
        region, export_filepath, 0, false, false);
      midi_region_export_to_midi_file (
        region, export_filepath, 0, true, false);

      g_assert_true (
        g_file_test (
          export_filepath,
          G_FILE_TEST_EXISTS |
            G_FILE_TEST_IS_REGULAR));

      io_remove (export_filepath);

      g_free (basename);
      g_free (export_filepath);

      test_helper_zrythm_cleanup ();

      if (iter == max_files)
        break;
    }
  g_strfreev (midi_files);

  io_rmdir (export_dir, true);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/midi_region/"

  g_test_add_func (
    TEST_PREFIX "test export",
    (GTestFunc) test_export);

  return g_test_run ();
}
