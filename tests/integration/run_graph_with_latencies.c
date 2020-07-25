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

/* This test was written because of an issue with not
 * post-processing nodes properly when no roll was
 * needed for them. This test will fail if it happens
 * again */

#include "zrythm-test-config.h"

#include "actions/create_tracks_action.h"
#include "actions/undo_manager.h"
#include "audio/control_port.h"
#include "audio/engine_dummy.h"
#include "audio/router.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

#include <glib.h>

static void
_test (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  PluginDescriptor * descr =
    test_plugin_manager_get_plugin_descriptor (
      pl_bundle, pl_uri, with_carla);

  /* 1. add no delay line */
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO_BUS, descr, NULL,
      TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* 2. set delay to high value */
  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  Plugin * pl = track->channel->inserts[0];
  Port * port = pl->in_ports[1];
  control_port_set_val_from_normalized (
    port, 0.5f, false);

  /* let the engine run */
  g_usleep (1000000);

  /* recalculate graph to update latencies */
  router_recalc_graph (ROUTER, F_SOFT);

  /* let the engine run */
  g_usleep (1000000);

  /* 3. start playback */
  transport_request_roll (TRANSPORT);

  /* let the engine run */
  g_usleep (4000000);

  /* 4. reload project */
  test_project_save_and_reload ();

  /* let the engine run */
  g_usleep (1000000);
}

static void
run_graph_with_playback_latencies (void)
{
#ifdef HAVE_NO_DELAY_LINE
  _test (
    NO_DELAY_LINE_BUNDLE, NO_DELAY_LINE_URI, false,
    false);
#endif
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/integration/run_graph_with_latencies/"

  g_test_add_func (
    TEST_PREFIX "run graph with playback latencies",
    (GTestFunc) run_graph_with_playback_latencies);

  return g_test_run ();
}
