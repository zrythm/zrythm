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

#include "audio/tempo_track.h"
#include "audio/track.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "helpers/plugin_manager.h"
#include "helpers/project.h"
#include "helpers/zrythm.h"

#include <locale.h>

static void
test_empty_save_load (void)
{
  test_helper_zrythm_init ();

  int ret;
  g_assert_nonnull (PROJECT);

  /* save and reload the project */
  test_project_save_and_reload ();

  /* resave it */
  ret = project_save (
    PROJECT, PROJECT->dir, F_NOT_BACKUP, 0,
    F_NO_ASYNC);
  g_assert_cmpint (ret, ==, 0);

  test_helper_zrythm_cleanup ();
}

static void
test_save_load_with_data (void)
{
  test_helper_zrythm_init ();

  int ret;
  g_assert_nonnull (PROJECT);

  /* add some data */
  Position p1, p2;
  test_project_rebootstrap_timeline (&p1, &p2);

  /* save the project */
  ret = project_save (
    PROJECT, PROJECT->dir, 0, 0, F_NO_ASYNC);
  g_assert_cmpint (ret, ==, 0);
  char * prj_file = g_build_filename (
    PROJECT->dir, PROJECT_FILE, NULL);

  /* stop the engine */
  EngineState state;
  engine_wait_for_pause (
    PROJECT->audio_engine, &state, true);

  /* remove objects */
  chord_track_clear (P_CHORD_TRACK);
  marker_track_clear (P_MARKER_TRACK);
  tempo_track_clear (P_TEMPO_TRACK);
  for (int i = TRACKLIST->num_tracks - 1;
       i > P_MASTER_TRACK->pos; i--)
    {
      Track * track = TRACKLIST->tracks[i];
      tracklist_remove_track (
        TRACKLIST, track, 1, 1, 0, 0);
    }
  track_clear (P_MASTER_TRACK);

  /* reload the project */
  ret = project_load (prj_file, 0);
  g_assert_cmpint (ret, ==, 0);

  /* stop the engine */
  engine_resume (PROJECT->audio_engine, &state);

  /* verify that the data is correct */
  test_project_check_vs_original_state (
    &p1, &p2, 0);

  test_helper_zrythm_cleanup ();
}

static void
test_new_from_template (void)
{
  test_helper_zrythm_init ();

  /* add plugins */
#ifdef HAVE_HELM
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);
#  ifdef HAVE_CARLA
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, true, 1);
#  endif
#endif

  test_project_save_and_reload ();

  /* create a new project using old one as
   * template */
  char * orig_dir = g_strdup (PROJECT->dir);
  g_assert_nonnull (orig_dir);
  char * filepath = g_build_filename (
    orig_dir, "project.zpj", NULL);
  g_free_and_null (ZRYTHM->create_project_path);
  ZRYTHM->create_project_path = g_dir_make_tmp (
    "zrythm_test_project_XXXXXX", NULL);
  project_load (filepath, true);

  io_rmdir (orig_dir, true);

  test_helper_zrythm_cleanup ();
}

static void
test_save_as_load_w_pool (void)
{
  test_helper_zrythm_init ();

  Position p1, p2;
  test_project_rebootstrap_timeline (&p1, &p2);

  /* save the project elsewhere */
  char * orig_dir = g_strdup (PROJECT->dir);
  g_assert_nonnull (orig_dir);
  char * new_dir = g_dir_make_tmp (
    "zrythm_test_project_XXXXXX", NULL);
  int ret = project_save (
    PROJECT, new_dir, false, false, F_NO_ASYNC);
  g_assert_cmpint (ret, ==, 0);

  /* free the project */
  object_free_w_func_and_null (
    project_free, PROJECT);

  /* load the new one */
  char * filepath = g_build_filename (
    new_dir, "project.zpj", NULL);
  ret = project_load (filepath, 0);
  g_assert_cmpint (ret, ==, 0);

  io_rmdir (orig_dir, true);

  test_helper_zrythm_cleanup ();
}

static void
test_save_backup_w_pool_and_plugins (void)
{
  test_helper_zrythm_init ();

  Position p1, p2;
  test_project_rebootstrap_timeline (&p1, &p2);

  /* add a plugin and create a duplicate track */
  int track_pos =
    test_plugin_manager_create_tracks_from_plugin (
      TEST_INSTRUMENT_BUNDLE_URI,
      TEST_INSTRUMENT_URI, true, true, 1);
  Track * track = TRACKLIST->tracks[track_pos];
  track_select (
    track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR,
    TRACKLIST->num_tracks, NULL);

  char * dir = g_strdup (PROJECT->dir);

  /* save a project backup */
  int ret = project_save (
    PROJECT, PROJECT->dir, F_BACKUP, false,
    F_NO_ASYNC);
  g_assert_cmpint (ret, ==, 0);
  g_assert_nonnull (PROJECT->backup_dir);
  char * backup_dir =
    g_strdup (PROJECT->backup_dir);

  /* free the project */
  object_free_w_func_and_null (
    project_free, PROJECT);

  /* load the backup directly */
  char * filepath = g_build_filename (
    backup_dir, PROJECT_FILE, NULL);
  ret = project_load (filepath, false);
  g_free (filepath);
  g_assert_cmpint (ret, ==, 0);

  /* test undo and redo cloning the track */
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* free the project */
  object_free_w_func_and_null (
    project_free, PROJECT);

  /* attempt to open the latest backup (mimic
   * behavior from UI) */
  ZRYTHM->open_newer_backup = true;
  filepath =
    g_build_filename (dir, "project.zpj", NULL);
  ret = project_load (filepath, false);
  g_free (filepath);
  g_assert_cmpint (ret, ==, 0);

  g_free (backup_dir);
  g_free (dir);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/project/"

  g_test_add_func (
    TEST_PREFIX
    "test save backup w pool and plugins",
    (GTestFunc) test_save_backup_w_pool_and_plugins);
  g_test_add_func (
    TEST_PREFIX "test new from template",
    (GTestFunc) test_new_from_template);
  g_test_add_func (
    TEST_PREFIX "test empty save load",
    (GTestFunc) test_empty_save_load);
  g_test_add_func (
    TEST_PREFIX "test save load with data",
    (GTestFunc) test_save_load_with_data);
  g_test_add_func (
    TEST_PREFIX "test save as load w pool",
    (GTestFunc) test_save_as_load_w_pool);

  return g_test_run ();
}
