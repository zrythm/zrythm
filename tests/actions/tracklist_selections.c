// SPDX-FileCopyrightText: © 2020-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "actions/tracklist_selections.h"
#include "actions/undoable_action.h"
#include "audio/audio_region.h"
#include "audio/automation_region.h"
#include "audio/chord_region.h"
#include "audio/control_port.h"
#include "audio/foldable_track.h"
#include "audio/master_track.h"
#include "audio/midi_event.h"
#include "audio/midi_note.h"
#include "audio/region.h"
#include "audio/router.h"
#include "project.h"
#include "utils/color.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"

static void
test_num_tracks_with_file (
  const char * filepath,
  const int    num_tracks)
{
  SupportedFile * file =
    supported_file_new_from_path (filepath);

  int num_tracks_before = TRACKLIST->num_tracks;

  bool ret = track_create_with_action (
    TRACK_TYPE_MIDI, NULL, file, PLAYHEAD, num_tracks_before,
    1, NULL);
  g_assert_true (ret);

  Track * first_track = TRACKLIST->tracks[num_tracks_before];
  track_select (
    first_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);

  g_assert_cmpint (
    TRACKLIST->num_tracks, ==, num_tracks_before + num_tracks);
}

static void
test_create_from_midi_file (void)
{
  test_helper_zrythm_init ();

  char * midi_file;

  midi_file = g_build_filename (
    TESTS_SRCDIR, "1_empty_track_1_track_with_data.mid", NULL);
  test_num_tracks_with_file (midi_file, 1);
  g_free (midi_file);

  midi_file = g_build_filename (
    TESTS_SRCDIR, "1_track_with_data.mid", NULL);
  test_num_tracks_with_file (midi_file, 1);
  g_free (midi_file);

  g_assert_cmpint (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1]
      ->lanes[0]
      ->regions[0]
      ->num_midi_notes,
    ==, 3);

  midi_file = g_build_filename (
    TESTS_SRCDIR, "those_who_remain.mid", NULL);
  test_num_tracks_with_file (midi_file, 1);
  g_free (midi_file);

  test_helper_zrythm_cleanup ();
}

static void
test_create_ins_when_redo_stack_nonempty (void)
{
  test_helper_zrythm_init ();

  track_create_empty_with_action (TRACK_TYPE_MIDI, NULL);

  undo_manager_undo (UNDO_MANAGER, NULL);

#ifdef HAVE_HELM
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);
  undo_manager_undo (UNDO_MANAGER, NULL);

  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);
  undo_manager_undo (UNDO_MANAGER, NULL);
#endif

  undo_manager_redo (UNDO_MANAGER, NULL);

  test_helper_zrythm_cleanup ();
}

#if defined(HAVE_HELM) || defined(HAVE_LSP_COMPRESSOR)
static void
_test_port_and_plugin_track_pos_after_duplication (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, is_instrument, with_carla, 1);

  int src_track_pos = TRACKLIST->num_tracks - 1;
  int dest_track_pos = TRACKLIST->num_tracks;

  /* select it */
  Track * src_track = TRACKLIST->tracks[src_track_pos];
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
  unsigned int track_name_hash =
    track_get_name_hash (src_track);
  ZRegion * region = automation_region_new (
    &start_pos, &end_pos, track_name_hash, at->index,
    at->num_regions);
  bool success = track_add_region (
    src_track, region, at, -1, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS, NULL);
  g_assert_true (success);
  arranger_object_select (
    (ArrangerObject *) region, true, false,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);

  /* create some automation points */
  Port * port = port_find_from_identifier (&at->port_id);
  position_set_to_bar (&start_pos, 1);
  g_debug ("deff %f", (double) port->deff);
  float normalized_val =
    control_port_real_val_to_normalized (port, port->deff);
  math_assert_nonnann (port->deff);
  math_assert_nonnann (normalized_val);
  AutomationPoint * ap = automation_point_new_float (
    port->deff, normalized_val, &start_pos);
  automation_region_add_ap (region, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, true, false, F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    AUTOMATION_SELECTIONS, NULL);
  math_assert_nonnann (ap->fvalue);
  math_assert_nonnann (ap->normalized_val);

  g_assert_true (track_validate (src_track));

  /* duplicate it */
  tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR,
    TRACKLIST->num_tracks, NULL);

  Track * dest_track = TRACKLIST->tracks[dest_track_pos];

  if (is_instrument)
    {
      /* check that track processor is connected
       * to the instrument */
      PortConnection * conn =
        port_connections_manager_get_source_or_dest (
          PORT_CONNECTIONS_MGR,
          &dest_track->processor->midi_out->id, false);
      g_assert_nonnull (conn);

      /* check that instrument is connected to
       * channel prefader */
      conn = port_connections_manager_get_source_or_dest (
        PORT_CONNECTIONS_MGR,
        &dest_track->channel->prefader->stereo_in->l->id,
        true);
      g_assert_nonnull (conn);
    }

  g_assert_true (track_validate (src_track));
  g_assert_true (track_validate (dest_track));

  /* move automation in 2nd track and undo/redo */
  atl = track_get_automation_tracklist (dest_track);
  ap = atl->ats[atl->num_ats - 1]->regions[0]->aps[0];
  arranger_object_select (
    (ArrangerObject *) ap, true, false, F_NO_PUBLISH_EVENTS);
  float prev_norm_val = ap->normalized_val;
  math_assert_nonnann (ap->normalized_val);
  math_assert_nonnann (prev_norm_val);
  automation_point_set_fvalue (
    ap, prev_norm_val - 0.1f, F_NORMALIZED,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_move_automation (
    (ArrangerSelections *) AUTOMATION_SELECTIONS, 0, 0.1,
    F_ALREADY_MOVED, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* let the engine run */
  g_usleep (1000000);

  /* go back to the start */
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
}
#endif

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
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false, false);
#endif

  test_helper_zrythm_cleanup ();
}

#ifdef HAVE_CARLA
static void
test_port_and_plugin_track_pos_after_duplication_with_carla (
  void)
{
  test_helper_zrythm_init ();

#  ifdef HAVE_HELM
  _test_port_and_plugin_track_pos_after_duplication (
    HELM_BUNDLE, HELM_URI, true, true);
#  endif
#  ifdef HAVE_LSP_COMPRESSOR
  _test_port_and_plugin_track_pos_after_duplication (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false, true);
#  endif

  test_helper_zrythm_cleanup ();
}
#endif

#ifdef HAVE_HELM
static void
_test_undo_track_deletion (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, is_instrument, with_carla, 1);

  /* save and reload the project */
  test_project_save_and_reload ();

  /* select it */
  Track * helm_track =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    helm_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

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
  unsigned int track_name_hash =
    track_get_name_hash (helm_track);
  ZRegion * region = automation_region_new (
    &start_pos, &end_pos, track_name_hash, at->index,
    at->num_regions);
  bool success = track_add_region (
    helm_track, region, at, -1, F_GEN_NAME,
    F_NO_PUBLISH_EVENTS, NULL);
  g_assert_true (success);
  arranger_object_select (
    (ArrangerObject *) region, true, false,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);

  /* create some automation points */
  Port * port = port_find_from_identifier (&at->port_id);
  position_set_to_bar (&start_pos, 1);
  AutomationPoint * ap = automation_point_new_float (
    port->deff,
    control_port_real_val_to_normalized (port, port->deff),
    &start_pos);
  automation_region_add_ap (region, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, true, false, F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    AUTOMATION_SELECTIONS, NULL);

  g_assert_true (track_validate (helm_track));

  /* save and reload the project */
  test_project_save_and_reload ();

  /* delete it */
  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);

  /* save and reload the project */
  test_project_save_and_reload ();

  /* undo deletion */
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* save and reload the project */
  test_project_save_and_reload ();

  g_assert_true (track_validate (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1]));

  /* let the engine run */
  g_usleep (1000000);
}
#endif

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
  test_helper_zrythm_init ();

  g_assert_cmpint (P_MASTER_TRACK->num_children, ==, 0);

  /* create 2 audio fx tracks and route them to
   * a new group track */
  Track * audio_fx1 = track_create_empty_with_action (
    TRACK_TYPE_AUDIO_BUS, NULL);
  Track * audio_fx2 = track_create_empty_with_action (
    TRACK_TYPE_AUDIO_BUS, NULL);
  Track * group = track_create_empty_with_action (
    TRACK_TYPE_AUDIO_GROUP, NULL);

  g_assert_true (group->children);
  g_assert_false (audio_fx1->children);
  g_assert_false (audio_fx2->children);
  g_assert_cmpint (P_MASTER_TRACK->num_children, ==, 3);

  /* route each fx track to the group */
  track_select (
    audio_fx1, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, group, NULL);
  g_assert_cmpint (P_MASTER_TRACK->num_children, ==, 2);
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (P_MASTER_TRACK->num_children, ==, 3);
  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (P_MASTER_TRACK->num_children, ==, 2);
  track_select (
    audio_fx2, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, group, NULL);
  g_assert_cmpint (P_MASTER_TRACK->num_children, ==, 1);

  g_assert_cmpint (group->num_children, ==, 2);
  g_assert_true (audio_fx1->channel->has_output);
  g_assert_true (audio_fx2->channel->has_output);
  g_assert_cmpuint (
    audio_fx1->channel->output_name_hash, ==,
    track_get_name_hash (group));
  g_assert_cmpuint (
    audio_fx2->channel->output_name_hash, ==,
    track_get_name_hash (group));

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
    group, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);
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
  undo_manager_undo (UNDO_MANAGER, NULL);
  group = TRACKLIST->tracks[group_pos];
  g_assert_cmpint (group->num_children, ==, 2);
  g_assert_cmpint (
    group->children[0], ==, track_get_name_hash (audio_fx1));
  g_assert_cmpint (
    group->children[1], ==, track_get_name_hash (audio_fx2));
  g_assert_true (audio_fx1->channel->has_output);
  g_assert_true (audio_fx2->channel->has_output);
  g_assert_cmpuint (
    audio_fx1->channel->output_name_hash, ==,
    track_get_name_hash (group));
  g_assert_cmpuint (
    audio_fx2->channel->output_name_hash, ==,
    track_get_name_hash (group));

  /* save and reload the project */
  audio_fx1_pos = audio_fx1->pos;
  audio_fx2_pos = audio_fx2->pos;
  test_project_save_and_reload ();
  audio_fx1 = TRACKLIST->tracks[audio_fx1_pos];
  audio_fx2 = TRACKLIST->tracks[audio_fx2_pos];

  /* redo and check that the connections are
   * gone again */
  undo_manager_redo (UNDO_MANAGER, NULL);
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
      ChannelSend * prefader_send = src->channel->sends[0];
      ChannelSend * postfader_send =
        src->channel->sends[CHANNEL_SEND_POST_FADER_START_SLOT];

      bool prefader_connected = channel_send_is_connected_to (
        prefader_send, dest->processor->stereo_in, NULL);
      bool fader_connected = channel_send_is_connected_to (
        postfader_send, dest->processor->stereo_in, NULL);

      bool pl_ports_connected = ports_connected (
        src->channel->inserts[0]->out_ports[pl_out_port_idx],
        dest->channel->inserts[0]->in_ports[pl_in_port_idx]);

      if (connected)
        {
          g_assert_true (
            prefader_connected && fader_connected
            && pl_ports_connected);
        }
      else
        {
          g_assert_false (
            prefader_connected || fader_connected
            || pl_ports_connected);
        }
    }

  if (src)
    {
      bool prefader_connected =
        channel_send_is_enabled (src->channel->sends[0]);
      bool fader_connected = channel_send_is_enabled (
        src->channel
          ->sends[CHANNEL_SEND_POST_FADER_START_SLOT]);
      bool pl_ports_connected =
        src->channel->inserts[0]
          ->out_ports[pl_out_port_idx]
          ->num_dests
        > 0;

      if (connected)
        {
          g_assert_true (
            prefader_connected && fader_connected
            && pl_ports_connected);
        }
      else
        {
          g_assert_false (
            prefader_connected || fader_connected
            || pl_ports_connected);
        }
    }
}

static void
test_track_deletion_with_sends (
  bool         test_deleting_target,
  const char * pl_bundle,
  const char * pl_uri)
{
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      pl_bundle, pl_uri, false);

  /* create an audio fx track and send both prefader
   * and postfader to a new audio fx track */
  Track * audio_fx_for_sending =
    track_create_for_plugin_at_idx_w_action (
      TRACK_TYPE_AUDIO_BUS, setting, TRACKLIST->num_tracks,
      NULL);
  Track * audio_fx_for_receiving =
    track_create_for_plugin_at_idx_w_action (
      TRACK_TYPE_AUDIO_BUS, setting, TRACKLIST->num_tracks,
      NULL);

  g_assert_cmpuint (
    audio_fx_for_receiving->channel->sends[0]->track_name_hash,
    ==,
    audio_fx_for_receiving->channel->sends[0]
      ->amount->id.track_name_hash);

  ChannelSend * prefader_send =
    audio_fx_for_sending->channel->sends[0];
  channel_send_action_perform_connect_audio (
    prefader_send,
    audio_fx_for_receiving->processor->stereo_in, NULL);
  ChannelSend * postfader_send =
    audio_fx_for_sending->channel
      ->sends[CHANNEL_SEND_POST_FADER_START_SLOT];
  channel_send_action_perform_connect_audio (
    postfader_send,
    audio_fx_for_receiving->processor->stereo_in, NULL);

  tracklist_validate (TRACKLIST);

  /* connect plugin from sending track to
   * plugin on receiving track */
  Port *   out_port = NULL;
  Port *   in_port = NULL;
  int      out_port_idx = -1;
  int      in_port_idx = -1;
  Plugin * pl = audio_fx_for_sending->channel->inserts[0];
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
      if (
        port->id.type == TYPE_CONTROL
        && port->id.flags & PORT_FLAG_PLUGIN_CONTROL)
        {
          /* connect the first in control port */
          in_port = port;
          in_port_idx = i;
          break;
        }
    }
  port_connection_action_perform_connect (
    &out_port->id, &in_port->id, NULL);

  /* assert they are connected */
  assert_sends_connected (
    audio_fx_for_sending, audio_fx_for_receiving, true,
    out_port_idx, in_port_idx);

  g_assert_cmpuint (
    prefader_send->track_name_hash, ==,
    prefader_send->amount->id.track_name_hash);
  g_assert_cmpuint (
    postfader_send->track_name_hash, ==,
    postfader_send->amount->id.track_name_hash);

  /* save and reload the project */
  int audio_fx_for_sending_pos = audio_fx_for_sending->pos;
  int audio_fx_for_receiving_pos = audio_fx_for_receiving->pos;
  test_project_save_and_reload ();
  audio_fx_for_sending =
    TRACKLIST->tracks[audio_fx_for_sending_pos];
  audio_fx_for_receiving =
    TRACKLIST->tracks[audio_fx_for_receiving_pos];

  assert_sends_connected (
    audio_fx_for_sending, audio_fx_for_receiving, true,
    out_port_idx, in_port_idx);

  if (test_deleting_target)
    {
      /* delete the receiving track and check that
       * the sends are gone */
      track_select (
        audio_fx_for_receiving, F_SELECT, F_EXCLUSIVE,
        F_NO_PUBLISH_EVENTS);
      tracklist_selections_action_perform_delete (
        TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);
      assert_sends_connected (
        audio_fx_for_sending, NULL, false, out_port_idx,
        in_port_idx);
      audio_fx_for_sending_pos = audio_fx_for_sending->pos;

      /* save and reload the project */
      test_project_save_and_reload ();
      audio_fx_for_sending =
        TRACKLIST->tracks[audio_fx_for_sending_pos];
      audio_fx_for_receiving =
        TRACKLIST->tracks[audio_fx_for_receiving_pos];
      assert_sends_connected (
        audio_fx_for_sending, NULL, false, out_port_idx,
        in_port_idx);
    }
  else
    {
      /* wait for a few cycles */
      engine_wait_n_cycles (AUDIO_ENGINE, 40);

      /* delete the sending track */
      track_select (
        audio_fx_for_sending, F_SELECT, F_EXCLUSIVE,
        F_NO_PUBLISH_EVENTS);
      tracklist_selections_action_perform_delete (
        TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);
      audio_fx_for_receiving_pos = audio_fx_for_receiving->pos;

      /* save and reload the project */
      test_project_save_and_reload ();
      audio_fx_for_receiving =
        TRACKLIST->tracks[audio_fx_for_receiving_pos];
    }

  project_validate (PROJECT);

  /* undo and check that the sends are
   * established again */
  undo_manager_undo (UNDO_MANAGER, NULL);

  audio_fx_for_sending =
    TRACKLIST->tracks[audio_fx_for_sending_pos];
  assert_sends_connected (
    audio_fx_for_sending, audio_fx_for_receiving, true,
    out_port_idx, in_port_idx);

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

  Track * track =
    track_create_empty_with_action (TRACK_TYPE_AUDIO, NULL);

  /* delete track and undo */
  track_select (
    track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

  /* set input gain and mono */
  port_set_control_value (
    track->processor->input_gain, 1.4f, false, false);
  port_set_control_value (
    track->processor->mono, 1.0f, false, false);

  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);

  undo_manager_undo (UNDO_MANAGER, NULL);

  track = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  g_assert_cmpfloat_with_epsilon (
    track->processor->input_gain->control, 1.4f, 0.001f);
  g_assert_cmpfloat_with_epsilon (
    track->processor->mono->control, 1.0f, 0.001f);

  undo_manager_redo (UNDO_MANAGER, NULL);

  test_helper_zrythm_cleanup ();
}

static void
test_track_deletion_with_lv2_worker (void)
{
  test_helper_zrythm_init ();

#ifdef HAVE_LSP_MULTISAMPLER_24_DO
  test_plugin_manager_create_tracks_from_plugin (
    LSP_MULTISAMPLER_24_DO_BUNDLE, LSP_MULTISAMPLER_24_DO_URI,
    true, false, 1);

  /* delete track and undo */
  Track * track = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);

  undo_manager_undo (UNDO_MANAGER, NULL);

  undo_manager_redo (UNDO_MANAGER, NULL);
#endif

  test_helper_zrythm_cleanup ();
}

static void
test_ins_track_deletion_w_automation (void)
{
#if defined(HAVE_TAL_FILTER) && defined(HAVE_NOIZE_MAKER)

  test_helper_zrythm_init ();

  engine_activate (AUDIO_ENGINE, false);

  /* create the instrument track */
  test_plugin_manager_create_tracks_from_plugin (
    NOIZE_MAKER_BUNDLE, NOIZE_MAKER_URI, true, false, 1);

  /* select the track */
  Track * track = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

  /* add an effect */
  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      TAL_FILTER_BUNDLE, TAL_FILTER_URI, false);
  mixer_selections_action_perform_create (
    PLUGIN_SLOT_INSERT, track_get_name_hash (track), 0,
    setting, 1, NULL);

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
  ZRegion * region = automation_region_new (
    &start_pos, &end_pos, track_get_name_hash (track),
    at->index, at->num_regions);
  bool success = track_add_region (
    track, region, at, -1, F_GEN_NAME, F_NO_PUBLISH_EVENTS,
    NULL);
  g_assert_true (success);
  arranger_object_select (
    (ArrangerObject *) region, true, false,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);

  /* create some automation points */
  Port * port = port_find_from_identifier (&at->port_id);
  position_set_to_bar (&start_pos, 1);
  AutomationPoint * ap = automation_point_new_float (
    port->deff,
    control_port_real_val_to_normalized (port, port->deff),
    &start_pos);
  automation_region_add_ap (region, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, true, false, F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    AUTOMATION_SELECTIONS, NULL);

  g_assert_true (track_validate (track));

  /* get the filter cutoff automation track */
  at = atl->ats[141];
  at->created = true;
  at->visible = true;

  /* create an automation region */
  position_set_to_bar (&start_pos, 2);
  position_set_to_bar (&end_pos, 4);
  region = automation_region_new (
    &start_pos, &end_pos, track_get_name_hash (track),
    at->index, at->num_regions);
  success = track_add_region (
    track, region, at, -1, F_GEN_NAME, F_NO_PUBLISH_EVENTS,
    NULL);
  g_assert_true (success);
  arranger_object_select (
    (ArrangerObject *) region, true, false,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);

  /* create some automation points */
  port = port_find_from_identifier (&at->port_id);
  position_set_to_bar (&start_pos, 1);
  ap = automation_point_new_float (
    port->deff,
    control_port_real_val_to_normalized (port, port->deff),
    &start_pos);
  automation_region_add_ap (region, ap, F_NO_PUBLISH_EVENTS);
  arranger_object_select (
    (ArrangerObject *) ap, true, false, F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    AUTOMATION_SELECTIONS, NULL);

  g_assert_true (track_validate (track));

  /* save and reload the project */
  test_project_save_and_reload ();

  track = TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  g_assert_true (track_validate (track));

  /* delete it */
  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);

  /* save and reload the project */
  test_project_save_and_reload ();

  /* undo deletion */
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* save and reload the project */
  test_project_save_and_reload ();

  g_assert_true (track_validate (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1]));

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

  track_create_empty_with_action (TRACK_TYPE_AUDIO_BUS, NULL);

  /* assert a track is selected */
  g_assert_cmpint (TRACKLIST_SELECTIONS->num_tracks, >, 0);

  track_select (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1], F_SELECT,
    F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

  /* delete the track */
  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);

  /* assert a track is selected */
  g_assert_cmpint (TRACKLIST_SELECTIONS->num_tracks, >, 0);

  /* assert undo history is not empty */
  g_assert_false (
    undo_stack_is_empty (UNDO_MANAGER->undo_stack));

  test_helper_zrythm_cleanup ();
}

#if defined(HAVE_HELM) || defined(HAVE_LSP_COMPRESSOR)
static void
_test_move_tracks (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  /* move markers track to the top */
  int prev_pos = P_MARKER_TRACK->pos;
  track_select (
    P_MARKER_TRACK, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_move (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, 0, NULL);

  track_validate (TRACKLIST->tracks[prev_pos]);
  track_validate (TRACKLIST->tracks[0]);

  PluginSetting * setting =
    test_plugin_manager_get_plugin_setting (
      pl_bundle, pl_uri, with_carla);
  g_return_if_fail (setting);

  /* fix the descriptor (for some reason lilv
   * reports it as Plugin instead of Instrument if
   * you don't do lilv_world_load_all) */
  if (is_instrument)
    {
      setting->descr->category = PC_INSTRUMENT;
    }
  g_free (setting->descr->category_str);
  setting->descr
    ->category_str = plugin_descriptor_category_to_string (
    setting->descr->category);

  /* create a track with an instrument */
  track_create_for_plugin_at_idx_w_action (
    is_instrument ? TRACK_TYPE_INSTRUMENT : TRACK_TYPE_AUDIO_BUS,
    setting, 3, NULL);
  Track * ins_track = TRACKLIST->tracks[3];
  if (is_instrument)
    {
      g_assert_true (ins_track->type == TRACK_TYPE_INSTRUMENT);
    }

  /* create an fx track and send to it */
  track_create_with_action (
    TRACK_TYPE_AUDIO_BUS, NULL, NULL, NULL, 4, 1, NULL);
  Track * fx_track = TRACKLIST->tracks[4];
  g_assert_true (fx_track->type == TRACK_TYPE_AUDIO_BUS);

  ChannelSend * send = ins_track->channel->sends[0];
  StereoPorts * stereo_in = fx_track->processor->stereo_in;
  GError *      err = NULL;
  bool          ret = channel_send_connect_stereo (
    send, fx_track->processor->stereo_in, NULL, NULL, false,
    F_VALIDATE, F_NO_RECALC_GRAPH, &err);
  g_assert_true (ret);

  /* check that the sends are correct */
  g_assert_true (channel_send_is_enabled (send));
  g_assert_true (
    channel_send_is_connected_to (send, stereo_in, NULL));

  /* create an automation region on the fx track */
  Position pos, end_pos;
  position_init (&pos);
  position_init (&end_pos);
  position_add_ticks (&end_pos, 600);
  ZRegion * ar = automation_region_new (
    &pos, &end_pos, track_get_name_hash (fx_track), 0, 0);
  bool success = track_add_region (
    fx_track, ar, fx_track->automation_tracklist.ats[0], -1,
    F_GEN_NAME, F_NO_PUBLISH_EVENTS, NULL);
  g_assert_true (success);
  arranger_object_select (
    (ArrangerObject *) ar, F_SELECT, F_NO_APPEND,
    F_NO_PUBLISH_EVENTS);
  arranger_selections_action_perform_create (
    TL_SELECTIONS, NULL);

  /* make the region the clip editor region */
  clip_editor_set_region (
    CLIP_EDITOR, ar, F_NO_PUBLISH_EVENTS);

  /* swap tracks */
  track_select (
    ins_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  engine_wait_n_cycles (AUDIO_ENGINE, 40);
  tracklist_selections_action_perform_move (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, 5, NULL);

  /* check that ids are updated */
  ins_track = TRACKLIST->tracks[4];
  fx_track = TRACKLIST->tracks[3];
  if (is_instrument)
    {
      g_assert_true (ins_track->type == TRACK_TYPE_INSTRUMENT);
    }
  g_assert_true (fx_track->type == TRACK_TYPE_AUDIO_BUS);
  send = ins_track->channel->sends[0];
  stereo_in = fx_track->processor->stereo_in;
  g_assert_cmpuint (
    stereo_in->l->id.track_name_hash, ==,
    track_get_name_hash (fx_track));
  g_assert_cmpuint (
    stereo_in->r->id.track_name_hash, ==,
    track_get_name_hash (fx_track));
  g_assert_true (send->track == ins_track);
  g_assert_true (channel_send_is_enabled (send));
  g_assert_true (
    channel_send_is_connected_to (send, stereo_in, NULL));
  track_validate (ins_track);
  track_validate (fx_track);

  /* check that the clip editor region is updated */
  ZRegion * clip_editor_region =
    clip_editor_get_region (CLIP_EDITOR);
  g_assert_true (clip_editor_region == ar);

  /* check that the stereo out of the audio fx
   * track points to the master track */
  g_assert_nonnull (port_connections_manager_find_connection (
    PORT_CONNECTIONS_MGR,
    &fx_track->channel->stereo_out->l->id,
    &P_MASTER_TRACK->processor->stereo_in->l->id));
  g_assert_nonnull (port_connections_manager_find_connection (
    PORT_CONNECTIONS_MGR,
    &fx_track->channel->stereo_out->r->id,
    &P_MASTER_TRACK->processor->stereo_in->r->id));

  /* TODO replace below */
#  if 0
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
#  endif

  /* unswap tracks */
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* check that ids are updated */
  ins_track = TRACKLIST->tracks[3];
  fx_track = TRACKLIST->tracks[4];
  if (is_instrument)
    {
      g_assert_true (ins_track->type == TRACK_TYPE_INSTRUMENT);
    }
  g_assert_true (fx_track->type == TRACK_TYPE_AUDIO_BUS);
  send = ins_track->channel->sends[0];
  stereo_in = fx_track->processor->stereo_in;
  g_assert_cmpuint (
    stereo_in->l->id.track_name_hash, ==,
    track_get_name_hash (fx_track));
  g_assert_cmpuint (
    stereo_in->r->id.track_name_hash, ==,
    track_get_name_hash (fx_track));
  g_assert_true (send->track == ins_track);
  g_assert_true (channel_send_is_enabled (send));
  g_assert_true (
    channel_send_is_connected_to (send, stereo_in, NULL));
  track_validate (ins_track);
  track_validate (fx_track);

  /* check that the stereo out of the audio fx
   * track points to the master track */
  g_assert_nonnull (port_connections_manager_find_connection (
    PORT_CONNECTIONS_MGR,
    &fx_track->channel->stereo_out->l->id,
    &P_MASTER_TRACK->processor->stereo_in->l->id));
  g_assert_nonnull (port_connections_manager_find_connection (
    PORT_CONNECTIONS_MGR,
    &fx_track->channel->stereo_out->r->id,
    &P_MASTER_TRACK->processor->stereo_in->r->id));

#  if 0
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
#  endif
}
#endif

static void
__test_move_tracks (bool with_carla)
{
  test_helper_zrythm_init ();

#ifdef HAVE_HELM
  _test_move_tracks (HELM_BUNDLE, HELM_URI, true, with_carla);
#endif
#ifdef HAVE_LSP_COMPRESSOR
  _test_move_tracks (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false,
    with_carla);
#endif

  test_helper_zrythm_cleanup ();
}

static void
test_move_tracks (void)
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
  track_create_with_action (
    TRACK_TYPE_MIDI, NULL, NULL, NULL, start_pos, 1, NULL);
  track_create_with_action (
    TRACK_TYPE_AUDIO_BUS, NULL, NULL, NULL, start_pos + 1, 1,
    NULL);
  track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, NULL, NULL, start_pos + 2, 1,
    NULL);

  g_assert_true (
    TRACKLIST->tracks[start_pos]->type == TRACK_TYPE_MIDI);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 1]->type
    == TRACK_TYPE_AUDIO_BUS);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 2]->type
    == TRACK_TYPE_AUDIO);

  /* select and duplicate */
  track_select (
    TRACKLIST->tracks[start_pos], F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  track_select (
    TRACKLIST->tracks[start_pos + 1], F_SELECT,
    F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    TRACKLIST->tracks[start_pos + 2], F_SELECT,
    F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, start_pos + 3,
    NULL);

  /* check order correct */
  g_assert_true (
    TRACKLIST->tracks[start_pos]->type == TRACK_TYPE_MIDI);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 1]->type
    == TRACK_TYPE_AUDIO_BUS);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 2]->type
    == TRACK_TYPE_AUDIO);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 3]->type == TRACK_TYPE_MIDI);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 4]->type
    == TRACK_TYPE_AUDIO_BUS);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 5]->type
    == TRACK_TYPE_AUDIO);

  /* undo and check */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_true (
    TRACKLIST->tracks[start_pos]->type == TRACK_TYPE_MIDI);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 1]->type
    == TRACK_TYPE_AUDIO_BUS);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 2]->type
    == TRACK_TYPE_AUDIO);

  /* select and duplicate after first */
  track_select (
    TRACKLIST->tracks[start_pos], F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  track_select (
    TRACKLIST->tracks[start_pos + 1], F_SELECT,
    F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    TRACKLIST->tracks[start_pos + 2], F_SELECT,
    F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, start_pos + 1,
    NULL);

  /* check order correct */
  g_assert_true (
    TRACKLIST->tracks[start_pos]->type == TRACK_TYPE_MIDI);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 1]->type == TRACK_TYPE_MIDI);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 2]->type
    == TRACK_TYPE_AUDIO_BUS);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 3]->type
    == TRACK_TYPE_AUDIO);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 4]->type
    == TRACK_TYPE_AUDIO_BUS);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 5]->type
    == TRACK_TYPE_AUDIO);

  test_helper_zrythm_cleanup ();
}

static void
test_delete_track_w_midi_file (void)
{
  test_helper_zrythm_init ();

  char * midi_file = g_build_filename (
    TESTS_SRCDIR, "format_1_two_tracks_with_data.mid", NULL);
  test_num_tracks_with_file (midi_file, 2);
  g_free (midi_file);

  g_assert_cmpint (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 2]
      ->lanes[0]
      ->regions[0]
      ->num_midi_notes,
    ==, 7);
  g_assert_cmpint (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1]
      ->lanes[0]
      ->regions[0]
      ->num_midi_notes,
    ==, 6);

  test_helper_zrythm_cleanup ();
}

static void
test_marker_track_unpin (void)
{
  test_helper_zrythm_init ();

  for (int i = 0; i < P_MARKER_TRACK->num_markers; i++)
    {
      Marker * m = P_MARKER_TRACK->markers[i];
      Track *  m_track =
        arranger_object_get_track ((ArrangerObject *) m);
      g_assert_true (m_track == P_MARKER_TRACK);
      g_assert_true (track_is_pinned (m_track));
    }

  track_select (
    P_MARKER_TRACK, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_unpin (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);

  for (int i = 0; i < P_MARKER_TRACK->num_markers; i++)
    {
      Marker * m = P_MARKER_TRACK->markers[i];
      Track *  m_track =
        arranger_object_get_track ((ArrangerObject *) m);
      g_assert_true (m_track == P_MARKER_TRACK);
      g_assert_false (track_is_pinned (m_track));
    }

  undo_manager_undo (UNDO_MANAGER, NULL);

  for (int i = 0; i < P_MARKER_TRACK->num_markers; i++)
    {
      Marker * m = P_MARKER_TRACK->markers[i];
      Track *  m_track =
        arranger_object_get_track ((ArrangerObject *) m);
      g_assert_true (m_track == P_MARKER_TRACK);
      g_assert_true (track_is_pinned (m_track));
    }

  test_helper_zrythm_cleanup ();
}

/**
 * Test duplicate when the last track in the
 * tracklist is the output of the duplicated track.
 */
static void
test_duplicate_w_output_and_send (void)
{
  test_helper_zrythm_init ();

  int start_pos = TRACKLIST->num_tracks;

  /* create audio track + audio group track + audio
   * fx track */
  track_create_with_action (
    TRACK_TYPE_AUDIO, NULL, NULL, NULL, start_pos, 1, NULL);
  track_create_with_action (
    TRACK_TYPE_AUDIO_GROUP, NULL, NULL, NULL, start_pos + 1,
    1, NULL);
  track_create_with_action (
    TRACK_TYPE_AUDIO_GROUP, NULL, NULL, NULL, start_pos + 2,
    1, NULL);
  track_create_with_action (
    TRACK_TYPE_AUDIO_BUS, NULL, NULL, NULL, start_pos + 3, 1,
    NULL);

  Track * audio_track = TRACKLIST->tracks[start_pos];
  Track * group_track = TRACKLIST->tracks[start_pos + 1];
  Track * group_track2 = TRACKLIST->tracks[start_pos + 2];
  Track * fx_track = TRACKLIST->tracks[start_pos + 3];

  /* send from audio track to fx track */
  channel_send_action_perform_connect_audio (
    audio_track->channel->sends[0],
    fx_track->processor->stereo_in, NULL);

  g_assert_true (
    TRACKLIST->tracks[start_pos]->type == TRACK_TYPE_AUDIO);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 1]->type
    == TRACK_TYPE_AUDIO_GROUP);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 2]->type
    == TRACK_TYPE_AUDIO_GROUP);
  g_assert_true (
    TRACKLIST->tracks[start_pos + 3]->type
    == TRACK_TYPE_AUDIO_BUS);

  /* route audio track to group track */
  track_select (
    TRACKLIST->tracks[start_pos], F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, group_track,
    NULL);

  /* route group track to group track 2 */
  track_select (
    TRACKLIST->tracks[start_pos + 1], F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_set_direct_out (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, group_track2,
    NULL);

  /* duplicate audio track and group track */
  track_select (
    TRACKLIST->tracks[start_pos], F_SELECT, F_NOT_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  g_assert_cmpint (TRACKLIST_SELECTIONS->num_tracks, ==, 2);
  g_assert_true (
    TRACKLIST_SELECTIONS->tracks[0] == group_track);
  g_assert_true (
    TRACKLIST_SELECTIONS->tracks[1] == audio_track);
  engine_wait_n_cycles (AUDIO_ENGINE, 40);
  tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, start_pos + 1,
    NULL);

  /* assert group of new audio track is group of
   * original audio track */
  Track * new_track = TRACKLIST->tracks[start_pos + 1];
  g_assert_true (new_track->type == TRACK_TYPE_AUDIO);
  g_assert_cmpuint (
    new_track->channel->output_name_hash, ==,
    track_get_name_hash (group_track));
  g_assert_true (
    ports_connected (
      new_track->channel->stereo_out->l,
      group_track->processor->stereo_in->l)
    && ports_connected (
      new_track->channel->stereo_out->r,
      group_track->processor->stereo_in->r));

  /* assert group of new group track is group of
   * original group track */
  Track * new_group_track = TRACKLIST->tracks[start_pos + 2];
  g_assert_true (
    new_group_track->type == TRACK_TYPE_AUDIO_GROUP);
  g_assert_cmpuint (
    new_group_track->channel->output_name_hash, ==,
    track_get_name_hash (group_track2));
  g_assert_true (
    ports_connected (
      new_group_track->channel->stereo_out->l,
      group_track2->processor->stereo_in->l)
    && ports_connected (
      new_group_track->channel->stereo_out->r,
      group_track2->processor->stereo_in->r));

  /* assert new audio track sends connected */
  ChannelSend * send = new_track->channel->sends[0];
  channel_send_validate (send);
  g_assert_true (channel_send_is_enabled (send));

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /* test delete each track */
  for (int i = 0; i < 4; i++)
    {
      new_track = TRACKLIST->tracks[start_pos + i];
      track_select (
        new_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
      tracklist_selections_action_perform_delete (
        TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);
      tracklist_validate (TRACKLIST);
      test_project_save_and_reload ();
      undo_manager_undo (UNDO_MANAGER, NULL);
      undo_manager_redo (UNDO_MANAGER, NULL);
      undo_manager_undo (UNDO_MANAGER, NULL);
    }

  test_helper_zrythm_cleanup ();
}

static void
test_track_deletion_w_mixer_selections (void)
{
  test_helper_zrythm_init ();

  Track * first_track = track_create_empty_with_action (
    TRACK_TYPE_AUDIO_BUS, NULL);

  int pl_track_pos =
    test_plugin_manager_create_tracks_from_plugin (
      EG_AMP_BUNDLE_URI, EG_AMP_URI, false, false, 1);
  Track * pl_track = TRACKLIST->tracks[pl_track_pos];

  mixer_selections_add_slot (
    MIXER_SELECTIONS, pl_track, PLUGIN_SLOT_INSERT, 0,
    F_NO_CLONE);
  g_assert_true (MIXER_SELECTIONS->has_any);
  g_assert_cmpuint (
    MIXER_SELECTIONS->track_name_hash, ==,
    track_get_name_hash (pl_track));

  track_select (
    first_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);

  test_helper_zrythm_cleanup ();
}

static void
test_ins_track_duplicate_w_send (void)
{
#if defined HAVE_HELM
  test_helper_zrythm_init ();

  /* create the instrument track */
  int ins_track_pos = TRACKLIST->num_tracks;
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, false, 1);
  Track * ins_track = TRACKLIST->tracks[ins_track_pos];

  /* create an audio fx track */
  int audio_fx_track_pos = TRACKLIST->num_tracks;
  track_create_with_action (
    TRACK_TYPE_AUDIO_BUS, NULL, NULL, NULL,
    audio_fx_track_pos, 1, NULL);
  Track * audio_fx_track =
    TRACKLIST->tracks[audio_fx_track_pos];

  /* send from audio track to fx track */
  channel_send_action_perform_connect_audio (
    ins_track->channel->sends[0],
    audio_fx_track->processor->stereo_in, NULL);

  /* duplicate the instrument track */
  track_select (
    ins_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  engine_wait_n_cycles (AUDIO_ENGINE, 40);
  tracklist_selections_action_perform_copy (
    TRACKLIST_SELECTIONS,

    PORT_CONNECTIONS_MGR, audio_fx_track_pos, NULL);
  Track * new_ins_track =
    TRACKLIST->tracks[audio_fx_track_pos];
  g_assert_true (new_ins_track->type == TRACK_TYPE_INSTRUMENT);

  /* assert new instrument track send is connected
   * to audio fx track */
  ChannelSend * send = new_ins_track->channel->sends[0];
  channel_send_validate (send);
  g_assert_true (channel_send_is_enabled (send));

  /* save and reload the project */
  test_project_save_and_reload ();

  test_helper_zrythm_cleanup ();
#endif
}

static void
test_create_midi_fx_track (void)
{
  test_helper_zrythm_init ();

  /* create an audio fx track */
  track_create_empty_with_action (TRACK_TYPE_MIDI_BUS, NULL);

  g_usleep (10000);

  test_helper_zrythm_cleanup ();
}

static void
test_move_multiple_tracks (void)
{
  test_helper_zrythm_init ();

  /* create 5 tracks */
  for (int i = 0; i < 5; i++)
    {
      track_create_empty_with_action (
        TRACK_TYPE_AUDIO_BUS, NULL);
    }

  /* move the first 2 tracks to the end */
  Track * track1 =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 5];
  Track * track2 =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 4];
  Track * track3 =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 3];
  Track * track4 =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 2];
  Track * track5 =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    track1, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    track2, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  g_assert_cmpint (track1->pos, <, track2->pos);
  tracklist_selections_action_perform_move (
    TRACKLIST_SELECTIONS,

    PORT_CONNECTIONS_MGR, TRACKLIST->num_tracks, NULL);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 1);

  /* same (move first 2 tracks to end), but select
   * in different order */
  track_select (
    track2, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    track1, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_move (
    TRACKLIST_SELECTIONS,

    PORT_CONNECTIONS_MGR, TRACKLIST->num_tracks, NULL);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 1);

  /* move the last 2 tracks up */
  track_select (
    track4, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    track5, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  g_assert_cmpint (track4->pos, <, track5->pos);
  tracklist_selections_action_perform_move (
    TRACKLIST_SELECTIONS,

    PORT_CONNECTIONS_MGR, TRACKLIST->num_tracks - 5, NULL);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 1);

  /* move the first and last tracks to the middle */
  track_select (
    track1, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    track5, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_move (
    TRACKLIST_SELECTIONS,

    PORT_CONNECTIONS_MGR, TRACKLIST->num_tracks - 2, NULL);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (track1->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (track2->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (track3->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (track4->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (track5->pos, ==, TRACKLIST->num_tracks - 1);

  test_helper_zrythm_cleanup ();
}

static void
_test_move_inside (
  const char * pl_bundle,
  const char * pl_uri,
  bool         is_instrument,
  bool         with_carla)
{
  test_helper_zrythm_init ();

  /* create folder track */
  track_create_empty_with_action (TRACK_TYPE_FOLDER, NULL);

  /* create audio fx track */
  test_plugin_manager_create_tracks_from_plugin (
    pl_bundle, pl_uri, is_instrument, with_carla, 1);

  /* move audio fx inside folder */
  Track * folder =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 2];
  g_assert_cmpint (folder->size, ==, 1);
  Track * audio_fx =
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1];
  track_select (
    audio_fx, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, folder, TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  /* validate */
  g_assert_cmpint (folder->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (
    audio_fx->pos, ==, TRACKLIST->num_tracks - 1);
  g_assert_cmpint (folder->size, ==, 2);
  GPtrArray * parents = g_ptr_array_new ();
  track_add_folder_parents (audio_fx, parents, false);
  g_assert_cmpuint (parents->len, ==, 1);
  g_assert_true (g_ptr_array_index (parents, 0) == folder);
  g_ptr_array_unref (parents);

  /* create audio group and move folder inside
   * group */
  Track * audio_group = track_create_empty_with_action (
    TRACK_TYPE_AUDIO_GROUP, NULL);
  track_select (
    folder, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    audio_fx, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, audio_group, TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  /* validate */
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (
    audio_fx->pos, ==, TRACKLIST->num_tracks - 1);
  g_assert_cmpint (folder->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (
    audio_group->pos, ==, TRACKLIST->num_tracks - 3);
  parents = g_ptr_array_new ();
  track_add_folder_parents (audio_fx, parents, false);
  g_assert_cmpuint (parents->len, ==, 2);
  g_assert_true (
    g_ptr_array_index (parents, 0) == audio_group);
  g_assert_true (g_ptr_array_index (parents, 1) == folder);
  g_ptr_array_unref (parents);
  parents = g_ptr_array_new ();
  track_add_folder_parents (folder, parents, false);
  g_assert_cmpuint (parents->len, ==, 1);
  g_assert_true (
    g_ptr_array_index (parents, 0) == audio_group);
  g_ptr_array_unref (parents);

  /* add 3 more tracks */
  Track * audio_fx2 = track_create_empty_with_action (
    TRACK_TYPE_AUDIO_BUS, NULL);
  Track * folder2 =
    track_create_empty_with_action (TRACK_TYPE_FOLDER, NULL);
  Track * audio_fx3 = track_create_empty_with_action (
    TRACK_TYPE_AUDIO_BUS, NULL);

  /* move 2 new fx tracks to folder */
  track_select (
    audio_fx2, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    audio_fx3, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, folder2, TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  /* validate */
  g_assert_cmpint (folder2->size, ==, 3);
  g_assert_cmpint (
    folder2->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (
    audio_fx2->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (
    audio_fx3->pos, ==, TRACKLIST->num_tracks - 1);
  parents = g_ptr_array_new ();
  track_add_folder_parents (audio_fx2, parents, false);
  g_assert_cmpuint (parents->len, ==, 1);
  g_assert_true (g_ptr_array_index (parents, 0) == folder2);
  g_ptr_array_unref (parents);
  parents = g_ptr_array_new ();
  track_add_folder_parents (audio_fx3, parents, false);
  g_assert_cmpuint (parents->len, ==, 1);
  g_assert_true (g_ptr_array_index (parents, 0) == folder2);
  g_ptr_array_unref (parents);

  /* undo move 2 tracks inside */
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* undo create 3 tracks */
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* validate */
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (
    audio_fx->pos, ==, TRACKLIST->num_tracks - 1);
  g_assert_cmpint (folder->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (
    audio_group->pos, ==, TRACKLIST->num_tracks - 3);
  parents = g_ptr_array_new ();
  track_add_folder_parents (audio_fx, parents, false);
  g_assert_cmpuint (parents->len, ==, 2);
  g_assert_true (
    g_ptr_array_index (parents, 0) == audio_group);
  g_assert_true (g_ptr_array_index (parents, 1) == folder);
  g_ptr_array_unref (parents);
  parents = g_ptr_array_new ();
  track_add_folder_parents (folder, parents, false);
  g_assert_cmpuint (parents->len, ==, 1);
  g_assert_true (
    g_ptr_array_index (parents, 0) == audio_group);
  g_ptr_array_unref (parents);

  /* undo move folder inside group */
  undo_manager_undo (UNDO_MANAGER, NULL);

  undo_manager_undo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);
  undo_manager_redo (UNDO_MANAGER, NULL);

  /*
   * [00] Chords
   * [01] Tempo
   * [02] Modulators
   * [03] Markers
   * [04] Master
   * [05] Audio Group Track
   * [06] -- Folder Track
   * [07] ---- LSP Compressor Stereo
   * [08] Folder Track 1
   * [09] -- Audio FX Track
   * [10] -- Audio FX Track 1
   */

  audio_group = TRACKLIST->tracks[5];
  folder = TRACKLIST->tracks[6];
  Track * lsp_comp = TRACKLIST->tracks[7];
  folder2 = TRACKLIST->tracks[8];
  audio_fx = TRACKLIST->tracks[9];
  audio_fx2 = TRACKLIST->tracks[10];
  g_assert_cmpint (audio_group->pos, ==, 5);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (folder->pos, ==, 6);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (lsp_comp->pos, ==, 7);
  g_assert_cmpint (folder2->pos, ==, 8);
  g_assert_cmpint (folder2->size, ==, 3);
  g_assert_cmpint (audio_fx->pos, ==, 9);
  g_assert_cmpint (audio_fx2->pos, ==, 10);

  /* move folder 2 into folder 1 */
  track_select (
    folder2, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, folder, TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  /*
   * expect:
   * [00] Chords
   * [01] Tempo
   * [02] Modulators
   * [03] Markers
   * [04] Master
   * [05] Audio Group Track
   * [06] -- Folder Track
   * [08] ---- Folder Track 1
   * [09] ------ Audio FX Track
   * [10] ------ Audio FX Track 1
   * [07] ---- LSP Compressor Stereo
   */
  g_assert_cmpint (audio_group->pos, ==, 5);
  g_assert_cmpint (audio_group->size, ==, 6);
  g_assert_cmpint (folder->pos, ==, 6);
  g_assert_cmpint (folder->size, ==, 5);
  g_assert_cmpint (folder2->pos, ==, 7);
  g_assert_cmpint (folder2->size, ==, 3);
  g_assert_cmpint (audio_fx->pos, ==, 8);
  g_assert_cmpint (audio_fx2->pos, ==, 9);
  g_assert_cmpint (lsp_comp->pos, ==, 10);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (audio_group->pos, ==, 5);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (folder->pos, ==, 6);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (lsp_comp->pos, ==, 7);
  g_assert_cmpint (folder2->pos, ==, 8);
  g_assert_cmpint (folder2->size, ==, 3);
  g_assert_cmpint (audio_fx->pos, ==, 9);
  g_assert_cmpint (audio_fx2->pos, ==, 10);

  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (audio_group->pos, ==, 5);
  g_assert_cmpint (audio_group->size, ==, 6);
  g_assert_cmpint (folder->pos, ==, 6);
  g_assert_cmpint (folder->size, ==, 5);
  g_assert_cmpint (folder2->pos, ==, 7);
  g_assert_cmpint (folder2->size, ==, 3);
  g_assert_cmpint (audio_fx->pos, ==, 8);
  g_assert_cmpint (audio_fx2->pos, ==, 9);
  g_assert_cmpint (lsp_comp->pos, ==, 10);

  undo_manager_undo (UNDO_MANAGER, NULL);

  /*
   * [00] Chords
   * [01] Tempo
   * [02] Modulators
   * [03] Markers
   * [04] Master
   * [05] Audio Group Track
   * [06] -- Folder Track
   * [07] ---- LSP Compressor Stereo
   * [08] Folder Track 1
   * [09] -- Audio FX Track
   * [10] -- Audio FX Track 1
   */

  /* move audio fx 2 above master */
  track_select (
    audio_fx2, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, P_MASTER_TRACK, TRACK_WIDGET_HIGHLIGHT_TOP,
    GDK_ACTION_MOVE);
  g_assert_cmpint (audio_fx2->pos, ==, 4);
  g_assert_cmpint (audio_group->pos, ==, 6);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (folder->pos, ==, 7);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (lsp_comp->pos, ==, 8);
  g_assert_cmpint (folder2->pos, ==, 9);
  g_assert_cmpint (folder2->size, ==, 2);
  g_assert_cmpint (audio_fx->pos, ==, 10);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (audio_group->pos, ==, 5);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (folder->pos, ==, 6);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (lsp_comp->pos, ==, 7);
  g_assert_cmpint (folder2->pos, ==, 8);
  g_assert_cmpint (folder2->size, ==, 3);
  g_assert_cmpint (audio_fx->pos, ==, 9);
  g_assert_cmpint (audio_fx2->pos, ==, 10);

  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (audio_fx2->pos, ==, 4);
  g_assert_cmpint (audio_group->pos, ==, 6);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (folder->pos, ==, 7);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (lsp_comp->pos, ==, 8);
  g_assert_cmpint (folder2->pos, ==, 9);
  g_assert_cmpint (folder2->size, ==, 2);
  g_assert_cmpint (audio_fx->pos, ==, 10);

  undo_manager_undo (UNDO_MANAGER, NULL);

  /*
   * [00] Chords
   * [01] Tempo
   * [02] Modulators
   * [03] Markers
   * [04] Master
   * [05] Audio Group Track
   * [06] -- Folder Track
   * [07] ---- LSP Compressor Stereo
   * [08] Folder Track 1
   * [09] -- Audio FX Track
   * [10] -- Audio FX Track 1
   */

  /* move audio fx 2 to audio group */
  track_select (
    audio_fx2, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, audio_group, TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);
  g_assert_cmpint (audio_group->pos, ==, 5);
  g_assert_cmpint (audio_group->size, ==, 4);
  g_assert_cmpint (audio_fx2->pos, ==, 6);
  g_assert_cmpint (folder->pos, ==, 7);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (lsp_comp->pos, ==, 8);
  g_assert_cmpint (folder2->pos, ==, 9);
  g_assert_cmpint (folder2->size, ==, 2);
  g_assert_cmpint (audio_fx->pos, ==, 10);

  /* move audio fx 2 to folder */
  track_select (
    audio_fx2, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, folder, TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);
  g_assert_cmpint (audio_group->pos, ==, 5);
  g_assert_cmpint (audio_group->size, ==, 4);
  g_assert_cmpint (folder->pos, ==, 6);
  g_assert_cmpint (folder->size, ==, 3);
  g_assert_cmpint (audio_fx2->pos, ==, 7);
  g_assert_cmpint (lsp_comp->pos, ==, 8);
  g_assert_cmpint (folder2->pos, ==, 9);
  g_assert_cmpint (folder2->size, ==, 2);
  g_assert_cmpint (audio_fx->pos, ==, 10);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (audio_group->pos, ==, 5);
  g_assert_cmpint (audio_group->size, ==, 4);
  g_assert_cmpint (audio_fx2->pos, ==, 6);
  g_assert_cmpint (folder->pos, ==, 7);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (lsp_comp->pos, ==, 8);
  g_assert_cmpint (folder2->pos, ==, 9);
  g_assert_cmpint (folder2->size, ==, 2);
  g_assert_cmpint (audio_fx->pos, ==, 10);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (audio_group->pos, ==, 5);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (folder->pos, ==, 6);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (lsp_comp->pos, ==, 7);
  g_assert_cmpint (folder2->pos, ==, 8);
  g_assert_cmpint (folder2->size, ==, 3);
  g_assert_cmpint (audio_fx->pos, ==, 9);
  g_assert_cmpint (audio_fx2->pos, ==, 10);

  /*
   * [00] Chords
   * [01] Tempo
   * [02] Modulators
   * [03] Markers
   * [04] Master
   * [05] Audio Group Track
   * [06] -- Folder Track
   * [07] ---- LSP Compressor Stereo
   * [08] Folder Track 1
   * [09] -- Audio FX Track
   * [10] -- Audio FX Track 1
   */

  /* move folder inside itself */
  track_select (
    folder, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, folder, TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  /* move chanel-less track into folder and get
   * status */
  track_select (
    P_MARKER_TRACK, F_SELECT, F_EXCLUSIVE,
    F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, folder, TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);
  foldable_track_is_status (
    folder, FOLDABLE_TRACK_MIXER_STATUS_MUTED);
  undo_manager_undo (UNDO_MANAGER, NULL);

  /* delete a track inside the folder */
  track_select (
    audio_fx, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_selections_action_perform_delete (
    TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, NULL);
  g_assert_cmpint (audio_group->pos, ==, 5);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (folder->pos, ==, 6);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (lsp_comp->pos, ==, 7);
  g_assert_cmpint (folder2->pos, ==, 8);
  g_assert_cmpint (folder2->size, ==, 2);
  g_assert_cmpint (audio_fx2->pos, ==, 9);

  undo_manager_undo (UNDO_MANAGER, NULL);
  audio_fx = TRACKLIST->tracks[9];
  g_assert_cmpint (audio_group->pos, ==, 5);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (folder->pos, ==, 6);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (lsp_comp->pos, ==, 7);
  g_assert_cmpint (folder2->pos, ==, 8);
  g_assert_cmpint (folder2->size, ==, 3);
  g_assert_cmpint (audio_fx->pos, ==, 9);
  g_assert_cmpint (audio_fx2->pos, ==, 10);

  /*
   * [00] Chords
   * [01] Tempo
   * [02] Modulators
   * [03] Markers
   * [04] Master
   * [05] Audio Group Track
   * [06] -- Folder Track
   * [07] ---- LSP Compressor Stereo
   * [08] Folder Track 1
   * [09] -- Audio FX Track
   * [10] -- Audio FX Track 1
   */

  /* create a new track and drag it under
   * Folder Track 1 */
  Track * midi =
    track_create_empty_with_action (TRACK_TYPE_MIDI, NULL);
  g_assert_nonnull (midi);

  tracklist_handle_move_or_copy (
    TRACKLIST, audio_fx2, TRACK_WIDGET_HIGHLIGHT_TOP,
    GDK_ACTION_MOVE);

  g_assert_cmpint (audio_group->pos, ==, 5);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (folder->pos, ==, 6);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (lsp_comp->pos, ==, 7);
  g_assert_cmpint (folder2->pos, ==, 8);
  g_assert_cmpint (folder2->size, ==, 4);
  g_assert_cmpint (audio_fx->pos, ==, 9);
  g_assert_cmpint (midi->pos, ==, 10);
  g_assert_cmpint (audio_fx2->pos, ==, 11);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (audio_group->pos, ==, 5);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (folder->pos, ==, 6);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (lsp_comp->pos, ==, 7);
  g_assert_cmpint (folder2->pos, ==, 8);
  g_assert_cmpint (folder2->size, ==, 3);
  g_assert_cmpint (audio_fx->pos, ==, 9);
  g_assert_cmpint (audio_fx2->pos, ==, 10);

  test_helper_zrythm_cleanup ();
}

static void
test_move_inside (void)
{
  (void) _test_move_inside;
#ifdef HAVE_LSP_COMPRESSOR
  _test_move_inside (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false, false);
#endif
}

static void
test_move_multiple_inside (void)
{
  test_helper_zrythm_init ();

  /* create folder track */
  Track * folder =
    track_create_empty_with_action (TRACK_TYPE_FOLDER, NULL);

  /* create 2 audio fx tracks */
  Track * audio_fx1 = track_create_empty_with_action (
    TRACK_TYPE_AUDIO_BUS, NULL);
  Track * audio_fx2 = track_create_empty_with_action (
    TRACK_TYPE_AUDIO_BUS, NULL);

  /* move audio fx 1 inside folder */
  track_select (
    audio_fx1, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, folder, TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (folder->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (
    audio_fx1->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (
    audio_fx2->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (folder->size, ==, 1);
  g_assert_cmpint (folder->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (
    audio_fx1->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (
    audio_fx2->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (folder->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (
    audio_fx1->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (
    audio_fx2->pos, ==, TRACKLIST->num_tracks - 1);

  /* create audio group */
  Track * audio_group = track_create_empty_with_action (
    TRACK_TYPE_AUDIO_GROUP, NULL);

  /* move folder to audio group */
  track_select (
    folder, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    audio_fx1, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, audio_group, TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (
    audio_fx2->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (
    audio_group->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (folder->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (
    audio_fx1->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (audio_group->size, ==, 1);
  g_assert_cmpint (folder->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (
    audio_fx1->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (
    audio_fx2->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (
    audio_group->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (
    audio_fx2->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (
    audio_group->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (folder->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (
    audio_fx1->pos, ==, TRACKLIST->num_tracks - 1);

  /*
   * [0] Chords
   * [1] Tempo
   * [2] Modulators
   * [3] Markers
   * [4] Master
   * [5] Audio FX Track 1
   * [6] Audio Group Track
   * [7] -- Folder Track
   * [8] ---- Audio FX Track
   */

  /* create 2 new MIDI tracks and drag them above
   * Folder Track */
  Track * midi =
    track_create_empty_with_action (TRACK_TYPE_MIDI, NULL);
  g_assert_nonnull (midi);
  Track * midi2 =
    track_create_empty_with_action (TRACK_TYPE_MIDI, NULL);
  g_assert_nonnull (midi2);
  track_select (
    midi, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    midi2, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (audio_fx2->pos, ==, 5);
  g_assert_cmpint (audio_group->pos, ==, 6);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (folder->pos, ==, 7);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (audio_fx1->pos, ==, 8);
  g_assert_cmpint (midi->pos, ==, 9);
  g_assert_cmpint (midi2->pos, ==, 10);

  tracklist_handle_move_or_copy (
    TRACKLIST, folder, TRACK_WIDGET_HIGHLIGHT_TOP,
    GDK_ACTION_MOVE);

  tracklist_print_tracks (TRACKLIST);

  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (audio_fx2->pos, ==, 5);
  g_assert_cmpint (audio_group->pos, ==, 6);
  g_assert_cmpint (audio_group->size, ==, 5);
  g_assert_cmpint (midi->pos, ==, 7);
  g_assert_cmpint (midi2->pos, ==, 8);
  g_assert_cmpint (folder->pos, ==, 9);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (audio_fx1->pos, ==, 10);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (audio_fx2->pos, ==, 5);
  g_assert_cmpint (audio_group->pos, ==, 6);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (folder->pos, ==, 7);
  g_assert_cmpint (audio_fx1->pos, ==, 8);
  g_assert_cmpint (midi->pos, ==, 9);
  g_assert_cmpint (midi2->pos, ==, 10);

  test_helper_zrythm_cleanup ();
}

static void
test_copy_multiple_inside (void)
{
  test_helper_zrythm_init ();

  /* create folder track */
  Track * folder =
    track_create_empty_with_action (TRACK_TYPE_FOLDER, NULL);

  /* create 2 audio fx tracks */
  Track * audio_fx1 = track_create_empty_with_action (
    TRACK_TYPE_AUDIO_BUS, NULL);
  Track * audio_fx2 = track_create_empty_with_action (
    TRACK_TYPE_AUDIO_BUS, NULL);

  /* move audio fx 1 inside folder */
  track_select (
    audio_fx1, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, folder, TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  /* create audio group */
  Track * audio_group = track_create_empty_with_action (
    TRACK_TYPE_AUDIO_GROUP, NULL);

  /* move folder to audio group */
  track_select (
    folder, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    audio_fx1, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, audio_group, TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_MOVE);

  /* create new folder */
  Track * folder2 =
    track_create_empty_with_action (TRACK_TYPE_FOLDER, NULL);

  /* copy-move audio group inside new folder */
  track_select (
    audio_group, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    folder, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    audio_fx1, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, folder2, TRACK_WIDGET_HIGHLIGHT_INSIDE,
    GDK_ACTION_COPY);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (
    audio_fx2->pos, ==, TRACKLIST->num_tracks - 8);
  g_assert_cmpint (
    audio_group->pos, ==, TRACKLIST->num_tracks - 7);
  g_assert_cmpint (folder->pos, ==, TRACKLIST->num_tracks - 6);
  g_assert_cmpint (
    audio_fx1->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (
    folder2->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (folder2->size, ==, 4);
  g_assert_true (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 3]->type
    == TRACK_TYPE_AUDIO_GROUP);
  g_assert_true (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 2]->type
    == TRACK_TYPE_FOLDER);
  g_assert_true (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1]->type
    == TRACK_TYPE_AUDIO_BUS);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (
    audio_fx2->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (
    audio_group->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (folder->pos, ==, TRACKLIST->num_tracks - 3);
  g_assert_cmpint (
    audio_fx1->pos, ==, TRACKLIST->num_tracks - 2);
  g_assert_cmpint (
    folder2->pos, ==, TRACKLIST->num_tracks - 1);

  undo_manager_redo (UNDO_MANAGER, NULL);
  g_assert_cmpint (folder->size, ==, 2);
  g_assert_cmpint (audio_group->size, ==, 3);
  g_assert_cmpint (
    audio_fx2->pos, ==, TRACKLIST->num_tracks - 8);
  g_assert_cmpint (
    audio_group->pos, ==, TRACKLIST->num_tracks - 7);
  g_assert_cmpint (folder->pos, ==, TRACKLIST->num_tracks - 6);
  g_assert_cmpint (
    audio_fx1->pos, ==, TRACKLIST->num_tracks - 5);
  g_assert_cmpint (
    folder2->pos, ==, TRACKLIST->num_tracks - 4);
  g_assert_cmpint (folder2->size, ==, 4);
  g_assert_true (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 3]->type
    == TRACK_TYPE_AUDIO_GROUP);
  g_assert_true (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 2]->type
    == TRACK_TYPE_FOLDER);
  g_assert_true (
    TRACKLIST->tracks[TRACKLIST->num_tracks - 1]->type
    == TRACK_TYPE_AUDIO_BUS);

  /*
   * [0] Chords
   * [1] Tempo
   * [2] Modulators
   * [3] Markers
   * [4] Master
   * [5] Audio FX Track 1
   * [6] Audio Group Track
   * [7] -- Folder Track
   * [8] ---- Audio FX Track
   * [009] Folder Track 1
   * [010] -- Audio Group Track 1
   * [011] ---- Folder Track 2
   * [012] ------ Audio FX Track 2
   */

  Track * audio_group2 = TRACKLIST->tracks[10];
  Track * folder3 = TRACKLIST->tracks[11];
  Track * audio_fx3 = TRACKLIST->tracks[12];

  /* create 2 new MIDI tracks and copy-drag them
   * above Audio Group Track 1 */
  Track * midi =
    track_create_empty_with_action (TRACK_TYPE_MIDI, NULL);
  g_assert_nonnull (midi);
  Track * midi2 =
    track_create_empty_with_action (TRACK_TYPE_MIDI, NULL);
  g_assert_nonnull (midi2);

  track_select (
    midi, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    midi2, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, audio_group2, TRACK_WIDGET_HIGHLIGHT_TOP,
    GDK_ACTION_COPY);

  /*
   * [0] Chords
   * [1] Tempo
   * [2] Modulators
   * [3] Markers
   * [4] Master
   * [5] Audio FX Track 1
   * [6] Audio Group Track
   * [7] -- Folder Track
   * [8] ---- Audio FX Track
   * [009] Folder Track 1
   * [010] -- MIDI Track 2
   * [011] -- MIDI Track 3
   * [012] -- Audio Group Track 1
   * [013] ---- Folder Track 2
   * [014] ------ Audio FX Track 2
   * [015] -- MIDI Track
   * [016] -- MIDI Track 1
   */

  tracklist_print_tracks (TRACKLIST);

  Track * midi3 = TRACKLIST->tracks[10];
  Track * midi4 = TRACKLIST->tracks[11];
  g_assert_cmpint (folder2->pos, ==, 9);
  g_assert_cmpint (folder2->size, ==, 6);
  g_assert_cmpint (midi3->pos, ==, 10);
  g_assert_cmpint (midi4->pos, ==, 11);
  g_assert_cmpint (audio_group2->pos, ==, 12);
  g_assert_cmpint (audio_group2->size, ==, 3);
  g_assert_cmpint (folder3->pos, ==, 13);
  g_assert_cmpint (folder3->size, ==, 2);
  g_assert_cmpint (audio_fx3->pos, ==, 14);
  g_assert_cmpint (midi->pos, ==, 15);
  g_assert_cmpint (midi2->pos, ==, 16);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (folder2->pos, ==, 9);
  g_assert_cmpint (folder2->size, ==, 4);
  g_assert_cmpint (audio_group2->pos, ==, 10);
  g_assert_cmpint (audio_group2->size, ==, 3);
  g_assert_cmpint (folder3->pos, ==, 11);
  g_assert_cmpint (folder3->size, ==, 2);
  g_assert_cmpint (audio_fx3->pos, ==, 12);
  g_assert_cmpint (midi->pos, ==, 13);
  g_assert_cmpint (midi2->pos, ==, 14);

  undo_manager_redo (UNDO_MANAGER, NULL);
  midi3 = TRACKLIST->tracks[10];
  midi4 = TRACKLIST->tracks[11];
  g_assert_cmpint (folder2->pos, ==, 9);
  g_assert_cmpint (folder2->size, ==, 6);
  g_assert_cmpint (midi3->pos, ==, 10);
  g_assert_cmpint (midi4->pos, ==, 11);
  g_assert_cmpint (audio_group2->pos, ==, 12);
  g_assert_cmpint (audio_group2->size, ==, 3);
  g_assert_cmpint (folder3->pos, ==, 13);
  g_assert_cmpint (folder3->size, ==, 2);
  g_assert_cmpint (audio_fx3->pos, ==, 14);
  g_assert_cmpint (midi->pos, ==, 15);
  g_assert_cmpint (midi2->pos, ==, 16);

  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (folder2->pos, ==, 9);
  g_assert_cmpint (folder2->size, ==, 4);
  g_assert_cmpint (audio_group2->pos, ==, 10);
  g_assert_cmpint (audio_group2->size, ==, 3);
  g_assert_cmpint (folder3->pos, ==, 11);
  g_assert_cmpint (folder3->size, ==, 2);
  g_assert_cmpint (audio_fx3->pos, ==, 12);
  g_assert_cmpint (midi->pos, ==, 13);
  g_assert_cmpint (midi2->pos, ==, 14);

  /*
   * [0] Chords
   * [1] Tempo
   * [2] Modulators
   * [3] Markers
   * [4] Master
   * [5] Audio FX Track 1
   * [6] Audio Group Track
   * [7] -- Folder Track
   * [8] ---- Audio FX Track
   * [009] Folder Track 1
   * [010] -- Audio Group Track 1
   * [011] ---- Folder Track 2
   * [012] ------ Audio FX Track 2
   * [013] MIDI Track
   * [014] MIDI Track 1
   */

  /* move MIDI tracks to Folder Track 2 */
  track_select (
    midi, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    midi2, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  g_assert_cmpint (TRACKLIST_SELECTIONS->num_tracks, ==, 2);
  tracklist_handle_move_or_copy (
    TRACKLIST, folder3, TRACK_WIDGET_HIGHLIGHT_BOTTOM,
    GDK_ACTION_MOVE);

  g_assert_cmpint (folder2->pos, ==, 9);
  g_assert_cmpint (folder2->size, ==, 6);
  g_assert_cmpint (audio_group2->pos, ==, 10);
  g_assert_cmpint (audio_group2->size, ==, 5);
  g_assert_cmpint (folder3->pos, ==, 11);
  g_assert_cmpint (folder3->size, ==, 4);
  g_assert_cmpint (midi->pos, ==, 12);
  g_assert_cmpint (midi2->pos, ==, 13);
  g_assert_cmpint (audio_fx3->pos, ==, 14);

  /* clone the midi tracks at the end */
  track_select (
    midi, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    midi2, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, audio_fx3, TRACK_WIDGET_HIGHLIGHT_BOTTOM,
    GDK_ACTION_COPY);

  midi3 = TRACKLIST->tracks[15];
  midi4 = TRACKLIST->tracks[16];
  g_assert_cmpint (folder2->pos, ==, 9);
  g_assert_cmpint (folder2->size, ==, 6);
  g_assert_cmpint (audio_group2->pos, ==, 10);
  g_assert_cmpint (audio_group2->size, ==, 5);
  g_assert_cmpint (folder3->pos, ==, 11);
  g_assert_cmpint (folder3->size, ==, 4);
  g_assert_cmpint (midi->pos, ==, 12);
  g_assert_cmpint (midi2->pos, ==, 13);
  g_assert_cmpint (audio_fx3->pos, ==, 14);
  g_assert_cmpint (midi3->pos, ==, 15);
  g_assert_cmpint (midi4->pos, ==, 16);

  /*
   * [000] Chords
   * [001] Tempo
   * [002] Modulators
   * [003] Markers
   * [004] Master
   * [005] Audio FX Track 1
   * [006] Audio Group Track
   * [007] -- Folder Track
   * [008] ---- Audio FX Track
   * [009] Folder Track 1
   * [010] -- Audio Group Track 1
   * [011] ---- Folder Track 2
   * [012] ------ MIDI Track
   * [013] ------ MIDI Track 1
   * [014] ------ Audio FX Track 2
   * [015] MIDI Track 2
   * [016] MIDI Track 3
   */

  /* move MIDI tracks below master */
  track_select (
    midi3, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    midi4, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, P_MASTER_TRACK, TRACK_WIDGET_HIGHLIGHT_BOTTOM,
    GDK_ACTION_MOVE);

  /* copy MIDI tracks below MIDI Track 1 */
  track_select (
    midi3, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    midi4, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, midi2, TRACK_WIDGET_HIGHLIGHT_BOTTOM,
    GDK_ACTION_COPY);

  Track * midi5 = TRACKLIST->tracks[16];
  Track * midi6 = TRACKLIST->tracks[17];
  g_assert_cmpint (folder2->pos, ==, 11);
  g_assert_cmpint (folder2->size, ==, 8);
  g_assert_cmpint (audio_group2->pos, ==, 12);
  g_assert_cmpint (audio_group2->size, ==, 7);
  g_assert_cmpint (folder3->pos, ==, 13);
  g_assert_cmpint (folder3->size, ==, 6);
  g_assert_cmpint (midi->pos, ==, 14);
  g_assert_cmpint (midi2->pos, ==, 15);
  g_assert_cmpint (midi5->pos, ==, 16);
  g_assert_cmpint (midi6->pos, ==, 17);
  g_assert_cmpint (audio_fx3->pos, ==, 18);

  /* undo */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (folder2->pos, ==, 11);
  g_assert_cmpint (folder2->size, ==, 6);
  g_assert_cmpint (audio_group2->pos, ==, 12);
  g_assert_cmpint (audio_group2->size, ==, 5);
  g_assert_cmpint (folder3->pos, ==, 13);
  g_assert_cmpint (folder3->size, ==, 4);
  g_assert_cmpint (midi->pos, ==, 14);
  g_assert_cmpint (midi2->pos, ==, 15);
  g_assert_cmpint (audio_fx3->pos, ==, 16);

  /* move MIDI Track 3 at the end */
  track_select (
    midi4, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, audio_fx3, TRACK_WIDGET_HIGHLIGHT_BOTTOM,
    GDK_ACTION_MOVE);

  tracklist_print_tracks (TRACKLIST);

  g_assert_cmpint (folder2->pos, ==, 10);
  g_assert_cmpint (folder2->size, ==, 6);
  g_assert_cmpint (audio_group2->pos, ==, 11);
  g_assert_cmpint (audio_group2->size, ==, 5);
  g_assert_cmpint (folder3->pos, ==, 12);
  g_assert_cmpint (folder3->size, ==, 4);
  g_assert_cmpint (midi->pos, ==, 13);
  g_assert_cmpint (midi2->pos, ==, 14);
  g_assert_cmpint (audio_fx3->pos, ==, 15);
  g_assert_cmpint (midi4->pos, ==, 16);

  /* copy MIDI Track 3 and MIDI Track 2 below
   * MIDI Track 1 */
  track_select (
    midi3, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    midi4, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, midi2, TRACK_WIDGET_HIGHLIGHT_BOTTOM,
    GDK_ACTION_COPY);

  tracklist_print_tracks (TRACKLIST);

  midi5 = TRACKLIST->tracks[15];
  midi6 = TRACKLIST->tracks[16];
  g_assert_cmpint (folder2->pos, ==, 10);
  g_assert_cmpint (folder2->size, ==, 8);
  g_assert_cmpint (audio_group2->pos, ==, 11);
  g_assert_cmpint (audio_group2->size, ==, 7);
  g_assert_cmpint (folder3->pos, ==, 12);
  g_assert_cmpint (folder3->size, ==, 6);
  g_assert_cmpint (midi->pos, ==, 13);
  g_assert_cmpint (midi2->pos, ==, 14);
  g_assert_cmpint (midi5->pos, ==, 15);
  g_assert_cmpint (midi6->pos, ==, 16);
  g_assert_cmpint (audio_fx3->pos, ==, 17);
  g_assert_cmpint (midi4->pos, ==, 18);
  g_assert_cmpint (TRACKLIST->num_tracks, ==, 19);

  /* undo */
  undo_manager_undo (UNDO_MANAGER, NULL);
  g_assert_cmpint (folder2->pos, ==, 10);
  g_assert_cmpint (folder2->size, ==, 6);
  g_assert_cmpint (audio_group2->pos, ==, 11);
  g_assert_cmpint (audio_group2->size, ==, 5);
  g_assert_cmpint (folder3->pos, ==, 12);
  g_assert_cmpint (folder3->size, ==, 4);
  g_assert_cmpint (midi->pos, ==, 13);
  g_assert_cmpint (midi2->pos, ==, 14);
  g_assert_cmpint (audio_fx3->pos, ==, 15);
  g_assert_cmpint (midi4->pos, ==, 16);
  g_assert_cmpint (TRACKLIST->num_tracks, ==, 17);

  tracklist_print_tracks (TRACKLIST);

  /* move MIDI Track 3 and MIDI Track 2 below
   * MIDI Track 1 */
  track_select (
    midi3, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  track_select (
    midi4, F_SELECT, F_NOT_EXCLUSIVE, F_NO_PUBLISH_EVENTS);
  tracklist_handle_move_or_copy (
    TRACKLIST, midi2, TRACK_WIDGET_HIGHLIGHT_BOTTOM,
    GDK_ACTION_MOVE);

  tracklist_print_tracks (TRACKLIST);

  g_assert_cmpint (folder2->pos, ==, 9);
  g_assert_cmpint (folder2->size, ==, 8);
  g_assert_cmpint (audio_group2->pos, ==, 10);
  g_assert_cmpint (audio_group2->size, ==, 7);
  g_assert_cmpint (folder3->pos, ==, 11);
  g_assert_cmpint (folder3->size, ==, 6);
  g_assert_cmpint (midi->pos, ==, 12);
  g_assert_cmpint (midi2->pos, ==, 13);
  g_assert_cmpint (midi3->pos, ==, 14);
  g_assert_cmpint (midi4->pos, ==, 15);
  g_assert_cmpint (audio_fx3->pos, ==, 16);
  g_assert_cmpint (TRACKLIST->num_tracks, ==, 17);

  test_helper_zrythm_cleanup ();
}

static void
test_copy_after_uninstalling_plugin (void)
{
#ifdef HAVE_HELM
  test_helper_zrythm_init ();

  bool with_carla = false;
  test_plugin_manager_create_tracks_from_plugin (
    HELM_BUNDLE, HELM_URI, true, with_carla, 1);

  Track * helm_track = tracklist_get_last_track (
    TRACKLIST, TRACKLIST_PIN_OPTION_BOTH, false);
  track_select (
    helm_track, F_SELECT, F_EXCLUSIVE, F_NO_PUBLISH_EVENTS);

  /* unload bundle */
  test_plugin_manager_reload_lilv_world_w_path ("/tmp");

  for (int i = 0; i < 2; i++)
    {
      /* expect failure */
      GError * err = NULL;
      bool     ret = false;
      switch (i)
        {
        case 0:
          ret = tracklist_selections_action_perform_copy (
            TRACKLIST_SELECTIONS,

            PORT_CONNECTIONS_MGR, TRACKLIST->num_tracks, &err);
          break;
        case 1:
          ret = tracklist_selections_action_perform_delete (
            TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, &err);
          break;
        }
      g_assert_false (ret);
      g_assert_nonnull (err);
      g_error_free_and_null (err);
    }

  test_helper_zrythm_cleanup ();
#endif
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/actions/tracklist_selections/"

  g_test_add_func (
    TEST_PREFIX "test no visible tracks after track deletion",
    (GTestFunc) test_no_visible_tracks_after_track_deletion);
  g_test_add_func (
    TEST_PREFIX "test track deletion with lv2 worker",
    (GTestFunc) test_track_deletion_with_lv2_worker);
  g_test_add_func (
    TEST_PREFIX "test track deletion w mixer selections",
    (GTestFunc) test_track_deletion_w_mixer_selections);
  g_test_add_func (
    TEST_PREFIX
    "test port and plugin track pos after duplication",
    (GTestFunc)
      test_port_and_plugin_track_pos_after_duplication);
  g_test_add_func (
    TEST_PREFIX "test ins track duplicate w send",
    (GTestFunc) test_ins_track_duplicate_w_send);
  g_test_add_func (
    TEST_PREFIX "test delete track w midi file",
    (GTestFunc) test_delete_track_w_midi_file);
  g_test_add_func (
    TEST_PREFIX "test ins track deletion with automation",
    (GTestFunc) test_ins_track_deletion_w_automation);
  g_test_add_func (
    TEST_PREFIX "test group track deletion",
    (GTestFunc) test_group_track_deletion);
  g_test_add_func (
    TEST_PREFIX "test move inside",
    (GTestFunc) test_move_inside);
  g_test_add_func (
    TEST_PREFIX "test copy multiple inside",
    (GTestFunc) test_copy_multiple_inside);
  g_test_add_func (
    TEST_PREFIX "test move multiple inside",
    (GTestFunc) test_move_multiple_inside);
#ifdef HAVE_CARLA
  g_test_add_func (
    TEST_PREFIX
    "test port and plugin track pos after duplication with carla",
    (GTestFunc)
      test_port_and_plugin_track_pos_after_duplication_with_carla);
#endif
  g_test_add_func (
    TEST_PREFIX "test_move_tracks",
    (GTestFunc) test_move_tracks);
  g_test_add_func (
    TEST_PREFIX "test duplicate w output and send",
    (GTestFunc) test_duplicate_w_output_and_send);
  g_test_add_func (
    TEST_PREFIX "test source track deletion with sends",
    (GTestFunc) test_source_track_deletion_with_sends);
#if 0
  /* uninstalling does not work with carla */
  g_test_add_func (
    TEST_PREFIX "test copy after uninstalling plugin",
    (GTestFunc) test_copy_after_uninstalling_plugin);
#endif
  g_test_add_func (
    TEST_PREFIX "test move multiple tracks",
    (GTestFunc) test_move_multiple_tracks);
  g_test_add_func (
    TEST_PREFIX "test multi track duplicate",
    (GTestFunc) test_multi_track_duplicate);
  g_test_add_func (
    TEST_PREFIX "test create midi fx track",
    (GTestFunc) test_create_midi_fx_track);
  g_test_add_func (
    TEST_PREFIX "test target track deletion with sends",
    (GTestFunc) test_target_track_deletion_with_sends);
  g_test_add_func (
    TEST_PREFIX "test marker track unpin",
    (GTestFunc) test_marker_track_unpin);
  g_test_add_func (
    TEST_PREFIX "test create from midi file",
    (GTestFunc) test_create_from_midi_file);
  g_test_add_func (
    TEST_PREFIX
    "test create instrument when redo stack "
    "non-empty",
    (GTestFunc) test_create_ins_when_redo_stack_nonempty);
  g_test_add_func (
    TEST_PREFIX "test undo track deletion",
    (GTestFunc) test_undo_track_deletion);
  g_test_add_func (
    TEST_PREFIX "test audio track deletion",
    (GTestFunc) test_audio_track_deletion);

  (void) test_copy_after_uninstalling_plugin;

  return g_test_run ();
}
