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

#include "actions/delete_tracks_action.h"
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

/**
 * Check that all ports in the track have the track
 * pos.
 */
static void
check_port_identifiers_in_track (
  Track * track,
  int     track_pos)
{
  int max_size = 20;
  int num_ports = 0;
  Port ** ports = calloc (max_size, sizeof (Port *));
  track_append_all_ports (
    track, &ports, &num_ports, true, &max_size,
    true);
  for (int i = 0; i < num_ports; i++)
    {
      Port * port = ports[i];
      g_assert_cmpint (
        port->id.track_pos, ==, track_pos);
      if (port->id.owner_type ==
            PORT_OWNER_TYPE_PLUGIN)
        {
          PluginIdentifier * pid = &port->id.plugin_id;
          g_assert_cmpint (
            pid->track_pos, ==, track_pos);
          Plugin * pl = plugin_find (pid);
          g_assert_true (
            plugin_identifier_is_equal (&pl->id, pid));
          g_assert_true (
            pl == track->channel->instrument);
        }
    }
  free (ports);
}

/**
 * Check the identifiers in each automation track.
 */
static void
check_port_identifiers_in_ats (
  Track * track,
  int     track_pos)
{
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  for (int i = 0; i < atl->num_ats; i++)
    {
      AutomationTrack * at = atl->ats[i];
      g_assert_cmpint (
        at->port_id.track_pos, ==, track_pos);
      if (at->port_id.owner_type ==
            PORT_OWNER_TYPE_PLUGIN)
        {
          g_assert_cmpint (
            at->port_id.plugin_id.track_pos, ==,
            track_pos);
        }
      g_assert_true (
        automation_track_find_from_port_id (
          &at->port_id) == at);
    }
}

static void
test_undo_track_deletion (void)
{
  LilvNode * path =
    lilv_new_uri (LILV_WORLD, HELM_BUNDLE);
  lilv_world_load_bundle (
    LILV_WORLD, path);
  lilv_node_free (path);

  plugin_manager_scan_plugins (
    PLUGIN_MANAGER, 1.0, NULL);
  g_assert_cmpint (
    PLUGIN_MANAGER->num_plugins, ==, 1);

  /* fix the descriptor (for some reason lilv
   * reports it as Plugin instead of Instrument if
   * you don't do lilv_world_load_all) */
  PluginDescriptor * descr =
    plugin_descriptor_clone (
      PLUGIN_MANAGER->plugin_descriptors[0]);
  /*g_warn_if_fail (*/
    /*descr->category == PC_INSTRUMENT);*/
  descr->category = PC_INSTRUMENT;
  g_free (descr->category_str);
  descr->category_str =
    plugin_descriptor_category_to_string (
      descr->category);

  /* create an instrument track from helm */
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_INSTRUMENT, descr, NULL,
      TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  plugin_descriptor_free (descr);

  /* select it */
  Track * helm_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    helm_track, F_SELECT, true, F_NO_PUBLISH_EVENTS);

  /* get an automation track */
  AutomationTracklist * atl =
    track_get_automation_tracklist (helm_track);
  AutomationTrack * at = atl->ats[40];
  at->created = true;
  at->visible = true;

  /* create an automation region */
  Position start_pos, end_pos;
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 4);
  ZRegion * region =
    automation_region_new (
      &start_pos, &end_pos, helm_track->pos,
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

  /* delete it */
  ua =
    delete_tracks_action_new (TRACKLIST_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* undo deletion */
  undo_manager_undo (UNDO_MANAGER);

  /* let the engine run */
  g_usleep (1000000);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/actions/delete_tracks/"

  g_test_add_func (
    TEST_PREFIX "test undo track deletion",
    (GTestFunc) test_undo_track_deletion);

  return g_test_run ();
}

