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

#include "actions/port_connection_action.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/control_port.h"
#include "audio/master_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/project.h"

#include <glib.h>

static void
_test_port_connection (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  LilvNode * path =
    lilv_new_uri (LILV_WORLD, pl_bundle);
  lilv_world_load_bundle (
    LILV_WORLD, path);
  lilv_node_free (path);

  plugin_manager_scan_plugins (
    PLUGIN_MANAGER, 1.0, NULL);
  /*g_assert_cmpint (*/
    /*PLUGIN_MANAGER->num_plugins, ==, 1);*/

  PluginDescriptor * descr = NULL;
  for (int i = 0; i < PLUGIN_MANAGER->num_plugins;
       i++)
    {
      if (string_is_equal (
            PLUGIN_MANAGER->plugin_descriptors[i]->
              uri, pl_uri, true))
        {
          descr =
            plugin_descriptor_clone (
              PLUGIN_MANAGER->plugin_descriptors[i]);
        }
    }

  /* fix the descriptor (for some reason lilv
   * reports it as Plugin instead of Instrument if
   * you don't do lilv_world_load_all) */
  if (is_instrument)
    {
      descr->category = PC_INSTRUMENT;
    }
  g_free (descr->category_str);
  descr->category_str =
    plugin_descriptor_category_to_string (
      descr->category);

  /* open with carla if requested */
  descr->open_with_carla = with_carla;

  UndoableAction * ua = NULL;

  /* create an extra track */
  ua =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO_BUS, NULL, NULL,
      TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * target_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  if (is_instrument)
    {
      /* create an instrument track from helm */
      ua =
        create_tracks_action_new (
          TRACK_TYPE_INSTRUMENT, descr, NULL,
          TRACKLIST->num_tracks, NULL, 1);
      undo_manager_perform (UNDO_MANAGER, ua);
    }
  else
    {
      /* create an audio fx track and add the plugin */
      ua =
        create_tracks_action_new (
          TRACK_TYPE_AUDIO_BUS, NULL, NULL,
          TRACKLIST->num_tracks, NULL, 1);
      undo_manager_perform (UNDO_MANAGER, ua);
      ua =
        create_plugins_action_new (
          descr, PLUGIN_SLOT_INSERT,
          TRACKLIST->num_tracks - 1, 0, 1);
      undo_manager_perform (UNDO_MANAGER, ua);
    }

  plugin_descriptor_free (descr);

  Track * src_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  /* connect a plugin CV out to the track's
   * balance */
  Port * src_port1 = NULL;
  Port * src_port2 = NULL;
  int max_size = 20;
  Port ** ports =
    calloc ((size_t) max_size, sizeof (Port *));
  int num_ports = 0;
  track_append_all_ports (
    src_track, &ports, &num_ports, true, &max_size,
    true);
  for (int i = 0; i < num_ports; i++)
    {
      Port * port = ports[i];
      if (port->id.owner_type ==
            PORT_OWNER_TYPE_PLUGIN &&
          port->id.type == TYPE_CV &&
          port->id.flow == FLOW_OUTPUT)
        {
          if (src_port1)
            {
              src_port2 = port;
              break;
            }
          else
            {
              src_port1 = port;
              continue;
            }
        }
    }
  free (ports);

  Port * dest_port = NULL;
  max_size = 20;
  ports =
    calloc ((size_t) max_size, sizeof (Port *));
  num_ports = 0;
  track_append_all_ports (
    target_track, &ports, &num_ports, true, &max_size,
    true);
  for (int i = 0; i < num_ports; i++)
    {
      Port * port = ports[i];
      if (port->id.owner_type ==
            PORT_OWNER_TYPE_FADER &&
          port->id.flags & PORT_FLAG_STEREO_BALANCE)
        {
          dest_port = port;
          break;
        }
    }
  free (ports);

  g_assert_nonnull (src_port1);
  g_assert_nonnull (src_port2);
  g_assert_nonnull (dest_port);
  g_assert_true (src_port1->is_project);
  g_assert_true (src_port2->is_project);
  g_assert_true (dest_port->is_project);

  ua =
    port_connection_action_new (
      PORT_CONNECTION_CONNECT, &src_port1->id,
      &dest_port->id);
  undo_manager_perform (UNDO_MANAGER, ua);

  g_assert_cmpint (dest_port->num_srcs, ==, 1);
  g_assert_cmpint (src_port1->num_dests, ==, 1);

  undo_manager_undo (UNDO_MANAGER);

  g_assert_cmpint (dest_port->num_srcs, ==, 0);
  g_assert_cmpint (src_port1->num_dests, ==, 0);

  undo_manager_redo (UNDO_MANAGER);

  g_assert_cmpint (dest_port->num_srcs, ==, 1);
  g_assert_cmpint (src_port1->num_dests, ==, 1);

  ua =
    port_connection_action_new (
      PORT_CONNECTION_CONNECT, &src_port2->id,
      &dest_port->id);
  undo_manager_perform (UNDO_MANAGER, ua);

  g_assert_cmpint (dest_port->num_srcs, ==, 2);
  g_assert_cmpint (src_port1->num_dests, ==, 1);
  g_assert_cmpint (src_port2->num_dests, ==, 1);
  g_assert_true (
    port_identifier_is_equal (
      &src_port1->dest_ids[0], &dest_port->id));
  g_assert_true (
    port_identifier_is_equal (
      &dest_port->src_ids[0], &src_port1->id));
  g_assert_true (
    port_identifier_is_equal (
      &src_port2->dest_ids[0], &dest_port->id));
  g_assert_true (
    port_identifier_is_equal (
      &dest_port->src_ids[1], &src_port2->id));
  g_assert_true (
    dest_port->srcs[0] == src_port1);
  g_assert_true (
    dest_port == src_port1->dests[0]);
  g_assert_true (
    dest_port->srcs[1] == src_port2);
  g_assert_true (
    dest_port == src_port2->dests[0]);

  undo_manager_undo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);

  /* let the engine run */
  g_usleep (1000000);
}

static void
test_port_connection (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_AMS_LFO
  _test_port_connection (
    AMS_LFO_BUNDLE, AMS_LFO_URI, true, false);
#endif

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/port_connection/"

  g_test_add_func (
    TEST_PREFIX "test_port_connection",
    (GTestFunc) test_port_connection);

  return g_test_run ();
}

