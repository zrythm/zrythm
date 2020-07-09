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

#include "actions/arranger_selections.h"
#include "actions/create_tracks_action.h"
#include "actions/copy_plugins_action.h"
#include "actions/delete_plugins_action.h"
#include "actions/move_plugins_action.h"
#include "actions/undoable_action.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/master_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "audio/tempo_track.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/zrythm.h"

#include <glib.h>

#define TRACK_POS 4
#define NEW_TRACK_POS 5

static Position start_pos, end_pos, ap1_pos, ap2_pos;

typedef enum TestPluginType
{
  PLUGIN_TYPE_AMP,
  PLUGIN_TYPE_FIFTHS
} TestPluginType;

static TestPluginType pl_type;

static bool bundle_loaded = false;

/**
 * Bootstraps the test with test data.
 */
static void
rebootstrap (
  TestPluginType type)
{
  pl_type = type;
  char plugin_dir[700];
  switch (type)
    {
    case PLUGIN_TYPE_AMP:
      sprintf (plugin_dir, "eg-amp.lv2");
      break;
    case PLUGIN_TYPE_FIFTHS:
      sprintf (plugin_dir, "eg-fifths.lv2");
      break;
    }

  char path_str[6000];
  sprintf (
    path_str, "file://%s/%s/",
    TESTS_BUILDDIR, plugin_dir);
  g_message ("path is %s", path_str);

  if (!bundle_loaded)
    {
      LilvNode * path =
        lilv_new_uri (LILV_WORLD, path_str);
      lilv_world_load_bundle (
        LILV_WORLD, path);
      lilv_node_free (path);
      bundle_loaded = true;
      plugin_manager_scan_plugins (
        PLUGIN_MANAGER, 1.0, NULL);
      g_assert_cmpint (
        PLUGIN_MANAGER->num_plugins, ==, 1);
    }

  /* remove any previous work */
  chord_track_clear (P_CHORD_TRACK);
  marker_track_clear (P_MARKER_TRACK);
  tempo_track_clear (P_MARKER_TRACK);
  for (int i = TRACKLIST->num_tracks - 1;
       i >= 4; i--)
    {
      Track * track = TRACKLIST->tracks[i];
      tracklist_remove_track (
        TRACKLIST, track, 1, 1, 0, 0);
    }
  track_clear (P_MASTER_TRACK);
}

static void
check_initial_state ()
{
  g_assert_cmpint (TRACKLIST->num_tracks, ==, 4);
}

static void
check_after_step1 ()
{
  g_assert_cmpint (TRACKLIST->num_tracks, ==, 5);

  /* check identifiers */
  Track * track = TRACKLIST->tracks[TRACK_POS];
  Channel * ch = track->channel;
  g_assert_nonnull (ch);
  Plugin * pl = ch->inserts[0];
  g_assert_nonnull (pl);
  g_assert_cmpint (pl->id.track_pos, ==, track->pos);
  g_assert_cmpint (pl->id.slot, ==, 0);
  g_assert_true (
    ch == plugin_get_channel (pl));

  /* check that automation tracks are created */
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  g_assert_nonnull (atl);
  switch (pl_type)
    {
    case PLUGIN_TYPE_AMP:
      g_assert_cmpint (atl->num_ats, ==, 5);
      break;
    case PLUGIN_TYPE_FIFTHS:
      g_assert_cmpint (atl->num_ats, ==, 5);
      break;
    }
  AutomationTrack * at = atl->ats[3];
  g_assert_nonnull (at);
  g_assert_cmpint (
    at->port_id.owner_type, ==,
    PORT_OWNER_TYPE_PLUGIN);
  g_assert_true (
    at->port_id.flags & PORT_FLAG_PLUGIN_ENABLED);
  at = atl->ats[4];
  g_assert_nonnull (at);
  g_assert_cmpint (
    at->port_id.owner_type, ==,
    PORT_OWNER_TYPE_PLUGIN);
  g_assert_true (
    at->port_id.flags & PORT_FLAG_PLUGIN_CONTROL);
}

static void
check_after_step2 ()
{
  Track * track =
    TRACKLIST->tracks[TRACK_POS];
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  AutomationTrack * at = atl->ats[4];

  g_assert_cmpint (at->num_regions, ==, 1);
  ZRegion * region = at->regions[0];
  g_assert_cmpint (
    region->id.track_pos, ==, track->pos);
  g_assert_cmpint (
    region->id.at_idx, ==, at->index);
  g_assert_cmpint (
    region->id.idx, ==, 0);
}

static void
check_after_step3 ()
{
  Track * track =
    TRACKLIST->tracks[TRACK_POS];
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  AutomationTrack * at = atl->ats[4];
  ZRegion * region = at->regions[0];

  g_assert_cmpint (region->num_aps, ==, 1);
  g_assert_cmpint (region->aps[0]->index, ==, 0);
  g_assert_true (
    region_identifier_is_equal (
      &region->id,
      &((ArrangerObject *)region->aps[0])->
        region_id));
}

static void
check_after_step4 ()
{
  Track * track =
    TRACKLIST->tracks[TRACK_POS];
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  AutomationTrack * at = atl->ats[4];
  ZRegion * region = at->regions[0];

  g_assert_cmpint (region->num_aps, ==, 2);
  g_assert_cmpint (region->aps[1]->index, ==, 1);
  g_assert_true (
    region_identifier_is_equal (
      &region->id,
      &((ArrangerObject *)region->aps[1])->
        region_id));
}

static void
check_after_step5 ()
{
  Track * track =
    TRACKLIST->tracks[TRACK_POS];
  Track * new_track =
    TRACKLIST->tracks[NEW_TRACK_POS];
  Channel * ch = track->channel;
  Channel * new_ch = new_track->channel;

  /* check that new track is created and the
   * identifiers are correct */
  g_assert_cmpint (TRACKLIST->num_tracks, ==, 6);
  g_assert_nonnull (new_track);
  g_assert_true (
    new_track->type == TRACK_TYPE_INSTRUMENT ||
    new_track->type == TRACK_TYPE_AUDIO_BUS);
  g_assert_nonnull (new_ch);
  Plugin * new_pl = new_ch->inserts[1];
  g_assert_nonnull (new_pl);
  g_assert_null (new_ch->inserts[0]);
  Plugin * pl = ch->inserts[0];
  g_assert_cmpint (pl->id.track_pos, ==, TRACK_POS);
  g_assert_cmpint (pl->id.slot, ==, 0);
  g_assert_cmpint (
    new_pl->id.track_pos, ==, NEW_TRACK_POS);
  g_assert_cmpint (new_pl->id.slot, ==, 1);
  g_assert_cmpint (
    MIXER_SELECTIONS->num_slots, ==, 1);
  g_assert_cmpint (
    MIXER_SELECTIONS->track_pos, ==, NEW_TRACK_POS);
  g_assert_cmpint (
    MIXER_SELECTIONS->slots[0], ==, 1);

  /* check that the automation was copied
   * correctly */
  AutomationTracklist * atl =
    track_get_automation_tracklist (new_track);
  g_assert_cmpint (atl->num_ats, ==, 5);
  AutomationTrack * at = atl->ats[4];
  g_assert_nonnull (at);
  g_assert_cmpint (at->index, ==, 4);
  g_assert_cmpint (at->num_regions, ==, 1);
  ZRegion * region = at->regions[0];
  g_assert_cmpint (region->num_aps, ==, 2);
  g_assert_cmpint (
    region->id.track_pos, ==, new_track->pos);
  g_assert_cmpint (region->id.at_idx, ==, 4);
  g_assert_cmpint (region->id.idx, ==, 0);
}

static void
check_after_step6 ()
{
  Track * new_track =
    TRACKLIST->tracks[NEW_TRACK_POS];
  Channel * new_ch = new_track->channel;

  /* check that there are 2 plugins in the track
   * now (slot 1 and 2) */
  g_assert_cmpint (
    MIXER_SELECTIONS->track_pos, ==, NEW_TRACK_POS);
  g_assert_cmpint (
    MIXER_SELECTIONS->slots[0], ==, 2);
  g_assert_nonnull (new_ch->inserts[2]);
  Plugin * slot2_plugin = new_ch->inserts[2];
  g_assert_cmpint (
    slot2_plugin->id.track_pos, ==, NEW_TRACK_POS);
  g_assert_cmpint (slot2_plugin->id.slot, ==, 2);

  /* check that the automation was copied too */
  AutomationTracklist * atl =
    track_get_automation_tracklist (new_track);
  g_assert_cmpint (atl->num_ats, ==, 7);
  AutomationTrack * at = atl->ats[6];
  g_assert_nonnull (at);
  g_assert_cmpint (at->index, ==, 6);
  g_assert_cmpint (at->num_regions, ==, 1);
  ZRegion * region = at->regions[0];
  g_assert_cmpint (region->num_aps, ==, 2);
  g_assert_cmpint (
    region->id.track_pos, ==, new_track->pos);
  g_assert_cmpint (region->id.at_idx, ==, 6);
  g_assert_cmpint (region->id.idx, ==, 0);
}

static void
check_after_step7 ()
{
  Track * track =
    TRACKLIST->tracks[TRACK_POS];
  Track * new_track =
    TRACKLIST->tracks[NEW_TRACK_POS];
  Channel * ch =
    TRACKLIST->tracks[TRACK_POS]->channel;
  Channel * new_ch = new_track->channel;

  /* verify that they were copied there */
  g_assert_cmpint (
    MIXER_SELECTIONS->num_slots, ==, 2);
  g_assert_true (MIXER_SELECTIONS->has_any);
  g_assert_cmpint (
    MIXER_SELECTIONS->track_pos, ==, track->pos);
  g_assert_nonnull (ch->inserts[5]);
  g_assert_nonnull (ch->inserts[6]);
  g_assert_nonnull (new_ch->inserts[1]);
  g_assert_nonnull (new_ch->inserts[2]);
  g_assert_null (new_ch->inserts[5]);
  g_assert_null (new_ch->inserts[6]);

  /* check their identifiers */
  g_assert_cmpint (
    ch->inserts[5]->id.track_pos, ==, track->pos);
  g_assert_cmpint (
    ch->inserts[5]->id.slot, ==, 5);
  g_assert_cmpint (
    ch->inserts[6]->id.track_pos, ==, track->pos);
  g_assert_cmpint (
    ch->inserts[6]->id.slot, ==, 6);

  /* check that the automations were copied too */
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  g_assert_cmpint (atl->num_ats, ==, 9);
  AutomationTrack * at = NULL;
  switch (pl_type)
    {
    case PLUGIN_TYPE_AMP:
      at =
        automation_tracklist_get_plugin_at (
          atl, PLUGIN_SLOT_INSERT, 5, "Gain");
      break;
    case PLUGIN_TYPE_FIFTHS:
      at =
        automation_tracklist_get_plugin_at (
          atl, PLUGIN_SLOT_INSERT, 5, "Test param");
      break;
    }
  g_assert_nonnull (at);
  g_assert_cmpint (at->num_regions, ==, 1);
  ZRegion * region = at->regions[0];
  g_assert_cmpint (region->num_aps, ==, 2);
  g_assert_cmpint (
    region->id.track_pos, ==, track->pos);
  g_assert_cmpint (region->id.at_idx, ==, at->index);
  g_assert_cmpint (region->id.idx, ==, 0);
}

static void
check_after_step8 ()
{
  Track * new_track =
    TRACKLIST->tracks[NEW_TRACK_POS];
  Channel * ch =
    TRACKLIST->tracks[TRACK_POS]->channel;
  Channel * new_ch = new_track->channel;

  /* verify that they were moved there */
  g_assert_cmpint (
    MIXER_SELECTIONS->num_slots, ==, 2);
  g_assert_true (MIXER_SELECTIONS->has_any);
  g_assert_cmpint (
    MIXER_SELECTIONS->track_pos, ==, new_track->pos);
  g_assert_null (ch->inserts[5]);
  g_assert_null (ch->inserts[6]);
  g_assert_nonnull (new_ch->inserts[7]);
  g_assert_nonnull (new_ch->inserts[8]);

  /* verify that the automation was moved too */
  AutomationTracklist * atl =
    track_get_automation_tracklist (new_track);
  g_assert_cmpint (atl->num_ats, ==, 11);
  AutomationTrack * at = NULL;
  switch (pl_type)
    {
    case PLUGIN_TYPE_AMP:
      at =
        automation_tracklist_get_plugin_at (
          atl, PLUGIN_SLOT_INSERT, 7, "Gain");
      break;
    case PLUGIN_TYPE_FIFTHS:
      at =
        automation_tracklist_get_plugin_at (
          atl, PLUGIN_SLOT_INSERT, 7, "Test param");
      break;
    }
  g_assert_nonnull (at);
  g_assert_cmpint (at->num_regions, ==, 1);
  ZRegion * region = at->regions[0];
  g_assert_cmpint (region->num_aps, ==, 2);
  g_assert_cmpint (
    region->id.track_pos, ==, new_track->pos);
  g_assert_cmpint (region->id.at_idx, ==, at->index);
  g_assert_cmpint (region->id.idx, ==, 0);
  AutomationPoint * ap = region->aps[0];
  g_assert_true (
    region_identifier_is_equal (
      &region->id,
      &((ArrangerObject *) ap)->region_id));
}

static void
check_after_step9 ()
{
  AutomationTracklist * atl =
    track_get_automation_tracklist (
      TRACKLIST->tracks[NEW_TRACK_POS]);

  /* verify that they were deleted along with their
   * automation */
  g_assert_cmpint (
    MIXER_SELECTIONS->num_slots, ==, 0);
  g_assert_true (!MIXER_SELECTIONS->has_any);
  g_assert_cmpint (atl->num_ats, ==, 7);
  g_test_expect_message (
    NULL, G_LOG_LEVEL_CRITICAL,
    "* should not be reached*");
  AutomationTrack * at = NULL;
  switch (pl_type)
    {
    case PLUGIN_TYPE_AMP:
      at =
        automation_tracklist_get_plugin_at (
          atl, PLUGIN_SLOT_INSERT, 7, "Gain");
      break;
    case PLUGIN_TYPE_FIFTHS:
      at =
        automation_tracklist_get_plugin_at (
          atl, PLUGIN_SLOT_INSERT, 7, "Test param");
      break;
    }
  (void) at;
  g_test_assert_expected_messages ();
}

static void
test_move_plugins ()
{
  check_initial_state ();

  /* create a track with a plugin */
  UndoableAction * action =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO_BUS,
      PLUGIN_MANAGER->plugin_descriptors[0], NULL,
      TRACK_POS, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, action);

  /* check if track is selected */
  Track * track = TRACKLIST->tracks[TRACK_POS];
  g_assert_cmpint (track->pos, ==, TRACK_POS);
  g_assert_nonnull (track);
  g_assert_true (
    tracklist_selections_contains_track (
      TRACKLIST_SELECTIONS, track));

  check_after_step1 ();

  /* add some automation */
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  AutomationTrack * at = atl->ats[4];
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 4);
  ZRegion * region =
    automation_region_new (
      &start_pos, &end_pos, track->pos, at->index,
      0);
  track_add_region (
    track, region, at, -1, 1, 0);
  arranger_object_select (
    (ArrangerObject *) region, 1, 0);
  action =
    arranger_selections_action_new_create (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, action);

  check_after_step2 ();

  position_set_to_bar (&ap1_pos, 1);
  position_set_to_bar (&ap2_pos, 2);
  AutomationPoint * ap =
    automation_point_new_float (
      -50.f, 0, &ap1_pos);
  automation_region_add_ap (
    region, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, 1, 0);
  action =
    arranger_selections_action_new_create (
      AUTOMATION_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, action);

  check_after_step3 ();

  ap =
    automation_point_new_float (
      -30.f, 0, &ap2_pos);
  automation_region_add_ap (
    region, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, 1, 0);
  action =
    arranger_selections_action_new_create (
      AUTOMATION_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, action);

  /* select the plugin */
  Channel * ch = track->channel;
  Plugin * pl = ch->inserts[0];
  mixer_selections_add_slot (
    MIXER_SELECTIONS, ch, PLUGIN_SLOT_INSERT,
    pl->id.slot);
  g_assert_cmpint (
    MIXER_SELECTIONS->num_slots, ==, 1);

  check_after_step4 ();

  /* duplicate to next slot in a new track */
  action =
    copy_plugins_action_new (
      MIXER_SELECTIONS, PLUGIN_SLOT_INSERT,
      NULL, 1);
  undo_manager_perform (UNDO_MANAGER, action);

  check_after_step5 ();

  /* copy the new plugin to the slot below */
  Track * new_track =
    TRACKLIST->tracks[NEW_TRACK_POS];
  Channel * new_ch = new_track->channel;
  action =
    copy_plugins_action_new (
      MIXER_SELECTIONS, PLUGIN_SLOT_INSERT,
      new_track, 2);
  undo_manager_perform (UNDO_MANAGER, action);

  /* select the plugin above too */
  g_assert_cmpint (
    MIXER_SELECTIONS->num_slots, ==, 1);
  g_assert_true (MIXER_SELECTIONS->has_any);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, new_ch, PLUGIN_SLOT_INSERT,
    1);
  g_assert_cmpint (
    MIXER_SELECTIONS->num_slots, ==, 2);

  check_after_step6 ();

  /* copy both to the previous track at slot 5 */
  action =
    copy_plugins_action_new (
      MIXER_SELECTIONS, PLUGIN_SLOT_INSERT,
      track, 5);
  undo_manager_perform (UNDO_MANAGER, action);

  check_after_step7 ();

  /* move the plugins to slot 7 and 8 in the next
   * track */
  action =
    move_plugins_action_new (
      MIXER_SELECTIONS, PLUGIN_SLOT_INSERT,
      new_track, 7);
  undo_manager_perform (UNDO_MANAGER, action);

  check_after_step8 ();

  /* delete these 2 plugins */
  action =
    delete_plugins_action_new (MIXER_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, action);

  check_after_step9 ();

  /* -------- start undoing everything -------- */

  /* undo deleting the plugins */
  undo_manager_undo (UNDO_MANAGER);

  /* verify that they exist along with their
   * automation */
  check_after_step8 ();

  /* undo moving plugins from track 3 to track 4 */
  undo_manager_undo (UNDO_MANAGER);
  check_after_step7 ();

  /* undo copy both plugins to the previous track
   * at slot 5 */
  undo_manager_undo (UNDO_MANAGER);
  check_after_step6 ();

  /* undo copying the plugin to the slot below it */
  undo_manager_undo (UNDO_MANAGER);
  check_after_step5 ();

  /* undo duplicating to next slot in a new track */
  undo_manager_undo (UNDO_MANAGER);
  check_after_step4 ();

  /* undo adding automation points */
  undo_manager_undo (UNDO_MANAGER);
  check_after_step3 ();
  undo_manager_undo (UNDO_MANAGER);
  check_after_step2 ();

  /* undo adding automation region */
  undo_manager_undo (UNDO_MANAGER);
  check_after_step1 ();

  /* undo adding a track with a plugin */
  undo_manager_undo (UNDO_MANAGER);
  check_initial_state ();

  /* -------- redo everything -------- */

  undo_manager_redo (UNDO_MANAGER);
  check_after_step1 ();
  undo_manager_redo (UNDO_MANAGER);
  check_after_step2 ();
  undo_manager_redo (UNDO_MANAGER);
  check_after_step3 ();
  undo_manager_redo (UNDO_MANAGER);
  check_after_step4 ();
  undo_manager_redo (UNDO_MANAGER);
  check_after_step5 ();
  undo_manager_redo (UNDO_MANAGER);
  check_after_step6 ();
  undo_manager_redo (UNDO_MANAGER);
  check_after_step7 ();
  undo_manager_redo (UNDO_MANAGER);
  check_after_step8 ();
  undo_manager_redo (UNDO_MANAGER);
  check_after_step9 ();
}

static void
test_move_plugins_amp ()
{
  rebootstrap (PLUGIN_TYPE_AMP);
  test_move_plugins ();
}

static void
test_move_plugins_fifths ()
{
  rebootstrap (PLUGIN_TYPE_FIFTHS);
  test_move_plugins ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/actions/mixer_selections/"

  g_test_add_func (
    TEST_PREFIX "test move plugins amp",
    (GTestFunc) test_move_plugins_amp);
  g_test_add_func (
    TEST_PREFIX "test move plugins fifths",
    (GTestFunc) test_move_plugins_fifths);

  return g_test_run ();
}
