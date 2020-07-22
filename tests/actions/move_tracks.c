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

#include "zrythm-config.h"
#include "zrythm-test-config.h"

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

static void
_test_move_tracks (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  UndoableAction * action;

  /* move markers track to the top */
  int prev_pos = P_MARKER_TRACK->pos;
  track_select (
    P_MARKER_TRACK, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  action =
    move_tracks_action_new (
      TRACKLIST_SELECTIONS, 0);
  undo_manager_perform (UNDO_MANAGER, action);

  track_verify_identifiers (
    TRACKLIST->tracks[prev_pos]);
  track_verify_identifiers (
    TRACKLIST->tracks[0]);

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

  /* create a track with an instrument */
  action =
    create_tracks_action_new (
      is_instrument ?
        TRACK_TYPE_INSTRUMENT :
        TRACK_TYPE_AUDIO_BUS,
      descr, NULL, 3, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, action);
  Track * ins_track = TRACKLIST->tracks[3];
  if (is_instrument)
    {
      g_assert_true (
        ins_track->type == TRACK_TYPE_INSTRUMENT);
    }

  /* create an fx track and send to it */
  action =
    create_tracks_action_new (
      TRACK_TYPE_AUDIO_BUS, NULL, NULL,
      4, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, action);
  Track * fx_track = TRACKLIST->tracks[4];
  g_assert_true (
    fx_track->type == TRACK_TYPE_AUDIO_BUS);

  ChannelSend * send = &ins_track->channel->sends[0];
  StereoPorts * stereo_in =
    fx_track->processor->stereo_in;
  channel_send_connect_stereo (
    send, fx_track->processor->stereo_in, NULL,
    NULL);

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
  if (is_instrument)
    {
      g_assert_true (
        ins_track->type == TRACK_TYPE_INSTRUMENT);
    }
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
  track_verify_identifiers (ins_track);
  track_verify_identifiers (fx_track);

  /* check that the stereo out of the audio fx
   * track points to the master track */
  g_assert_true (
    port_identifier_is_equal (
      &fx_track->channel->stereo_out->l->
        dest_ids[0],
      &P_MASTER_TRACK->processor->stereo_in->l->
        id));
  g_assert_true (
    port_identifier_is_equal (
      &fx_track->channel->stereo_out->r->
        dest_ids[0],
      &P_MASTER_TRACK->processor->stereo_in->r->
        id));

  /* verify fx track out ports */
  port_verify_src_and_dests (
    fx_track->channel->stereo_out->l);
  port_verify_src_and_dests (
    fx_track->channel->stereo_out->r);

  /* verify master track in ports */
  port_verify_src_and_dests (
    P_MASTER_TRACK->processor->stereo_in->l);
  port_verify_src_and_dests (
    P_MASTER_TRACK->processor->stereo_in->r);

  /* unswap tracks */
  undo_manager_undo (UNDO_MANAGER);

  /* check that ids are updated */
  ins_track = TRACKLIST->tracks[3];
  fx_track = TRACKLIST->tracks[4];
  if (is_instrument)
    {
      g_assert_true (
        ins_track->type == TRACK_TYPE_INSTRUMENT);
    }
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
  track_verify_identifiers (ins_track);
  track_verify_identifiers (fx_track);

  /* check that the stereo out of the audio fx
   * track points to the master track */
  g_assert_true (
    port_identifier_is_equal (
      &fx_track->channel->stereo_out->l->
        dest_ids[0],
      &P_MASTER_TRACK->processor->stereo_in->l->
        id));
  g_assert_true (
    port_identifier_is_equal (
      &fx_track->channel->stereo_out->r->
        dest_ids[0],
      &P_MASTER_TRACK->processor->stereo_in->r->
        id));

  /* verify fx track out ports */
  port_verify_src_and_dests (
    fx_track->channel->stereo_out->l);
  port_verify_src_and_dests (
    fx_track->channel->stereo_out->r);

  /* verify master track in ports */
  port_verify_src_and_dests (
    P_MASTER_TRACK->processor->stereo_in->l);
  port_verify_src_and_dests (
    P_MASTER_TRACK->processor->stereo_in->r);
}

static void __test_move_tracks (bool with_carla)
{
  test_helper_zrythm_init ();

#ifdef HAVE_HELM
  _test_move_tracks (
    HELM_BUNDLE, HELM_URI, true, with_carla);
#endif
#ifdef HAVE_LSP_COMPRESSOR
  _test_move_tracks (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI,
    false, with_carla);
#endif

  test_helper_zrythm_cleanup ();
}

static void test_move_tracks ()
{
  __test_move_tracks (false);
#ifdef HAVE_CARLA
  __test_move_tracks (true);
#endif
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/actions/move_tracks/"

  g_test_add_func (
    TEST_PREFIX "test_move_tracks",
    (GTestFunc) test_move_tracks);

  return g_test_run ();
}
