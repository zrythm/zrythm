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

#include "actions/arranger_selections.h"
#include "actions/undoable_action.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/master_track.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include "tests/helpers/project.h"

#include <glib.h>

/**
 * Bootstraps the test with test data.
 */
static void
rebootstrap (void)
{
  char path_str[6000];
  sprintf (
    path_str, "file://%s/%s/",
    TESTS_BUILDDIR, "test-instrument.lv2");
  g_message ("path is %s", path_str);

  plugin_manager_free (PLUGIN_MANAGER);
  ZRYTHM->plugin_manager = plugin_manager_new ();
  LilvNode * path =
    lilv_new_uri (LILV_WORLD, path_str);
  lilv_world_load_bundle (
    LILV_WORLD, path);
  lilv_node_free (path);

  plugin_manager_scan_plugins (
    PLUGIN_MANAGER, 1.0, NULL);
  g_assert_cmpint (
    PLUGIN_MANAGER->num_plugins, ==, 1);

  /* for some reason it gets labeled as a plugin,
   * force instrument */
  PLUGIN_MANAGER->plugin_descriptors[0]->category =
    PC_INSTRUMENT;
  PLUGIN_MANAGER->plugin_descriptors[0]->category_str =
    g_strdup ("Instrument");
}

static void
test_move_tracks ()
{
  rebootstrap ();

  /* create a track with an instrument */
  UndoableAction * action =
    create_tracks_action_new (
      TRACK_TYPE_INSTRUMENT,
      PLUGIN_MANAGER->plugin_descriptors[0], NULL,
      3, 1);
  undo_manager_perform (UNDO_MANAGER, action);
  Track * ins_track = TRACKLIST->tracks[3];
  g_assert_true (
    ins_track->type == TRACK_TYPE_INSTRUMENT);

  /* create an fx track and send to it */
  action =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO_BUS, NULL, NULL,
      4, 1);
  undo_manager_perform (UNDO_MANAGER, action);
  Track * fx_track = TRACKLIST->tracks[4];
  g_assert_true (
    fx_track->type == TRACK_TYPE_AUDIO_BUS);

  ChannelSend * send = &ins_track->channel->sends[0];
  StereoPorts * stereo_in =
    fx_track->processor->stereo_in;
  channel_send_connect_stereo (
    send, fx_track->processor->stereo_in);

  /* check that the sends are correct */
  g_assert_true (!send->is_empty);
  g_assert_true (
    port_identifier_is_equal (
      &send->dest_l_id, &stereo_in->l->id) &&
    port_identifier_is_equal (
      &send->dest_r_id, &stereo_in->r->id));

  /* swap tracks */
  track_select (ins_track, true, true, false);
  action =
    move_tracks_action_new (
      TRACKLIST_SELECTIONS, 4);
  undo_manager_perform (UNDO_MANAGER, action);

  /* check that ids are updated */
  ins_track = TRACKLIST->tracks[4];
  fx_track = TRACKLIST->tracks[3];
  g_assert_true (
    ins_track->type == TRACK_TYPE_INSTRUMENT);
  g_assert_true (
    fx_track->type == TRACK_TYPE_AUDIO_BUS);
  send = &ins_track->channel->sends[0];
  stereo_in =
    fx_track->processor->stereo_in;
  g_assert_cmpint (
    stereo_in->l->id.track_pos, ==, 3);
  g_assert_cmpint (
    stereo_in->r->id.track_pos, ==, 3);
  g_assert_cmpint (send->track_pos, ==, 4);
  g_assert_true (!send->is_empty);
  g_assert_true (
    port_identifier_is_equal (
      &send->dest_l_id, &stereo_in->l->id) &&
    port_identifier_is_equal (
      &send->dest_r_id, &stereo_in->r->id));

  /* unswap tracks */
  undo_manager_undo (UNDO_MANAGER);

  /* check that ids are updated */
  ins_track = TRACKLIST->tracks[3];
  fx_track = TRACKLIST->tracks[4];
  g_assert_true (
    ins_track->type == TRACK_TYPE_INSTRUMENT);
  g_assert_true (
    fx_track->type == TRACK_TYPE_AUDIO_BUS);
  send = &ins_track->channel->sends[0];
  stereo_in =
    fx_track->processor->stereo_in;
  g_assert_cmpint (
    stereo_in->l->id.track_pos, ==, 4);
  g_assert_cmpint (
    stereo_in->r->id.track_pos, ==, 4);
  g_assert_cmpint (send->track_pos, ==, 3);
  g_assert_true (!send->is_empty);
  g_assert_true (
    port_identifier_is_equal (
      &send->dest_l_id, &stereo_in->l->id) &&
    port_identifier_is_equal (
      &send->dest_r_id, &stereo_in->r->id));
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/actions/move_tracks/"

  g_test_add_func (
    TEST_PREFIX "test move tracks",
    (GTestFunc) test_move_tracks);

  return g_test_run ();
}

