// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_selections.h"
#include "actions/channel_send_action.h"
#include "actions/chord_action.h"
#include "actions/midi_mapping_action.h"
#include "actions/mixer_selections_action.h"
#include "actions/port_action.h"
#include "actions/port_connection_action.h"
#include "actions/range_action.h"
#include "actions/tracklist_selections.h"
#include "actions/transport_action.h"
#include "dsp/channel_send.h"
#include "io/serialization/actions.h"
#include "io/serialization/arranger_objects.h"
#include "io/serialization/channel.h"
#include "io/serialization/engine.h"
#include "io/serialization/extra.h"
#include "io/serialization/gui_backend.h"
#include "io/serialization/plugin.h"
#include "io/serialization/port.h"
#include "io/serialization/selections.h"
#include "io/serialization/track.h"

#include <yyjson.h>

typedef enum
{
  Z_IO_SERIALIZATION_ACTIONS_ERROR_FAILED,
} ZIOSerializationActionsError;

#define Z_IO_SERIALIZATION_ACTIONS_ERROR \
  z_io_serialization_actions_error_quark ()
GQuark
z_io_serialization_actions_error_quark (void);
G_DEFINE_QUARK (z - io - serialization - actions - error - quark, z_io_serialization_actions_error)

bool
undoable_action_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       action_obj,
  const UndoableAction * action,
  GError **              error)
{
  yyjson_mut_obj_add_int (doc, action_obj, "type", action->type);
  yyjson_mut_obj_add_real (
    doc, action_obj, "framesPerTick", action->frames_per_tick);
  yyjson_mut_obj_add_int (doc, action_obj, "sampleRate", action->sample_rate);
  yyjson_mut_obj_add_int (doc, action_obj, "stackIndex", action->stack_idx);
  yyjson_mut_obj_add_int (doc, action_obj, "numActions", action->num_actions);
  return true;
}

bool
arranger_selections_action_serialize_to_json (
  yyjson_mut_doc *                 doc,
  yyjson_mut_val *                 action_obj,
  const ArrangerSelectionsAction * action,
  GError **                        error)
{
  yyjson_mut_val * parent_instance_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "base");
  undoable_action_serialize_to_json (
    doc, parent_instance_obj, &action->parent_instance, error);
  yyjson_mut_obj_add_int (doc, action_obj, "type", action->type);
  yyjson_mut_obj_add_int (doc, action_obj, "editType", action->edit_type);
  yyjson_mut_obj_add_int (doc, action_obj, "resizeType", action->resize_type);
  yyjson_mut_obj_add_real (doc, action_obj, "ticks", action->ticks);
  yyjson_mut_obj_add_int (doc, action_obj, "deltaTracks", action->delta_tracks);
  yyjson_mut_obj_add_int (doc, action_obj, "deltaLanes", action->delta_lanes);
  yyjson_mut_obj_add_int (doc, action_obj, "deltaChords", action->delta_chords);
  yyjson_mut_obj_add_int (doc, action_obj, "deltaPitch", action->delta_pitch);
  yyjson_mut_obj_add_int (doc, action_obj, "deltaVel", action->delta_vel);
  yyjson_mut_obj_add_real (
    doc, action_obj, "deltaNormalizedAmount", action->delta_normalized_amount);
  if (action->target_port)
    {
      yyjson_mut_val * target_port_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "targetPort");
      port_identifier_serialize_to_json (
        doc, target_port_obj, action->target_port, error);
    }
  if (action->str)
    {
      yyjson_mut_obj_add_str (doc, action_obj, "str", action->str);
    }
  yyjson_mut_val * pos_obj = yyjson_mut_obj_add_obj (doc, action_obj, "pos");
  position_serialize_to_json (doc, pos_obj, &action->pos, error);
  if (action->opts)
    {
      yyjson_mut_val * opts_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "quantizeOptions");
      quantize_options_serialize_to_json (doc, opts_obj, action->opts, error);
    }
  if (action->chord_sel)
    {
      yyjson_mut_val * chord_sel_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "chordSelections");
      chord_selections_serialize_to_json (
        doc, chord_sel_obj, action->chord_sel, error);
    }
  if (action->tl_sel)
    {
      yyjson_mut_val * tl_sel_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "timelineSelections");
      timeline_selections_serialize_to_json (
        doc, tl_sel_obj, action->tl_sel, error);
    }
  if (action->ma_sel)
    {
      yyjson_mut_val * ma_sel_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "midiArrangerSelections");
      midi_arranger_selections_serialize_to_json (
        doc, ma_sel_obj, action->ma_sel, error);
    }
  if (action->automation_sel)
    {
      yyjson_mut_val * automation_sel_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "automationSelections");
      automation_selections_serialize_to_json (
        doc, automation_sel_obj, action->automation_sel, error);
    }
  if (action->audio_sel)
    {
      yyjson_mut_val * audio_sel_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "audioSelections");
      audio_selections_serialize_to_json (
        doc, audio_sel_obj, action->audio_sel, error);
    }
  if (action->chord_sel_after)
    {
      yyjson_mut_val * chord_sel_after_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "chordSelectionsAfter");
      chord_selections_serialize_to_json (
        doc, chord_sel_after_obj, action->chord_sel_after, error);
    }
  if (action->tl_sel_after)
    {
      yyjson_mut_val * tl_sel_after_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "timelineSelectionsAfter");
      timeline_selections_serialize_to_json (
        doc, tl_sel_after_obj, action->tl_sel_after, error);
    }
  if (action->ma_sel_after)
    {
      yyjson_mut_val * ma_sel_after_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "midiArrangerSelectionsAfter");
      midi_arranger_selections_serialize_to_json (
        doc, ma_sel_after_obj, action->ma_sel_after, error);
    }
  if (action->automation_sel_after)
    {
      yyjson_mut_val * automation_sel_after_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "automationSelectionsAfter");
      automation_selections_serialize_to_json (
        doc, automation_sel_after_obj, action->automation_sel_after, error);
    }
  if (action->audio_sel_after)
    {
      yyjson_mut_val * audio_sel_after_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "audioSelectionsAfter");
      audio_selections_serialize_to_json (
        doc, audio_sel_after_obj, action->audio_sel_after, error);
    }
  if (action->region_before)
    {
      yyjson_mut_val * region_before_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "regionBefore");
      region_serialize_to_json (
        doc, region_before_obj, action->region_before, error);
    }
  if (action->region_after)
    {
      yyjson_mut_val * region_after_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "regionAfter");
      region_serialize_to_json (
        doc, region_after_obj, action->region_after, error);
    }
  yyjson_mut_val * region_r1_arr =
    yyjson_mut_obj_add_arr (doc, action_obj, "regionR1");
  yyjson_mut_val * region_r2_arr =
    yyjson_mut_obj_add_arr (doc, action_obj, "regionR2");
  yyjson_mut_val * midi_note_r1_arr =
    yyjson_mut_obj_add_arr (doc, action_obj, "midiNoteR1");
  yyjson_mut_val * midi_note_r2_arr =
    yyjson_mut_obj_add_arr (doc, action_obj, "midiNoteR2");
  for (int i = 0; i < action->num_split_objs; i++)
    {
      if (action->region_r1[i])
        {
          yyjson_mut_val * region_r1_obj =
            yyjson_mut_arr_add_obj (doc, region_r1_arr);
          region_serialize_to_json (
            doc, region_r1_obj, action->region_r1[i], error);
          yyjson_mut_val * region_r2_obj =
            yyjson_mut_arr_add_obj (doc, region_r2_arr);
          region_serialize_to_json (
            doc, region_r2_obj, action->region_r2[i], error);
        }
      if (action->mn_r1[i])
        {
          yyjson_mut_val * mn_r1_obj =
            yyjson_mut_arr_add_obj (doc, midi_note_r1_arr);
          midi_note_serialize_to_json (doc, mn_r1_obj, action->mn_r1[i], error);
          yyjson_mut_val * mn_r2_obj =
            yyjson_mut_arr_add_obj (doc, midi_note_r2_arr);
          midi_note_serialize_to_json (doc, mn_r2_obj, action->mn_r2[i], error);
        }
    }
  return true;
}

bool
mixer_selections_action_serialize_to_json (
  yyjson_mut_doc *              doc,
  yyjson_mut_val *              action_obj,
  const MixerSelectionsAction * action,
  GError **                     error)
{
  yyjson_mut_val * parent_instance_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "base");
  undoable_action_serialize_to_json (
    doc, parent_instance_obj, &action->parent_instance, error);
  yyjson_mut_obj_add_int (doc, action_obj, "type", action->type);
  yyjson_mut_obj_add_int (doc, action_obj, "slotType", action->slot_type);
  yyjson_mut_obj_add_int (doc, action_obj, "toSlot", action->to_slot);
  yyjson_mut_obj_add_uint (
    doc, action_obj, "toTrackNameHash", action->to_track_name_hash);
  yyjson_mut_obj_add_int (doc, action_obj, "newChannel", action->new_channel);
  yyjson_mut_obj_add_int (doc, action_obj, "num_plugins", action->num_plugins);
  if (action->setting)
    {
      yyjson_mut_val * setting_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "setting");
      plugin_setting_serialize_to_json (
        doc, setting_obj, action->setting, error);
    }
  if (action->ms_before)
    {
      yyjson_mut_val * ms_before_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "mixerSelectionsBefore");
      mixer_selections_serialize_to_json (
        doc, ms_before_obj, action->ms_before, error);
    }
  if (action->deleted_ms)
    {
      yyjson_mut_val * deleted_ms_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "deletedSelections");
      mixer_selections_serialize_to_json (
        doc, deleted_ms_obj, action->deleted_ms, error);
    }
  yyjson_mut_val * ats_arr =
    yyjson_mut_obj_add_arr (doc, action_obj, "automationTracks");
  for (int i = 0; i < action->num_ats; i++)
    {
      AutomationTrack * at = action->ats[i];
      yyjson_mut_val *  at_obj = yyjson_mut_arr_add_obj (doc, ats_arr);
      automation_track_serialize_to_json (doc, at_obj, at, error);
    }
  yyjson_mut_val * deleted_ats_arr =
    yyjson_mut_obj_add_arr (doc, action_obj, "deletedAutomationTracks");
  for (int i = 0; i < action->num_deleted_ats; i++)
    {
      AutomationTrack * at = action->deleted_ats[i];
      yyjson_mut_val *  at_obj = yyjson_mut_arr_add_obj (doc, deleted_ats_arr);
      automation_track_serialize_to_json (doc, at_obj, at, error);
    }
  if (action->connections_mgr_before)
    {
      yyjson_mut_val * connections_mgr_before_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "connectionsManagerBefore");
      port_connections_manager_serialize_to_json (
        doc, connections_mgr_before_obj, action->connections_mgr_before, error);
    }
  if (action->connections_mgr_after)
    {
      yyjson_mut_val * connections_mgr_after_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "connectionsManagerAfter");
      port_connections_manager_serialize_to_json (
        doc, connections_mgr_after_obj, action->connections_mgr_after, error);
    }
  return true;
}

bool
tracklist_selections_action_serialize_to_json (
  yyjson_mut_doc *                  doc,
  yyjson_mut_val *                  action_obj,
  const TracklistSelectionsAction * action,
  GError **                         error)
{
  yyjson_mut_val * parent_instance_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "base");
  undoable_action_serialize_to_json (
    doc, parent_instance_obj, &action->parent_instance, error);
  yyjson_mut_obj_add_int (doc, action_obj, "type", action->type);
  yyjson_mut_obj_add_int (doc, action_obj, "trackType", action->track_type);
  if (action->pl_setting)
    {
      yyjson_mut_val * plugin_setting_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "pluginSetting");
      plugin_setting_serialize_to_json (
        doc, plugin_setting_obj, action->pl_setting, error);
    }
  yyjson_mut_obj_add_bool (doc, action_obj, "isEmpty", action->is_empty);
  yyjson_mut_obj_add_int (doc, action_obj, "trackPos", action->track_pos);
  yyjson_mut_obj_add_int (doc, action_obj, "lanePos", action->lane_pos);
  yyjson_mut_obj_add_bool (doc, action_obj, "havePos", action->have_pos);
  yyjson_mut_val * position_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "pos");
  position_serialize_to_json (doc, position_obj, &action->pos, error);
  yyjson_mut_val * tracks_before_arr =
    yyjson_mut_obj_add_arr (doc, action_obj, "tracksBefore");
  yyjson_mut_val * tracks_after_arr =
    yyjson_mut_obj_add_arr (doc, action_obj, "tracksAfter");
  for (int i = 0; i < action->num_tracks; i++)
    {
      yyjson_mut_arr_add_int (doc, tracks_before_arr, action->tracks_before[i]);
      yyjson_mut_arr_add_int (doc, tracks_after_arr, action->tracks_after[i]);
    }
  yyjson_mut_obj_add_int (doc, action_obj, "numTracks", action->num_tracks);
  if (action->file_basename)
    {
      yyjson_mut_obj_add_str (
        doc, action_obj, "fileBasename", action->file_basename);
    }
  if (action->base64_midi)
    {
      yyjson_mut_obj_add_str (
        doc, action_obj, "base64Midi", action->base64_midi);
    }
  yyjson_mut_obj_add_int (doc, action_obj, "poolId", action->pool_id);
  if (action->tls_before)
    {
      yyjson_mut_val * tls_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "tracklistSelectionsBefore");
      tracklist_selections_serialize_to_json (
        doc, tls_obj, action->tls_before, error);
    }
  if (action->tls_after)
    {
      yyjson_mut_val * tls_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "tracklistSelectionsAfter");
      tracklist_selections_serialize_to_json (
        doc, tls_obj, action->tls_after, error);
    }
  if (action->foldable_tls_before)
    {
      yyjson_mut_val * foldable_tls_obj = yyjson_mut_obj_add_obj (
        doc, action_obj, "foldableTracklistSelectionsBefore");
      tracklist_selections_serialize_to_json (
        doc, foldable_tls_obj, action->foldable_tls_before, error);
    }
  if (action->out_tracks)
    {
      yyjson_mut_val * out_tracks_arr =
        yyjson_mut_obj_add_arr (doc, action_obj, "outTracks");
      for (int i = 0; i < action->num_out_tracks; i++)
        {
          yyjson_mut_arr_add_uint (doc, out_tracks_arr, action->out_tracks[i]);
        }
    }
  if (action->src_sends)
    {
      yyjson_mut_val * src_sends_arr =
        yyjson_mut_obj_add_arr (doc, action_obj, "srcSends");
      for (int i = 0; i < action->num_src_sends; i++)
        {
          yyjson_mut_val * src_send_obj =
            yyjson_mut_arr_add_obj (doc, src_sends_arr);
          channel_send_serialize_to_json (
            doc, src_send_obj, action->src_sends[i], error);
        }
    }
  yyjson_mut_obj_add_int (doc, action_obj, "editType", action->edit_type);
  if (action->ival_before)
    {
      yyjson_mut_val * ival_before_arr =
        yyjson_mut_obj_add_arr (doc, action_obj, "iValBefore");
      for (int i = 0; i < action->num_tracks; i++)
        {
          yyjson_mut_arr_add_int (doc, ival_before_arr, action->ival_before[i]);
        }
    }
  yyjson_mut_obj_add_int (doc, action_obj, "iValAfter", action->ival_after);
  if (action->colors_before)
    {
      yyjson_mut_val * colors_before_arr =
        yyjson_mut_obj_add_arr (doc, action_obj, "colorsBefore");
      for (int i = 0; i < action->num_tracks; i++)
        {
          yyjson_mut_val * color_obj =
            yyjson_mut_arr_add_obj (doc, colors_before_arr);
          gdk_rgba_serialize_to_json (
            doc, color_obj, &action->colors_before[i], error);
        }
    }
  yyjson_mut_val * new_color_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "newColor");
  gdk_rgba_serialize_to_json (doc, new_color_obj, &action->new_color, error);
  if (action->new_txt)
    {
      yyjson_mut_obj_add_str (doc, action_obj, "newTxt", action->new_txt);
    }
  yyjson_mut_obj_add_real (doc, action_obj, "valBefore", action->val_before);
  yyjson_mut_obj_add_real (doc, action_obj, "valAfter", action->val_after);
  yyjson_mut_obj_add_int (
    doc, action_obj, "numFoldChangeTracks", action->num_fold_change_tracks);
  if (action->connections_mgr_before)
    {
      yyjson_mut_val * connections_mgr_before_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "connectionsManagerBefore");
      port_connections_manager_serialize_to_json (
        doc, connections_mgr_before_obj, action->connections_mgr_before, error);
    }
  if (action->connections_mgr_after)
    {
      yyjson_mut_val * connections_mgr_after_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "connectionsManagerAfter");
      port_connections_manager_serialize_to_json (
        doc, connections_mgr_after_obj, action->connections_mgr_after, error);
    }
  return true;
}

bool
channel_send_action_serialize_to_json (
  yyjson_mut_doc *          doc,
  yyjson_mut_val *          action_obj,
  const ChannelSendAction * action,
  GError **                 error)
{
  yyjson_mut_val * parent_instance_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "base");
  undoable_action_serialize_to_json (
    doc, parent_instance_obj, &action->parent_instance, error);
  yyjson_mut_obj_add_int (doc, action_obj, "type", action->type);
  yyjson_mut_val * send_before_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "sendBefore");
  channel_send_serialize_to_json (
    doc, send_before_obj, action->send_before, error);
  if (action->connections_mgr_before)
    {
      yyjson_mut_val * connections_mgr_before_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "connectionsManagerBefore");
      port_connections_manager_serialize_to_json (
        doc, connections_mgr_before_obj, action->connections_mgr_before, error);
    }
  if (action->connections_mgr_after)
    {
      yyjson_mut_val * connections_mgr_after_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "connectionsManagerAfter");
      port_connections_manager_serialize_to_json (
        doc, connections_mgr_after_obj, action->connections_mgr_after, error);
    }
  return true;
}

bool
port_connection_action_serialize_to_json (
  yyjson_mut_doc *             doc,
  yyjson_mut_val *             action_obj,
  const PortConnectionAction * action,
  GError **                    error)
{
  yyjson_mut_val * parent_instance_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "base");
  undoable_action_serialize_to_json (
    doc, parent_instance_obj, &action->parent_instance, error);
  yyjson_mut_obj_add_int (doc, action_obj, "type", action->type);
  yyjson_mut_val * connection_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "connection");
  port_connection_serialize_to_json (
    doc, connection_obj, action->connection, error);
  yyjson_mut_obj_add_real (doc, action_obj, "val", action->val);
  return true;
}

bool
port_action_serialize_to_json (
  yyjson_mut_doc *   doc,
  yyjson_mut_val *   action_obj,
  const PortAction * action,
  GError **          error)
{
  yyjson_mut_val * parent_instance_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "base");
  undoable_action_serialize_to_json (
    doc, parent_instance_obj, &action->parent_instance, error);
  yyjson_mut_obj_add_int (doc, action_obj, "type", action->type);
  yyjson_mut_val * port_id_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "portId");
  port_identifier_serialize_to_json (doc, port_id_obj, &action->port_id, error);
  yyjson_mut_obj_add_real (doc, action_obj, "val", action->val);
  return true;
}

bool
midi_mapping_action_serialize_to_json (
  yyjson_mut_doc *          doc,
  yyjson_mut_val *          action_obj,
  const MidiMappingAction * action,
  GError **                 error)
{
  yyjson_mut_val * parent_instance_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "base");
  undoable_action_serialize_to_json (
    doc, parent_instance_obj, &action->parent_instance, error);
  yyjson_mut_obj_add_int (doc, action_obj, "index", action->idx);
  if (action->dest_port_id)
    {
      yyjson_mut_val * dest_port_id_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "destPortId");
      port_identifier_serialize_to_json (
        doc, dest_port_id_obj, action->dest_port_id, error);
    }
  if (action->dev_port)
    {
      yyjson_mut_val * dev_port_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "devPort");
      ext_port_serialize_to_json (doc, dev_port_obj, action->dev_port, error);
    }
  yyjson_mut_val * buf_arr = yyjson_mut_arr_with_uint8 (doc, action->buf, 3);
  yyjson_mut_obj_add_val (doc, action_obj, "buf", buf_arr);
  return true;
}

bool
range_action_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    action_obj,
  const RangeAction * action,
  GError **           error)
{
  yyjson_mut_val * parent_instance_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "base");
  undoable_action_serialize_to_json (
    doc, parent_instance_obj, &action->parent_instance, error);
  yyjson_mut_val * start_pos_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "startPos");
  position_serialize_to_json (doc, start_pos_obj, &action->start_pos, error);
  yyjson_mut_val * end_pos_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "endPos");
  position_serialize_to_json (doc, end_pos_obj, &action->end_pos, error);
  yyjson_mut_obj_add_int (doc, action_obj, "type", action->type);
  yyjson_mut_val * sel_before_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "selectionsBefore");
  timeline_selections_serialize_to_json (
    doc, sel_before_obj, action->sel_before, error);
  yyjson_mut_val * sel_after_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "selectionsAfter");
  timeline_selections_serialize_to_json (
    doc, sel_after_obj, action->sel_after, error);
  /* FIXME is this needed? */
  yyjson_mut_obj_add_bool (doc, action_obj, "firstRun", action->first_run);
  yyjson_mut_val * transport_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "transport");
  transport_serialize_to_json (doc, transport_obj, action->transport, error);
  return true;
}

bool
transport_action_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        action_obj,
  const TransportAction * action,
  GError **               error)
{
  yyjson_mut_val * parent_instance_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "base");
  undoable_action_serialize_to_json (
    doc, parent_instance_obj, &action->parent_instance, error);
  yyjson_mut_obj_add_int (doc, action_obj, "type", action->type);
  yyjson_mut_obj_add_real (doc, action_obj, "bpmBefore", action->bpm_before);
  yyjson_mut_obj_add_real (doc, action_obj, "bpmAfter", action->bpm_after);
  yyjson_mut_obj_add_int (doc, action_obj, "intBefore", action->int_before);
  yyjson_mut_obj_add_int (doc, action_obj, "intAfter", action->int_after);
  yyjson_mut_obj_add_int (doc, action_obj, "musicalMode", action->musical_mode);
  return true;
}

bool
chord_action_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    action_obj,
  const ChordAction * action,
  GError **           error)
{
  yyjson_mut_val * parent_instance_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "base");
  undoable_action_serialize_to_json (
    doc, parent_instance_obj, &action->parent_instance, error);
  yyjson_mut_obj_add_int (doc, action_obj, "type", action->type);
  if (action->chord_before)
    {
      yyjson_mut_val * chord_before_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "chordBefore");
      chord_descriptor_serialize_to_json (
        doc, chord_before_obj, action->chord_before, error);
    }
  if (action->chord_after)
    {
      yyjson_mut_val * chord_after_obj =
        yyjson_mut_obj_add_obj (doc, action_obj, "chordAfter");
      chord_descriptor_serialize_to_json (
        doc, chord_after_obj, action->chord_after, error);
    }
  yyjson_mut_obj_add_int (doc, action_obj, "chordIndex", action->chord_idx);
  if (action->chords_before)
    {
      yyjson_mut_val * chords_before_arr =
        yyjson_mut_obj_add_arr (doc, action_obj, "chordsBefore");
      for (int i = 0; i < CHORD_EDITOR_NUM_CHORDS; i++)
        {
          yyjson_mut_val * chord_before_obj =
            yyjson_mut_arr_add_obj (doc, chords_before_arr);
          chord_descriptor_serialize_to_json (
            doc, chord_before_obj, action->chords_before[i], error);
        }
    }
  if (action->chords_after)
    {
      yyjson_mut_val * chords_after_arr =
        yyjson_mut_obj_add_arr (doc, action_obj, "chordsAfter");
      for (int i = 0; i < CHORD_EDITOR_NUM_CHORDS; i++)
        {
          yyjson_mut_val * chord_after_obj =
            yyjson_mut_arr_add_obj (doc, chords_after_arr);
          chord_descriptor_serialize_to_json (
            doc, chord_after_obj, action->chords_after[i], error);
        }
    }
  return true;
}
