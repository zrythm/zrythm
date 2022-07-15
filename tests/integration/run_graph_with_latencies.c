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

/* This test was written because of an issue with not
 * post-processing nodes properly when no roll was
 * needed for them. This test will fail if it happens
 * again */

#include "zrythm-test-config.h"

#include "actions/tracklist_selections.h"
#include "actions/undo_manager.h"
#include "audio/control_port.h"
#include "audio/engine_dummy.h"
#include "audio/graph.h"
#include "audio/router.h"
#include "audio/tempo_track.h"
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

#ifdef HAVE_NO_DELAY_LINE
static Port *
get_delay_port (Plugin * pl)
{
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      if (string_is_equal_ignore_case (
            pl->in_ports[i]->id.label, "Delay Time"))
        {
          return pl->in_ports[i];
          break;
        }
    }
  g_assert_not_reached ();
}

static void
_test (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  /* 1. create instrument track */
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, is_instrument, with_carla, 1);

  Track * track = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  int     orig_track_pos = track->pos;

  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      NO_DELAY_LINE_BUNDLE, NO_DELAY_LINE_URI, with_carla);

  /* 2. add no delay line */
  mixer_selections_action_perform_create (
    PLUGIN_SLOT_INSERT, track_get_name_hash (track), 0,
    setting, 1, NULL);

  /* 3. set delay to high value */
  Plugin * pl = track->channel->inserts[0];
  Port *   port = get_delay_port (pl);
  ;
  control_port_set_val_from_normalized (port, 0.1f, false);

  /* let the engine run */
  g_usleep (1000000);
  nframes_t latency = pl->latency;
  g_assert_cmpint (latency, >, 0);

  /* recalculate graph to update latencies */
  router_recalc_graph (ROUTER, F_SOFT);
  GraphNode * node =
    graph_find_node_from_track (ROUTER->graph, track, false);
  g_assert_true (node);
  g_assert_cmpint (latency, ==, node->route_playback_latency);

  /* let the engine run */
  g_usleep (1000000);
  g_assert_cmpint (latency, ==, node->route_playback_latency);

  /* 3. start playback */
  transport_request_roll (TRANSPORT, true);

  /* let the engine run */
  g_usleep (4000000);

  /* 4. reload project */
  test_project_save_and_reload ();

  /* let the engine run */
  g_usleep (1000000);

  /* add another track with latency and check if
   * OK */
  setting = test_plugin_manager_get_plugin_setting (
    NO_DELAY_LINE_BUNDLE, NO_DELAY_LINE_URI, with_carla);
  Track * new_track = track_create_with_action (
    TRACK_TYPE_AUDIO_BUS, setting, NULL, NULL,
    TRACKLIST->num_tracks, 1, NULL);

  pl = new_track->channel->inserts[0];
  port = get_delay_port (pl);
  ;
  control_port_set_val_from_normalized (port, 0.2f, false);

  /* let the engine run */
  g_usleep (1000000);
  nframes_t latency2 = pl->latency;
  g_assert_cmpint (latency2, >, 0);
  g_assert_cmpint (latency2, >, latency);

  /* recalculate graph to update latencies */
  router_recalc_graph (ROUTER, F_SOFT);
  node = graph_find_node_from_track (
    ROUTER->graph, P_TEMPO_TRACK, false);
  g_assert_true (node);
  g_assert_cmpint (latency2, ==, node->route_playback_latency);
  node = graph_find_node_from_track (
    ROUTER->graph, new_track, false);
  g_assert_true (node);
  g_assert_cmpint (latency2, ==, node->route_playback_latency);

  /* let the engine run */
  g_usleep (1000000);
  g_assert_cmpint (latency2, ==, node->route_playback_latency);

  /* 3. start playback */
  transport_request_roll (TRANSPORT, true);

  /* let the engine run */
  g_usleep (4000000);

  /* set latencies to 0 and verify that the updated
   * latency for tempo is 0 */
  pl = new_track->channel->inserts[0];
  port = get_delay_port (pl);
  ;
  control_port_set_val_from_normalized (port, 0.f, false);
  track = TRACKLIST->tracks[orig_track_pos];
  pl = track->channel->inserts[0];
  port = get_delay_port (pl);
  ;
  control_port_set_val_from_normalized (port, 0.f, false);

  g_usleep (1000000);
  g_assert_cmpint (pl->latency, ==, 0);

  /* recalculate graph to update latencies */
  router_recalc_graph (ROUTER, F_SOFT);
  node = graph_find_node_from_track (
    ROUTER->graph, P_TEMPO_TRACK, false);
  g_assert_true (node);
  g_assert_cmpint (node->route_playback_latency, ==, 0);
}
#endif

static void
run_graph_with_playback_latencies (void)
{
#ifdef HAVE_NO_DELAY_LINE
#  ifdef HAVE_HELM
  _test (HELM_BUNDLE, HELM_URI, true, false);
#  endif
#endif
#if 0
#  ifdef HAVE_NO_DELAY_LINE
  _test (
    NO_DELAY_LINE_BUNDLE, NO_DELAY_LINE_URI, false,
    false);
#  endif
#endif
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/integration/run_graph_with_latencies/"

  g_test_add_func (
    TEST_PREFIX "run graph with playback latencies",
    (GTestFunc) run_graph_with_playback_latencies);

  return g_test_run ();
}
