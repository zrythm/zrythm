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

#include "actions/create_plugins_action.h"
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
_test_create_plugins (
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
  /*if (is_instrument)*/
    /*{*/
      /*descr->category = PC_INSTRUMENT;*/
    /*}*/
  /*g_free (descr->category_str);*/
  /*descr->category_str =*/
    /*plugin_descriptor_category_to_string (*/
      /*descr->category);*/

  /* open with carla if requested */
  descr->open_with_carla = with_carla;

  UndoableAction * ua = NULL;
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

  int src_track_pos = TRACKLIST->num_tracks - 1;
  Track * src_track =
    TRACKLIST->tracks[src_track_pos];

  if (is_instrument)
    {
      g_assert_true (src_track->channel->instrument);
    }
  else
    {
      g_assert_true (src_track->channel->inserts[0]);
    }

  /* duplicate the track */
  track_select (
    src_track, F_SELECT, true, F_NO_PUBLISH_EVENTS);
  ua =
    copy_tracks_action_new (
      TRACKLIST_SELECTIONS, TRACKLIST->num_tracks);
  g_assert_true (
    track_verify_identifiers (src_track));
  undo_manager_perform (UNDO_MANAGER, ua);

  int dest_track_pos = TRACKLIST->num_tracks - 1;
  Track * dest_track =
    TRACKLIST->tracks[dest_track_pos];

  g_assert_true (
    track_verify_identifiers (src_track));
  g_assert_true (
    track_verify_identifiers (dest_track));

  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);

  g_message ("letting engine run...");

  /* let the engine run */
  g_usleep (1000000);

  g_message ("done");
}

static void
test_create_plugins (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_HELM
  _test_create_plugins (
    HELM_BUNDLE, HELM_URI, true, false);
#endif
#ifdef HAVE_LSP_COMPRESSOR
  _test_create_plugins (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI,
    false, false);
#endif
#ifdef HAVE_SHERLOCK_ATOM_INSPECTOR
  _test_create_plugins (
    SHERLOCK_ATOM_INSPECTOR_BUNDLE,
    SHERLOCK_ATOM_INSPECTOR_URI,
    false, false);
#endif
#ifdef HAVE_CARLA_RACK
  _test_create_plugins (
    CARLA_RACK_BUNDLE, CARLA_RACK_URI,
    true, false);
#endif
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/actions/create_plugins/"

  g_test_add_func (
    TEST_PREFIX
    "test_create_plugins",
    (GTestFunc) test_create_plugins);

  test_helper_zrythm_cleanup ();

  return g_test_run ();
}
