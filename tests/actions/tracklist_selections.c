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

#include "zrythm-test-config.h"

#include "actions/tracklist_selections.h"
#include "actions/undoable_action.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/control_port.h"
#include "audio/master_track.h"
#include "audio/midi_event.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "audio/router.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/color.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"

#include <glib.h>

static void
test_num_tracks_with_file (
  const char * filepath,
  const int    num_tracks)
{
  SupportedFile * file =
    supported_file_new_from_path (filepath);

  int num_tracks_before = TRACKLIST->num_tracks;

  UndoableAction * ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_MIDI, NULL, file,
      num_tracks_before, PLAYHEAD, 1);
  undo_manager_perform (
    UNDO_MANAGER, ua);

  Track * first_track =
    TRACKLIST->tracks[num_tracks_before];
  track_select (
    first_track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  ua =
    tracklist_selections_action_new_delete (
      TRACKLIST_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);
  undo_manager_undo (UNDO_MANAGER);

  g_assert_cmpint (
    TRACKLIST->num_tracks, ==,
    num_tracks_before + num_tracks);
}

static void
test_create_from_midi_file (void)
{
  test_helper_zrythm_init ();

  char * midi_file;

  /* TODO this should pass in the future */
#if 0
  midi_file =
    g_build_filename (
      TESTS_SRCDIR,
      "1_empty_track_1_track_with_data.mid",
      NULL);
  test_num_tracks_with_file (midi_file, 2);
  g_free (midi_file);
#endif

  midi_file =
    g_build_filename (
      TESTS_SRCDIR,
      "1_track_with_data.mid",
      NULL);
  test_num_tracks_with_file (midi_file, 1);
  g_free (midi_file);

  g_assert_cmpint (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1]->
      lanes[0]->regions[0]->num_midi_notes, ==, 3);

  test_helper_zrythm_cleanup ();
}

static void
test_create_ins_when_redo_stack_nonempty (void)
{
  test_helper_zrythm_init ();

  UndoableAction * ua =
    tracklist_selections_action_new_create_midi (
      TRACKLIST->num_tracks, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  undo_manager_undo (UNDO_MANAGER);

#ifdef HAVE_HELM
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);
  undo_manager_undo (UNDO_MANAGER);

  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);
  undo_manager_undo (UNDO_MANAGER);
#endif

  undo_manager_redo (UNDO_MANAGER);

  test_helper_zrythm_cleanup ();
}

static void
_test_port_and_plugin_track_pos_after_duplication (
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

  /* open with carla if requested */
  descr->open_with_carla = with_carla;

  /* create an instrument track from helm */
  UndoableAction * ua =
    tracklist_selections_action_new_create (
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
  track_add_region  (
    src_track, region, at, -1, F_GEN_NAME,
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

  /* duplicate it */
  ua =
    tracklist_selections_action_new_copy (
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
    ap, prev_norm_val - 0.1f, F_NORMALIZED,
    F_NO_PUBLISH_EVENTS);
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

#ifdef HAVE_CARLA
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
#endif

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
    tracklist_selections_action_new_create (
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
    tracklist_selections_action_new_delete (
      TRACKLIST_SELECTIONS);
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
  test_helper_zrythm_init ();
#ifdef HAVE_HELM
  _test_undo_track_deletion (
    HELM_BUNDLE, HELM_URI, true, false);
#endif
  test_helper_zrythm_cleanup ();
}

static void
test_group_track_deletion (void)
{
  UndoableAction * ua;

  test_helper_zrythm_init ();

  g_assert_cmpint (
    P_MASTER_TRACK->num_children, ==, 0);

  /* create 2 audio fx tracks and route them to
   * a new group track */
  ua =
    tracklist_selections_action_new_create_audio_fx (
      NULL, TRACKLIST->num_tracks, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * audio_fx1 =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  ua =
    tracklist_selections_action_new_create_audio_fx (
      NULL, TRACKLIST->num_tracks, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * audio_fx2 =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  ua =
    tracklist_selections_action_new_create_audio_group (
      TRACKLIST->num_tracks, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * group =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];

  g_assert_true (group->children);
  g_assert_false (audio_fx1->children);
  g_assert_false (audio_fx2->children);
  g_assert_cmpint (
    P_MASTER_TRACK->num_children, ==, 3);

  /* route each fx track to the group */
  track_select (
    audio_fx1, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  ua =
    tracklist_selections_action_new_edit_direct_out(
      TRACKLIST_SELECTIONS, group);
  undo_manager_perform (UNDO_MANAGER, ua);
  g_assert_cmpint (
    P_MASTER_TRACK->num_children, ==, 2);
  undo_manager_undo (UNDO_MANAGER);
  g_assert_cmpint (
    P_MASTER_TRACK->num_children, ==, 3);
  undo_manager_redo (UNDO_MANAGER);
  g_assert_cmpint (
    P_MASTER_TRACK->num_children, ==, 2);
  track_select (
    audio_fx2, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  ua =
    tracklist_selections_action_new_edit_direct_out(
      TRACKLIST_SELECTIONS, group);
  undo_manager_perform (UNDO_MANAGER, ua);
  g_assert_cmpint (
    P_MASTER_TRACK->num_children, ==, 1);

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
    tracklist_selections_action_new_delete (
      TRACKLIST_SELECTIONS);
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

  test_helper_zrythm_cleanup ();
}

#ifdef HAVE_AMS_LFO
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
    tracklist_selections_action_new_create_audio_fx (
      descr, TRACKLIST->num_tracks, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  Track * audio_fx_for_sending =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  ua =
    tracklist_selections_action_new_create_audio_fx (
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
        tracklist_selections_action_new_delete (
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
        tracklist_selections_action_new_delete (
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
#endif

static void
test_target_track_deletion_with_sends (void)
{
  test_helper_zrythm_init ();
#ifdef HAVE_AMS_LFO
  test_track_deletion_with_sends (
    true, AMS_LFO_BUNDLE, AMS_LFO_URI);
#endif
  test_helper_zrythm_cleanup ();
}

static void
test_source_track_deletion_with_sends (void)
{
  test_helper_zrythm_init ();
#ifdef HAVE_AMS_LFO
  test_track_deletion_with_sends (
    false, AMS_LFO_BUNDLE, AMS_LFO_URI);
#endif
  test_helper_zrythm_cleanup ();
}


static void
test_audio_track_deletion (void)
{
  test_helper_zrythm_init ();

  UndoableAction * ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_AUDIO, NULL, NULL,
      TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* delete track and undo */
  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);

  /* set input gain and mono */
  port_set_control_value (
    track->processor->input_gain, 1.4f, false,
    false);
  port_set_control_value (
    track->processor->mono, 1.0f, false, false);

  ua =
    tracklist_selections_action_new_delete (
      TRACKLIST_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  undo_manager_undo (UNDO_MANAGER);

  track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  g_assert_cmpfloat_with_epsilon (
    track->processor->input_gain->control,
    1.4f, 0.001f);
  g_assert_cmpfloat_with_epsilon (
    track->processor->mono->control, 1.0f, 0.001f);

  undo_manager_redo (UNDO_MANAGER);

  test_helper_zrythm_cleanup ();
}

static void
test_track_deletion_with_lv2_worker (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_LSP_MULTISAMPLER_24_DO
  PluginDescriptor * descr =
    test_plugin_manager_get_plugin_descriptor (
      LSP_MULTISAMPLER_24_DO_BUNDLE,
      LSP_MULTISAMPLER_24_DO_URI, false);
  UndoableAction * ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_INSTRUMENT, descr, NULL,
      TRACKLIST->num_tracks, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* delete track and undo */
  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);

  ua =
    tracklist_selections_action_new_delete (
      TRACKLIST_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  undo_manager_undo (UNDO_MANAGER);

  undo_manager_redo (UNDO_MANAGER);
#endif

  test_helper_zrythm_cleanup ();
}

static void
test_ins_track_deletion_w_automation (void)
{
#if defined (HAVE_TAL_FILTER) && \
  defined (HAVE_NOIZE_MAKER)

  test_helper_zrythm_init ();

  engine_activate (AUDIO_ENGINE, false);

  /* create the instrument track */
  test_plugin_manager_create_tracks_from_plugin (
    NOIZE_MAKER_BUNDLE, NOIZE_MAKER_URI, true,
    false, 1);

  /* select the track */
  Track * track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    track, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);

  /* add an effect */
  PluginDescriptor * descr =
    test_plugin_manager_get_plugin_descriptor (
      TAL_FILTER_BUNDLE, TAL_FILTER_URI, false);
  UndoableAction * ua =
    mixer_selections_action_new_create (
      PLUGIN_SLOT_INSERT, track->pos, 0, descr, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  engine_activate (AUDIO_ENGINE, true);

  /* get the instrument cutoff automation track */
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  AutomationTrack * at = atl->ats[41];
  at->created = true;
  at->visible = true;

  /* create an automation region */
  Position start_pos, end_pos;
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 4);
  ZRegion * region =
    automation_region_new (
      &start_pos, &end_pos, track->pos,
      at->index, at->num_regions);
  track_add_region (
    track, region, at, -1, F_GEN_NAME,
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
    track_verify_identifiers (track));

  /* get the filter cutoff automation track */
  at = atl->ats[141];
  at->created = true;
  at->visible = true;

  /* create an automation region */
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 4);
  region =
    automation_region_new (
      &start_pos, &end_pos, track->pos,
      at->index, at->num_regions);
  track_add_region (
    track, region, at, -1, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) region, true, false);
  ua =
    arranger_selections_action_new_create (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* create some automation points */
  port = automation_track_get_port (at);
  position_set_to_bar (&start_pos, 1);
  ap =
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
    track_verify_identifiers (track));

  /* save and reload the project */
  test_project_save_and_reload ();

  track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  g_assert_true (
    track_verify_identifiers (track));

  /* delete it */
  ua =
    tracklist_selections_action_new_delete (
      TRACKLIST_SELECTIONS);
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

  test_helper_zrythm_cleanup ();
#endif
}

static void
test_no_visible_tracks_after_track_deletion (void)
{
  test_helper_zrythm_init ();

  /* hide all tracks */
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      track->visible = false;
    }

  UndoableAction * ua =
    tracklist_selections_action_new_create_audio_fx (
      NULL, TRACKLIST->num_tracks, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* assert a track is selected */
  g_assert_cmpint (
    TRACKLIST_SELECTIONS->num_tracks, >, 0);

  track_select (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1],
    F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

  /* delete the track */
  ua =
    tracklist_selections_action_new_delete (
      TRACKLIST_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* assert a track is selected */
  g_assert_cmpint (
    TRACKLIST_SELECTIONS->num_tracks, >, 0);

  test_helper_zrythm_cleanup ();
}

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
    tracklist_selections_action_new_move (
      TRACKLIST_SELECTIONS, 0);
  undo_manager_perform (UNDO_MANAGER, action);

  track_verify_identifiers (
    TRACKLIST->tracks[prev_pos]);
  track_verify_identifiers (
    TRACKLIST->tracks[0]);

  PluginDescriptor * descr =
    test_plugin_manager_get_plugin_descriptor (
      pl_bundle, pl_uri, with_carla);
  g_return_if_fail (descr);

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

  /* create a track with an instrument */
  action =
    tracklist_selections_action_new_create (
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
    tracklist_selections_action_new_create (
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

  /* create an automation region on the fx track */
  Position pos, end_pos;
  position_init (&pos);
  position_init (&end_pos);
  position_add_ticks (&end_pos, 600);
  ZRegion * ar =
    automation_region_new (
      &pos, &end_pos, fx_track->pos, 0, 0);
  track_add_region (
    fx_track, ar,
    fx_track->automation_tracklist.ats[0],
    -1, F_GEN_NAME, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ar, F_SELECT, F_NO_APPEND);
  action =
    arranger_selections_action_new_create (
      TL_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, action);

  /* make the region the clip editor region */
  clip_editor_set_region (
    CLIP_EDITOR, ar, F_NO_PUBLISH_EVENTS);

  /* swap tracks */
  track_select (ins_track, true, true, false);
  action =
    tracklist_selections_action_new_move (
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

  /* check that the clip editor region is updated */
  ZRegion * clip_editor_region =
    clip_editor_get_region (CLIP_EDITOR);
  g_assert_true (clip_editor_region == ar);

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

static void
test_multi_track_duplicate (void)
{
  test_helper_zrythm_init ();

  int start_pos = TRACKLIST->num_tracks;

  /* create midi track, audio fx track and audio
   * track */
  UndoableAction * ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_MIDI, NULL, NULL,
      start_pos, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_AUDIO_BUS, NULL, NULL,
      start_pos + 1, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);
  ua =
    tracklist_selections_action_new_create (
      TRACK_TYPE_AUDIO, NULL, NULL,
      start_pos + 2, NULL, 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  g_assert_true (
    TRACKLIST->tracks[start_pos]->type ==
      TRACK_TYPE_MIDI);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 1]->type ==
      TRACK_TYPE_AUDIO_BUS);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 2]->type ==
      TRACK_TYPE_AUDIO);

  /* select and duplicate */
  track_select (
    TRACKLIST->tracks[start_pos], F_SELECT,
    F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    TRACKLIST->tracks[start_pos + 1], F_SELECT,
    F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    TRACKLIST->tracks[start_pos + 2], F_SELECT,
    F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  ua =
    tracklist_selections_action_new_copy (
      TRACKLIST_SELECTIONS, start_pos + 3);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* check order correct */
  g_assert_true (
    TRACKLIST->tracks[start_pos]->type ==
      TRACK_TYPE_MIDI);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 1]->type ==
      TRACK_TYPE_AUDIO_BUS);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 2]->type ==
      TRACK_TYPE_AUDIO);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 3]->type ==
      TRACK_TYPE_MIDI);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 4]->type ==
      TRACK_TYPE_AUDIO_BUS);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 5]->type ==
      TRACK_TYPE_AUDIO);

  /* undo and check */
  undo_manager_undo (UNDO_MANAGER);
  g_assert_true (
    TRACKLIST->tracks[start_pos]->type ==
      TRACK_TYPE_MIDI);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 1]->type ==
      TRACK_TYPE_AUDIO_BUS);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 2]->type ==
      TRACK_TYPE_AUDIO);

  /* select and duplicate after first */
  track_select (
    TRACKLIST->tracks[start_pos], F_SELECT,
    F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    TRACKLIST->tracks[start_pos + 1], F_SELECT,
    F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    TRACKLIST->tracks[start_pos + 2], F_SELECT,
    F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  ua =
    tracklist_selections_action_new_copy (
      TRACKLIST_SELECTIONS, start_pos + 1);
  undo_manager_perform (UNDO_MANAGER, ua);

  /* check order correct */
  g_assert_true (
    TRACKLIST->tracks[start_pos]->type ==
      TRACK_TYPE_MIDI);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 1]->type ==
      TRACK_TYPE_MIDI);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 2]->type ==
      TRACK_TYPE_AUDIO_BUS);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 3]->type ==
      TRACK_TYPE_AUDIO);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 4]->type ==
      TRACK_TYPE_AUDIO_BUS);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 5]->type ==
      TRACK_TYPE_AUDIO);

  test_helper_zrythm_cleanup ();
}

static void
test_delete_track_w_midi_file (void)
{
  test_helper_zrythm_init ();

  char * midi_file =
    g_build_filename (
      TESTS_SRCDIR,
      "format_1_two_tracks_with_data.mid",
      NULL);
  test_num_tracks_with_file (midi_file, 2);
  g_free (midi_file);

  g_assert_cmpint (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 2]->
      lanes[0]->regions[0]->num_midi_notes, ==, 7);
  g_assert_cmpint (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1]->
      lanes[0]->regions[0]->num_midi_notes, ==, 6);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/tracklist_selections/"

  g_test_add_func (
    TEST_PREFIX "test create from midi file",
    (GTestFunc) test_create_from_midi_file);
  g_test_add_func (
    TEST_PREFIX
    "test create instrument when redo stack "
    "non-empty",
    (GTestFunc)
    test_create_ins_when_redo_stack_nonempty);
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
  g_test_add_func (
    TEST_PREFIX "test ins track deletion with automation",
    (GTestFunc) test_ins_track_deletion_w_automation);
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
  g_test_add_func (
    TEST_PREFIX "test audio track deletion",
    (GTestFunc) test_audio_track_deletion);
  g_test_add_func (
    TEST_PREFIX "test track deletion with lv2 worker",
    (GTestFunc) test_track_deletion_with_lv2_worker);
  g_test_add_func (
    TEST_PREFIX "test no visible tracks after track deletion",
    (GTestFunc) test_no_visible_tracks_after_track_deletion);
  g_test_add_func (
    TEST_PREFIX "test_move_tracks",
    (GTestFunc) test_move_tracks);
  g_test_add_func (
    TEST_PREFIX "test multi track duplicate",
    (GTestFunc) test_multi_track_duplicate);
  g_test_add_func (
    TEST_PREFIX "test delete track w midi file",
    (GTestFunc) test_delete_track_w_midi_file);

  return g_test_run ();
}
