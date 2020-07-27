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

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"

#include <glib.h>

static void
_test_undo_track_deletion (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  PluginDescriptor * descr =
    test_plugin_manager_get_plugin_descriptor (
      pl_bundle, pl_uri, with_carla);

  if (is_instrument)
    {
      /* fix the descriptor (for some reason lilv
       * reports it as Plugin instead of Instrument if
       * you don't do lilv_world_load_all) */
      descr->category = PC_INSTRUMENT;
      g_free (descr->category_str);
      descr->category_str =
        plugin_descriptor_category_to_string (
          descr->category);
    }

  /* create an instrument track from helm */
  UndoableAction * ua =
    create_tracks_action_new (
      TRACK_TYPE_INSTRUMENT, descr, NULL,
      TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* save and reload the project */
  test_project_save_and_reload ();

  /* select it */
  Track * helm_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    helm_track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);

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
  track_add_region (
    helm_track, region, at, -1, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS);
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

  g_assert_true (
    track_verify_identifiers (helm_track));

  /* save and reload the project */
  test_project_save_and_reload ();

  /* delete it */
  ua =
    delete_tracks_action_new (TRACKLIST_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* save and reload the project */
  test_project_save_and_reload ();

  /* undo deletion */
  undo_manager_undo (UNDO_MANAGER);

  /* save and reload the project */
  test_project_save_and_reload ();

  g_assert_true (
    track_verify_identifiers (
      TRACKLIST->tracks[TRACKLIST->num_tracks -1]));

  /* let the engine run */
  g_usleep (1000000);
}

static void
test_undo_track_deletion (void)
{
#ifdef HAVE_HELM
  _test_undo_track_deletion (
    HELM_BUNDLE, HELM_URI, true, false);
#endif
}

static void
test_group_track_deletion (void)
{
  UndoableAction * ua;

  /* create 2 audio fx tracks and route them to
   * a new group track */
  ua =
    create_tracks_action_new_audio_fx (
      NULL, TRACKLIST->num_tracks, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * audio_fx1 =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  ua =
    create_tracks_action_new_audio_fx (
      NULL, TRACKLIST->num_tracks, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * audio_fx2 =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  ua =
    create_tracks_action_new_audio_group (
      TRACKLIST->num_tracks, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * group =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  g_assert_true (group->children);
  g_assert_false (audio_fx1->children);
  g_assert_false (audio_fx2->children);

  /* route each fx track to the group */
  track_select (
    audio_fx1, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  ua =
    edit_tracks_action_new_direct_out (
      TRACKLIST_SELECTIONS, group);
  undo_manager_perform (UNDO_MANAGER, ua);
  track_select (
    audio_fx2, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  ua =
    edit_tracks_action_new_direct_out (
      TRACKLIST_SELECTIONS, group);
  undo_manager_perform (UNDO_MANAGER, ua);

  g_assert_cmpint (group->num_children, ==, 2);
  g_assert_true (audio_fx1->channel->has_output);
  g_assert_true (audio_fx2->channel->has_output);
  g_assert_cmpint (
    audio_fx1->channel->output_pos, ==, group->pos);
  g_assert_cmpint (
    audio_fx2->channel->output_pos, ==, group->pos);

  /* save and reload the project */
  int group_pos = group->pos;
  int audio_fx1_pos = audio_fx1->pos;
  int audio_fx2_pos = audio_fx2->pos;
  test_project_save_and_reload ();
  group = TRACKLIST->tracks[group_pos];
  audio_fx1 = TRACKLIST->tracks[audio_fx1_pos];
  audio_fx2 = TRACKLIST->tracks[audio_fx2_pos];

  /* delete the group track and check that each
   * fx track has its output set to none */
  track_select (
    group, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  ua =
    delete_tracks_action_new (TRACKLIST_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);
  g_assert_false (audio_fx1->channel->has_output);
  g_assert_false (audio_fx2->channel->has_output);

  /* save and reload the project */
  audio_fx1_pos = audio_fx1->pos;
  audio_fx2_pos = audio_fx2->pos;
  test_project_save_and_reload ();
  audio_fx1 = TRACKLIST->tracks[audio_fx1_pos];
  audio_fx2 = TRACKLIST->tracks[audio_fx2_pos];

  g_assert_false (audio_fx1->channel->has_output);
  g_assert_false (audio_fx2->channel->has_output);

  /* undo and check that the connections are
   * established again */
  undo_manager_undo (UNDO_MANAGER);
  group = TRACKLIST->tracks[group_pos];
  g_assert_cmpint (group->num_children, ==, 2);
  g_assert_cmpint (
    group->children[0], ==, audio_fx1->pos);
  g_assert_cmpint (
    group->children[1], ==, audio_fx2->pos);
  g_assert_true (audio_fx1->channel->has_output);
  g_assert_true (audio_fx2->channel->has_output);
  g_assert_cmpint (
    audio_fx1->channel->output_pos, ==, group->pos);
  g_assert_cmpint (
    audio_fx2->channel->output_pos, ==, group->pos);

  /* save and reload the project */
  audio_fx1_pos = audio_fx1->pos;
  audio_fx2_pos = audio_fx2->pos;
  test_project_save_and_reload ();
  audio_fx1 = TRACKLIST->tracks[audio_fx1_pos];
  audio_fx2 = TRACKLIST->tracks[audio_fx2_pos];

  /* redo and check that the connections are
   * gone again */
  undo_manager_redo (UNDO_MANAGER);
  g_assert_false (audio_fx1->channel->has_output);
  g_assert_false (audio_fx2->channel->has_output);
}

/**
 * Asserts that src is connected/disconnected
 * to/from send.
 *
 * @param pl_out_port_idx Outgoing port index on
 *   src track.
 * @param pl_in_port_idx Incoming port index on
 *   src track.
 */
static void
assert_sends_connected (
  Track * src,
  Track * dest,
  bool    connected,
  int     pl_out_port_idx,
  int     pl_in_port_idx)
{
  if (src && dest)
    {
      bool prefader_l_connected =
        ports_connected (
          src->channel->prefader->stereo_out->l,
          dest->processor->stereo_in->l);
      bool prefader_r_connected =
        ports_connected (
          src->channel->prefader->stereo_out->r,
          dest->processor->stereo_in->r);
      bool fader_l_connected =
        ports_connected (
          src->channel->fader->stereo_out->l,
          dest->processor->stereo_in->l);
      bool fader_r_connected =
        ports_connected (
          src->channel->fader->stereo_out->r,
          dest->processor->stereo_in->r);

      bool pl_ports_connected =
        ports_connected (
          src->channel->inserts[0]->out_ports[
            pl_out_port_idx],
          dest->channel->inserts[0]->in_ports[
            pl_in_port_idx]);

      if (connected)
        {
          g_assert_true (
            prefader_l_connected &&
            prefader_r_connected &&
            fader_l_connected &&
            fader_r_connected &&
            pl_ports_connected);
        }
      else
        {
          g_assert_false (
            prefader_l_connected ||
            prefader_r_connected ||
            fader_l_connected ||
            fader_r_connected ||
            pl_ports_connected);
        }
    }

  if (src)
    {
      bool prefader_connected =
        !src->channel->sends[0].is_empty;
      bool fader_connected =
        !src->channel->sends[
          CHANNEL_SEND_POST_FADER_START_SLOT].
            is_empty;
      bool pl_ports_connected =
        src->channel->inserts[0]->out_ports[
          pl_out_port_idx]->num_dests > 0;

      if (connected)
        {
          g_assert_true (
            prefader_connected &&
            fader_connected &&
            pl_ports_connected);
        }
      else
        {
          g_assert_false (
            prefader_connected ||
            fader_connected ||
            pl_ports_connected);
        }
    }
}

static void
test_track_deletion_with_sends (
  bool         test_deleting_target,
  const char * pl_bundle,
  const char * pl_uri)
{
  UndoableAction * ua;

  PluginDescriptor * descr =
    test_plugin_manager_get_plugin_descriptor (
      pl_bundle, pl_uri, false);

  /* create an audio fx track and send both prefader
   * and postfader to a new audio fx track */
  ua =
    create_tracks_action_new_audio_fx (
      descr, TRACKLIST->num_tracks, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * audio_fx_for_sending =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  ua =
    create_tracks_action_new_audio_fx (
      descr, TRACKLIST->num_tracks, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * audio_fx_for_receiving =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  ua =
    channel_send_action_new_connect_audio (
      &audio_fx_for_sending->channel->sends[0],
      audio_fx_for_receiving->processor->
        stereo_in);
  undo_manager_perform (UNDO_MANAGER, ua);
  ua =
    channel_send_action_new_connect_audio (
      &audio_fx_for_sending->channel->sends[
        CHANNEL_SEND_POST_FADER_START_SLOT],
      audio_fx_for_receiving->processor->
        stereo_in);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* connect plugin from sending track to
   * plugin on receiving track */
  Port * out_port = NULL;
  Port * in_port = NULL;
  int out_port_idx = -1;
  int in_port_idx = -1;
  Plugin * pl =
    audio_fx_for_sending->channel->inserts[0];
  for (int i = 0; i < pl->num_out_ports; i++)
    {
      Port * port = pl->out_ports[i];
      if (port->id.type == TYPE_CV)
        {
          /* connect the first out CV port */
          out_port = port;
          out_port_idx = i;
          break;
        }
    }
  pl = audio_fx_for_receiving->channel->inserts[0];
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];
      if (port->id.type == TYPE_CONTROL &&
          port->id.flags & PORT_FLAG_PLUGIN_CONTROL)
        {
          /* connect the first in control port */
          in_port = port;
          in_port_idx = i;
          break;
        }
    }
  ua =
    port_connection_action_new_connect (
      &out_port->id, &in_port->id);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* assert they are connected */
  assert_sends_connected (
    audio_fx_for_sending, audio_fx_for_receiving,
    true, out_port_idx, in_port_idx);

  /* save and reload the project */
  int audio_fx_for_sending_pos =
    audio_fx_for_sending->pos;
  int audio_fx_for_receiving_pos =
    audio_fx_for_receiving->pos;
  test_project_save_and_reload ();
  audio_fx_for_sending =
    TRACKLIST->tracks[audio_fx_for_sending_pos];
  audio_fx_for_receiving =
    TRACKLIST->tracks[audio_fx_for_receiving_pos];

  assert_sends_connected (
    audio_fx_for_sending, audio_fx_for_receiving,
    true, out_port_idx, in_port_idx);

  if (test_deleting_target)
    {
      /* delete the receiving track and check that
       * the sends are gone */
      track_select (
        audio_fx_for_receiving, F_SELECT,
        F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
      ua =
        delete_tracks_action_new (
          TRACKLIST_SELECTIONS);
      undo_manager_perform (UNDO_MANAGER, ua);
      assert_sends_connected (
        audio_fx_for_sending, NULL, false,
        out_port_idx, in_port_idx);
      audio_fx_for_sending_pos =
        audio_fx_for_sending->pos;

      /* save and reload the project */
      test_project_save_and_reload ();
      audio_fx_for_sending =
        TRACKLIST->tracks[audio_fx_for_sending_pos];
      audio_fx_for_receiving =
        TRACKLIST->tracks[
          audio_fx_for_receiving_pos];
      assert_sends_connected (
        audio_fx_for_sending, NULL, false,
        out_port_idx, in_port_idx);
    }
  else
    {
      /* delete the sending track */
      track_select (
        audio_fx_for_sending, F_SELECT,
        F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
      ua =
        delete_tracks_action_new (
          TRACKLIST_SELECTIONS);
      undo_manager_perform (UNDO_MANAGER, ua);
      audio_fx_for_receiving_pos =
        audio_fx_for_receiving->pos;

      /* save and reload the project */
      test_project_save_and_reload ();
      audio_fx_for_receiving =
        TRACKLIST->tracks[
          audio_fx_for_receiving_pos];
    }

  /* undo and check that the sends are
   * established again */
  undo_manager_undo (UNDO_MANAGER);

  audio_fx_for_sending =
    TRACKLIST->tracks[audio_fx_for_sending_pos];
  assert_sends_connected (
    audio_fx_for_sending, audio_fx_for_receiving,
    true, out_port_idx, in_port_idx);

  /* TODO test MIDI sends */
}

static void
test_target_track_deletion_with_sends (void)
{
#ifdef HAVE_AMS_LFO
  test_track_deletion_with_sends (
    true, AMS_LFO_BUNDLE, AMS_LFO_URI);
#endif
}

static void
test_source_track_deletion_with_sends (void)
{
#ifdef HAVE_AMS_LFO
  test_track_deletion_with_sends (
    false, AMS_LFO_BUNDLE, AMS_LFO_URI);
#endif
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
  g_test_add_func (
    TEST_PREFIX "test group track deletion",
    (GTestFunc) test_group_track_deletion);
  g_test_add_func (
    TEST_PREFIX
    "test target track deletion with sends",
    (GTestFunc)
    test_target_track_deletion_with_sends);
  g_test_add_func (
    TEST_PREFIX
    "test source track deletion with sends",
    (GTestFunc)
    test_source_track_deletion_with_sends);

  return g_test_run ();
}
