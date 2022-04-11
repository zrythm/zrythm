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
#include "actions/undo_manager.h"
#include "audio/engine_dummy.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

static void
_test (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  /* 1. add helm */
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, is_instrument, with_carla,
    1);

  /* select it */
  Track * helm_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    helm_track, F_SELECT, true,
    F_NO_PUBLISH_EVENTS);

  /* 2. delete track */
  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR,
    NULL);

  /* 3. undo track deletion */
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* let the engine run */
  g_usleep (1000000);

  /* 4. reload project */
  test_project_save_and_reload ();

  /* 5. redo track deletion */
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* 6. undo track deletion */
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* let the engine run */
  g_usleep (1000000);
}

static void
test (void)
{
#ifdef HAVE_HELM
  _test (HELM_BUNDLE, HELM_URI, true, false);
#endif
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX \
  "/integration/undo_redo_helm_track_creation/"

  g_test_add_func (
    TEST_PREFIX "test", (GTestFunc) test);

  return g_test_run ();
}
