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

#include "audio/track.h"
#include "audio/tempo_track.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "helpers/project.h"
#include "helpers/zrythm.h"

#include <glib.h>
#include <locale.h>

static void
test_empty_save_load ()
{
  int ret;
  g_assert_nonnull (PROJECT);

  /* save and reload the project */
  test_project_save_and_reload ();

  /* resave it */
  ret =
    project_save (
      PROJECT, PROJECT->dir, 0, 0, F_NO_ASYNC);
  g_assert_cmpint (ret, ==, 0);
}

static void
test_save_load_with_data ()
{
  int ret;
  g_assert_nonnull (PROJECT);

  /* add some data */
  Position p1, p2;
  test_project_rebootstrap_timeline (&p1, &p2);

  /* save the project */
  ret =
    project_save (
      PROJECT, PROJECT->dir, 0, 0, F_NO_ASYNC);
  g_assert_cmpint (ret, ==, 0);
  char * prj_file =
    g_build_filename (
      PROJECT->dir, PROJECT_FILE, NULL);

  /* remove objects */
  chord_track_clear (P_CHORD_TRACK);
  marker_track_clear (P_MARKER_TRACK);
  tempo_track_clear (P_TEMPO_TRACK);
  for (int i = TRACKLIST->num_tracks - 1;
       i >= 4; i--)
    {
      Track * track = TRACKLIST->tracks[i];
      tracklist_remove_track (
        TRACKLIST, track, 1, 1, 0, 0);
    }
  track_clear (P_MASTER_TRACK);

  /* reload the project */
  ret =
    project_load (prj_file, 0);
  g_assert_cmpint (ret, ==, 0);

  /* verify that the data is correct */
  test_project_check_vs_original_state (
    &p1, &p2, 0);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/project/"

  g_test_add_func (
    TEST_PREFIX "test empty save load",
    (GTestFunc) test_empty_save_load);

  g_test_add_func (
    TEST_PREFIX "test save load with data",
    (GTestFunc) test_save_load_with_data);

  return g_test_run ();
}
