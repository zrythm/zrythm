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

#include "actions/move_plugins_action.h"
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
_test_port_and_plugin_track_pos_after_move (
  const char * pl_bundle,
  const char * pl_uri,
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

  /* open with carla if requested */
  descr->open_with_carla = with_carla;

  /* create an instrument track from helm */
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO_BUS, descr, NULL,
      TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  plugin_descriptor_free (descr);

  int src_track_pos = TRACKLIST->num_tracks - 1;
  int dest_track_pos = TRACKLIST->num_tracks;

  /* select it */
  Track * src_track =
    TRACKLIST->tracks[src_track_pos];
  track_select (
    src_track, F_SELECT, true, F_NO_PUBLISH_EVENTS);

  /* get an automation track */
  AutomationTracklist * atl =
    track_get_automation_tracklist (src_track);
  AutomationTrack * at = atl->ats[atl->num_ats - 1];
  at->created = true;
  at->visible = true;

  /* create an automation region */
  Position start_pos, end_pos;
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 4);
  ZRegion * region =
    automation_region_new (
      &start_pos, &end_pos, src_track->pos,
      at->index, at->num_regions);
  automation_track_add_region (at, region);
  arranger_object_select (
    (ArrangerObject *) region, true, false);
  ua =
    arranger_selections_action_new_create (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* create some automation points */
  Port * port = automation_track_get_port (at);
  position_set_to_bar (&start_pos, 1);
  AutomationPoint * ap =
    automation_point_new_float (
      port->deff,
      control_port_real_val_to_normalized (
        port, port->deff),
      &start_pos);
  automation_region_add_ap (
    region, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, true, false);
  ua =
    arranger_selections_action_new_create (
      AUTOMATION_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* duplicate it */
  ua =
    copy_tracks_action_new (
      TRACKLIST_SELECTIONS, TRACKLIST->num_tracks);

  g_assert_true (
    track_verify_identifiers (src_track));

  undo_manager_perform (UNDO_MANAGER, ua);

  Track * dest_track =
    TRACKLIST->tracks[dest_track_pos];

  g_assert_true (
    track_verify_identifiers (src_track));
  g_assert_true (
    track_verify_identifiers (dest_track));

  /* move plugin from 1st track to 2nd track and
   * undo/redo */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, src_track->channel,
    PLUGIN_SLOT_INSERT, 0);
  ua =
    move_plugins_action_new (
      MIXER_SELECTIONS, PLUGIN_SLOT_INSERT,
      dest_track, 1);

  undo_manager_perform (UNDO_MANAGER, ua);

  g_assert_true (
    track_verify_identifiers (src_track));
  g_assert_true (
    track_verify_identifiers (dest_track));

  undo_manager_undo (UNDO_MANAGER);

  g_assert_true (
    track_verify_identifiers (src_track));
  g_assert_true (
    track_verify_identifiers (dest_track));

  undo_manager_redo (UNDO_MANAGER);

  g_assert_true (
    track_verify_identifiers (src_track));
  g_assert_true (
    track_verify_identifiers (dest_track));

  undo_manager_undo (UNDO_MANAGER);

  /* move plugin from 1st slot to the 2nd slot and
   * undo/redo */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, src_track->channel,
    PLUGIN_SLOT_INSERT, 0);
  ua =
    move_plugins_action_new (
      MIXER_SELECTIONS, PLUGIN_SLOT_INSERT,
      src_track, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* let the engine run */
  g_usleep (1000000);

  /* go back to the start */
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_undo (UNDO_MANAGER);
}

static void
test_port_and_plugin_track_pos_after_move (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_LSP_COMPRESSOR
  _test_port_and_plugin_track_pos_after_move (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI,
    false);
#endif

  test_helper_zrythm_cleanup ();
}

static void
test_port_and_plugin_track_pos_after_move_with_carla (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_LSP_COMPRESSOR
  _test_port_and_plugin_track_pos_after_move (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI,
    true);
#endif

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/move_plugins/"

  g_test_add_func (
    TEST_PREFIX
    "test port and plugin track pos after move",
    (GTestFunc)
    test_port_and_plugin_track_pos_after_move);
#ifdef HAVE_CARLA
  g_test_add_func (
    TEST_PREFIX
    "test port and plugin track pos after move with carla",
    (GTestFunc)
    test_port_and_plugin_track_pos_after_move_with_carla);
#endif

  return g_test_run ();
}
