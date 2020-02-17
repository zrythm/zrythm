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

#include "actions/move_plugins_action.h"
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

#include "tests/helpers/zrythm.h"

#include <glib.h>

/**
 * Bootstraps the test with test data.
 */
static void
rebootstrap ()
{
  char path_str[6000];
  sprintf (
    path_str, "file://%s/%s",
    TESTS_BUILDDIR, "eg-amp.lv2/");
  g_message ("path is %s", path_str);

  plugin_manager_init (PLUGIN_MANAGER);
  LilvNode * path =
    lilv_new_uri (LILV_WORLD, path_str);
  lilv_world_load_bundle (
    LILV_WORLD, path);
  lilv_node_free (path);

  double progress = 0;
  plugin_manager_scan_plugins (
    PLUGIN_MANAGER, 1.0, &progress);
  g_assert_cmpint (PLUGIN_MANAGER->num_plugins, ==, 1);
}

static void
test_move_plugins ()
{
  g_assert_cmpint (TRACKLIST->num_tracks, ==, 3);
  UndoableAction * action =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO_BUS,
      PLUGIN_MANAGER->plugin_descriptors[0], NULL,
      3, 1);
  undo_manager_perform (UNDO_MANAGER, action);
  g_assert_cmpint (TRACKLIST->num_tracks, ==, 4);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/actions/mixer_selections/"

  rebootstrap ();
  g_test_add_func (
    TEST_PREFIX "test move plugins",
    (GTestFunc) test_move_plugins);

  return g_test_run ();
}
