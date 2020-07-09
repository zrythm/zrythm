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

#include "actions/copy_tracks_action.h"
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
_test_port_and_plugin_track_pos_after_duplication (
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

  /* create an instrument track from helm */
  UndoableAction * ua =
    create_tracks_action_new (
      is_instrument ?
        TRACK_TYPE_INSTRUMENT :
        TRACK_TYPE_AUDIO_BUS,
      descr, NULL,
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

  /* move automation in 2nd track and undo/redo */
  atl = track_get_automation_tracklist (dest_track);
  ap = atl->ats[atl->num_ats - 1]->regions[0]->aps[0];
  arranger_object_select (
    (ArrangerObject *) ap, true, false);
  float prev_norm_val = ap->normalized_val;
  automation_point_set_fvalue (
    ap, prev_norm_val - 0.1, true);
  ua =
    arranger_selections_action_new_move_automation (
      (ArrangerSelections *) AUTOMATION_SELECTIONS,
      0, 0.1, F_ALREADY_MOVED);
  undo_manager_perform (UNDO_MANAGER, ua);
  undo_manager_undo (UNDO_MANAGER);
  undo_manager_redo (UNDO_MANAGER);

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
test_port_and_plugin_track_pos_after_duplication (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_HELM
  _test_port_and_plugin_track_pos_after_duplication (
    HELM_BUNDLE, HELM_URI, true, false);
#endif
#ifdef HAVE_LSP_COMPRESSOR
  _test_port_and_plugin_track_pos_after_duplication (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI,
    false, false);
#endif

  test_helper_zrythm_cleanup ();
}

static void
test_port_and_plugin_track_pos_after_duplication_with_carla (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_HELM
  _test_port_and_plugin_track_pos_after_duplication (
    HELM_BUNDLE, HELM_URI, true, true);
#endif
#ifdef HAVE_LSP_COMPRESSOR
  _test_port_and_plugin_track_pos_after_duplication (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI,
    false, true);
#endif

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/copy_tracks/"

  g_test_add_func (
    TEST_PREFIX
    "test port and plugin track pos after duplication",
    (GTestFunc)
    test_port_and_plugin_track_pos_after_duplication);
#ifdef HAVE_CARLA
  g_test_add_func (
    TEST_PREFIX
    "test port and plugin track pos after duplication with carla",
    (GTestFunc)
    test_port_and_plugin_track_pos_after_duplication_with_carla);
#endif

  return g_test_run ();
}
