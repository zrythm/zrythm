// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
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
#include "utils/objects.h"

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
  yyjson_mut_obj_add_uint (doc, action_obj, "sampleRate", action->sample_rate);
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
  yyjson_mut_obj_add_int (doc, action_obj, "numPlugins", action->num_plugins);
  yyjson_mut_obj_add_int (doc, action_obj, "newVal", action->new_val);
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
  yyjson_mut_obj_add_int (doc, action_obj, "type", action->type);
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
  yyjson_mut_obj_add_int (doc, action_obj, "type", action->type);
  yyjson_mut_val * start_pos_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "startPos");
  position_serialize_to_json (doc, start_pos_obj, &action->start_pos, error);
  yyjson_mut_val * end_pos_obj =
    yyjson_mut_obj_add_obj (doc, action_obj, "endPos");
  position_serialize_to_json (doc, end_pos_obj, &action->end_pos, error);
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
  yyjson_mut_obj_add_bool (doc, action_obj, "musicalMode", action->musical_mode);
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

bool
undoable_action_deserialize_from_json (
  yyjson_doc *     doc,
  yyjson_val *     action_obj,
  UndoableAction * action,
  GError **        error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (action_obj);
  action->type =
    (UndoableActionType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  action->frames_per_tick =
    yyjson_get_real (yyjson_obj_iter_get (&it, "framesPerTick"));
  action->sample_rate =
    yyjson_get_uint (yyjson_obj_iter_get (&it, "sampleRate"));
  action->stack_idx = yyjson_get_int (yyjson_obj_iter_get (&it, "stackIndex"));
  action->num_actions = yyjson_get_int (yyjson_obj_iter_get (&it, "numActions"));
  return true;
}

bool
arranger_selections_action_deserialize_from_json (
  yyjson_doc *               doc,
  yyjson_val *               action_obj,
  ArrangerSelectionsAction * action,
  GError **                  error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (action_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  undoable_action_deserialize_from_json (
    doc, base_obj, &action->parent_instance, error);
  action->type = (ArrangerSelectionsActionType) yyjson_get_int (
    yyjson_obj_iter_get (&it, "type"));
  action->edit_type = (ArrangerSelectionsActionEditType) yyjson_get_int (
    yyjson_obj_iter_get (&it, "editType"));
  action->resize_type = (ArrangerSelectionsActionResizeType) yyjson_get_int (
    yyjson_obj_iter_get (&it, "resizeType"));
  action->ticks = yyjson_get_real (yyjson_obj_iter_get (&it, "ticks"));
  action->delta_tracks =
    yyjson_get_int (yyjson_obj_iter_get (&it, "deltaTracks"));
  action->delta_lanes = yyjson_get_int (yyjson_obj_iter_get (&it, "deltaLanes"));
  action->delta_chords =
    yyjson_get_int (yyjson_obj_iter_get (&it, "deltaChords"));
  action->delta_pitch = yyjson_get_int (yyjson_obj_iter_get (&it, "deltaPitch"));
  action->delta_vel = yyjson_get_int (yyjson_obj_iter_get (&it, "deltaVel"));
  action->delta_normalized_amount =
    yyjson_get_real (yyjson_obj_iter_get (&it, "deltaNormalizedAmount"));
  yyjson_val * target_port_obj = yyjson_obj_iter_get (&it, "targetPort");
  if (target_port_obj)
    {
      action->target_port = object_new (Port);
      port_identifier_deserialize_from_json (
        doc, target_port_obj, action->target_port, error);
    }
  yyjson_val * str_obj = yyjson_obj_iter_get (&it, "str");
  if (str_obj)
    {
      action->str = g_strdup (yyjson_get_str (str_obj));
    }
  yyjson_val * pos_obj = yyjson_obj_iter_get (&it, "pos");
  position_deserialize_from_json (doc, pos_obj, &action->pos, error);
  yyjson_val * opts_obj = yyjson_obj_iter_get (&it, "quantizeOptions");
  if (opts_obj)
    {
      action->opts = object_new (QuantizeOptions);
      quantize_options_deserialize_from_json (
        doc, opts_obj, action->opts, error);
    }
  yyjson_val * chord_sel_obj = yyjson_obj_iter_get (&it, "chordSelections");
  if (chord_sel_obj)
    {
      action->chord_sel = object_new (ChordSelections);
      chord_selections_deserialize_from_json (
        doc, chord_sel_obj, action->chord_sel, error);
    }
  yyjson_val * tl_sel_obj = yyjson_obj_iter_get (&it, "timelineSelections");
  if (tl_sel_obj)
    {
      action->tl_sel = object_new (TimelineSelections);
      timeline_selections_deserialize_from_json (
        doc, tl_sel_obj, action->tl_sel, error);
    }
  yyjson_val * ma_sel_obj = yyjson_obj_iter_get (&it, "midiArrangerSelections");
  if (ma_sel_obj)
    {
      action->ma_sel = object_new (MidiArrangerSelections);
      midi_arranger_selections_deserialize_from_json (
        doc, ma_sel_obj, action->ma_sel, error);
    }
  yyjson_val * automation_sel_obj =
    yyjson_obj_iter_get (&it, "automationSelections");
  if (automation_sel_obj)
    {
      action->automation_sel = object_new (AutomationSelections);
      automation_selections_deserialize_from_json (
        doc, automation_sel_obj, action->automation_sel, error);
    }
  yyjson_val * audio_sel_obj = yyjson_obj_iter_get (&it, "audioSelections");
  if (audio_sel_obj)
    {
      action->audio_sel = object_new (AudioSelections);
      audio_selections_deserialize_from_json (
        doc, audio_sel_obj, action->audio_sel, error);
    }
  yyjson_val * chord_sel_after_obj =
    yyjson_obj_iter_get (&it, "chordSelectionsAfter");
  if (chord_sel_after_obj)
    {
      action->chord_sel_after = object_new (ChordSelections);
      chord_selections_deserialize_from_json (
        doc, chord_sel_after_obj, action->chord_sel_after, error);
    }
  yyjson_val * tl_sel_after_obj =
    yyjson_obj_iter_get (&it, "timelineSelectionsAfter");
  if (tl_sel_after_obj)
    {
      action->tl_sel_after = object_new (TimelineSelections);
      timeline_selections_deserialize_from_json (
        doc, tl_sel_after_obj, action->tl_sel_after, error);
    }
  yyjson_val * ma_sel_after_obj =
    yyjson_obj_iter_get (&it, "midiArrangerSelectionsAfter");
  if (ma_sel_after_obj)
    {
      action->ma_sel_after = object_new (MidiArrangerSelections);
      midi_arranger_selections_deserialize_from_json (
        doc, ma_sel_after_obj, action->ma_sel_after, error);
    }
  yyjson_val * automation_sel_after_obj =
    yyjson_obj_iter_get (&it, "automationSelectionsAfter");
  if (automation_sel_after_obj)
    {
      action->automation_sel_after = object_new (AutomationSelections);
      automation_selections_deserialize_from_json (
        doc, automation_sel_after_obj, action->automation_sel_after, error);
    }
  yyjson_val * audio_sel_after_obj =
    yyjson_obj_iter_get (&it, "audioSelectionsAfter");
  if (audio_sel_after_obj)
    {
      action->audio_sel_after = object_new (AudioSelections);
      audio_selections_deserialize_from_json (
        doc, audio_sel_after_obj, action->audio_sel_after, error);
    }
  yyjson_val * region_before_obj = yyjson_obj_iter_get (&it, "regionBefore");
  if (region_before_obj)
    {
      action->region_before = object_new (ZRegion);
      region_deserialize_from_json (
        doc, region_before_obj, action->region_before, error);
    }
  yyjson_val * region_after_obj = yyjson_obj_iter_get (&it, "regionAfter");
  if (region_after_obj)
    {
      action->region_after = object_new (ZRegion);
      region_deserialize_from_json (
        doc, region_after_obj, action->region_after, error);
    }
  yyjson_val * region_r1_arr = yyjson_obj_iter_get (&it, "regionR1");
  yyjson_val * region_r2_arr = yyjson_obj_iter_get (&it, "regionR2");
  yyjson_val * midi_note_r1_arr = yyjson_obj_iter_get (&it, "midiNoteR1");
  yyjson_val * midi_note_r2_arr = yyjson_obj_iter_get (&it, "midiNoteR2");
  action->num_split_objs =
    MAX (yyjson_arr_size (region_r1_arr), yyjson_arr_size (midi_note_r1_arr));
  size_t          count = 0;
  yyjson_arr_iter region_r1_iter = yyjson_arr_iter_with (region_r1_arr);
  yyjson_val *    region_r1_obj = NULL;
  while ((region_r1_obj = yyjson_arr_iter_next (&region_r1_iter)))
    {
      ZRegion * r = object_new (ZRegion);
      action->region_r1[count++] = r;
      region_deserialize_from_json (doc, region_r1_obj, r, error);
    }
  count = 0;
  yyjson_arr_iter region_r2_iter = yyjson_arr_iter_with (region_r2_arr);
  yyjson_val *    region_r2_obj = NULL;
  while ((region_r2_obj = yyjson_arr_iter_next (&region_r2_iter)))
    {
      ZRegion * r = object_new (ZRegion);
      action->region_r2[count++] = r;
      region_deserialize_from_json (doc, region_r2_obj, r, error);
    }
  count = 0;
  yyjson_arr_iter mn_r1_iter = yyjson_arr_iter_with (midi_note_r1_arr);
  yyjson_val *    mn_r1_obj = NULL;
  while ((mn_r1_obj = yyjson_arr_iter_next (&mn_r1_iter)))
    {
      MidiNote * r = object_new (MidiNote);
      action->mn_r1[count++] = r;
      midi_note_deserialize_from_json (doc, mn_r1_obj, r, error);
    }
  count = 0;
  yyjson_arr_iter mn_r2_iter = yyjson_arr_iter_with (midi_note_r2_arr);
  yyjson_val *    mn_r2_obj = NULL;
  while ((mn_r2_obj = yyjson_arr_iter_next (&mn_r2_iter)))
    {
      MidiNote * r = object_new (MidiNote);
      action->mn_r2[count++] = r;
      midi_note_deserialize_from_json (doc, mn_r2_obj, r, error);
    }
  return true;
}

bool
mixer_selections_action_deserialize_from_json (
  yyjson_doc *            doc,
  yyjson_val *            action_obj,
  MixerSelectionsAction * action,
  GError **               error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (action_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  undoable_action_deserialize_from_json (
    doc, base_obj, &action->parent_instance, error);
  action->type = (MixerSelectionsActionType) yyjson_get_int (
    yyjson_obj_iter_get (&it, "type"));
  action->slot_type =
    (PluginSlotType) yyjson_get_int (yyjson_obj_iter_get (&it, "slotType"));
  action->to_slot = yyjson_get_int (yyjson_obj_iter_get (&it, "toSlot"));
  action->to_track_name_hash =
    yyjson_get_uint (yyjson_obj_iter_get (&it, "toTrackNameHash"));
  action->new_channel = yyjson_get_int (yyjson_obj_iter_get (&it, "newChannel"));
  action->num_plugins = yyjson_get_int (yyjson_obj_iter_get (&it, "numPlugins"));
  /* introduced in 1.9 */
  yyjson_val * new_val_obj = yyjson_obj_iter_get (&it, "newVal");
  if (new_val_obj)
    {
      action->new_val = yyjson_get_int (new_val_obj);
    }
  yyjson_val * setting_obj = yyjson_obj_iter_get (&it, "setting");
  if (setting_obj)
    {
      action->setting = object_new (PluginSetting);
      plugin_setting_deserialize_from_json (
        doc, setting_obj, action->setting, error);
    }
  yyjson_val * ms_before_obj =
    yyjson_obj_iter_get (&it, "mixerSelectionsBefore");
  if (ms_before_obj)
    {
      action->ms_before = object_new (MixerSelections);
      mixer_selections_deserialize_from_json (
        doc, ms_before_obj, action->ms_before, error);
    }
  yyjson_val * deleted_ms_obj = yyjson_obj_iter_get (&it, "deletedSelections");
  if (deleted_ms_obj)
    {
      action->deleted_ms = object_new (MixerSelections);
      mixer_selections_deserialize_from_json (
        doc, deleted_ms_obj, action->deleted_ms, error);
    }
  yyjson_val *    ats_arr = yyjson_obj_iter_get (&it, "automationTracks");
  yyjson_arr_iter at_it = yyjson_arr_iter_with (ats_arr);
  yyjson_val *    at_obj = NULL;
  while ((at_obj = yyjson_arr_iter_next (&at_it)))
    {
      AutomationTrack * at = object_new (AutomationTrack);
      action->ats[action->num_ats++] = at;
      automation_track_deserialize_from_json (doc, at_obj, at, error);
    }
  yyjson_val * deleted_ats_arr =
    yyjson_obj_iter_get (&it, "deletedAutomationTracks");
  yyjson_arr_iter deleted_at_it = yyjson_arr_iter_with (deleted_ats_arr);
  yyjson_val *    deleted_at_obj = NULL;
  while ((deleted_at_obj = yyjson_arr_iter_next (&deleted_at_it)))
    {
      AutomationTrack * at = object_new (AutomationTrack);
      action->deleted_ats[action->num_deleted_ats++] = at;
      automation_track_deserialize_from_json (doc, deleted_at_obj, at, error);
    }
  yyjson_val * connections_mgr_before_obj =
    yyjson_obj_iter_get (&it, "connectionsManagerBefore");
  if (connections_mgr_before_obj)
    {
      action->connections_mgr_before = object_new (PortConnectionsManager);
      port_connections_manager_deserialize_from_json (
        doc, connections_mgr_before_obj, action->connections_mgr_before, error);
    }
  yyjson_val * connections_mgr_after_obj =
    yyjson_obj_iter_get (&it, "connectionsManagerAfter");
  if (connections_mgr_after_obj)
    {
      action->connections_mgr_after = object_new (PortConnectionsManager);
      port_connections_manager_deserialize_from_json (
        doc, connections_mgr_after_obj, action->connections_mgr_after, error);
    }
  return true;
}

bool
tracklist_selections_action_deserialize_from_json (
  yyjson_doc *                doc,
  yyjson_val *                action_obj,
  TracklistSelectionsAction * action,
  GError **                   error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (action_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  undoable_action_deserialize_from_json (
    doc, base_obj, &action->parent_instance, error);
  action->type = (TracklistSelectionsActionType) yyjson_get_int (
    yyjson_obj_iter_get (&it, "type"));
  action->track_type =
    (TrackType) yyjson_get_int (yyjson_obj_iter_get (&it, "trackType"));
  yyjson_val * pl_setting_obj = yyjson_obj_iter_get (&it, "pluginSetting");
  if (pl_setting_obj)
    {
      action->pl_setting = object_new (PluginSetting);
      plugin_setting_deserialize_from_json (
        doc, pl_setting_obj, action->pl_setting, error);
    }
  action->is_empty = yyjson_get_bool (yyjson_obj_iter_get (&it, "isEmpty"));
  action->track_pos = yyjson_get_int (yyjson_obj_iter_get (&it, "trackPos"));
  action->lane_pos = yyjson_get_int (yyjson_obj_iter_get (&it, "lanePos"));
  action->have_pos = yyjson_get_bool (yyjson_obj_iter_get (&it, "havePos"));
  yyjson_val * position_obj = yyjson_obj_iter_get (&it, "pos");
  position_deserialize_from_json (doc, position_obj, &action->pos, error);
  yyjson_val *    tracks_before_arr = yyjson_obj_iter_get (&it, "tracksBefore");
  yyjson_val *    tracks_after_arr = yyjson_obj_iter_get (&it, "tracksAfter");
  size_t          count = 0;
  yyjson_arr_iter tracks_before_it = yyjson_arr_iter_with (tracks_before_arr);
  yyjson_val *    track_before_obj = NULL;
  while ((track_before_obj = yyjson_arr_iter_next (&tracks_before_it)))
    {
      action->tracks_before[count++] = yyjson_get_int (track_before_obj);
    }
  count = 0;
  yyjson_arr_iter tracks_after_it = yyjson_arr_iter_with (tracks_after_arr);
  yyjson_val *    track_after_obj = NULL;
  while ((track_after_obj = yyjson_arr_iter_next (&tracks_after_it)))
    {
      action->tracks_after[count++] = yyjson_get_int (track_after_obj);
    }
  action->num_tracks = yyjson_get_int (yyjson_obj_iter_get (&it, "numTracks"));
  yyjson_val * file_basename_obj = yyjson_obj_iter_get (&it, "fileBasename");
  if (file_basename_obj)
    {
      action->file_basename = g_strdup (yyjson_get_str (file_basename_obj));
    }
  yyjson_val * base64_midi_obj = yyjson_obj_iter_get (&it, "base64Midi");
  if (base64_midi_obj)
    {
      action->base64_midi = g_strdup (yyjson_get_str (base64_midi_obj));
    }
  action->pool_id = yyjson_get_int (yyjson_obj_iter_get (&it, "poolId"));
  yyjson_val * tls_before_obj =
    yyjson_obj_iter_get (&it, "tracklistSelectionsBefore");
  if (tls_before_obj)
    {
      action->tls_before = object_new (TracklistSelections);
      tracklist_selections_deserialize_from_json (
        doc, tls_before_obj, action->tls_before, error);
    }
  yyjson_val * tls_after_obj =
    yyjson_obj_iter_get (&it, "tracklistSelectionsAfter");
  if (tls_after_obj)
    {
      action->tls_after = object_new (TracklistSelections);
      tracklist_selections_deserialize_from_json (
        doc, tls_after_obj, action->tls_after, error);
    }
  yyjson_val * foldable_tls_before_obj =
    yyjson_obj_iter_get (&it, "foldableTracklistSelectionsBefore");
  if (foldable_tls_before_obj)
    {
      action->foldable_tls_before = object_new (TracklistSelections);
      tracklist_selections_deserialize_from_json (
        doc, foldable_tls_before_obj, action->foldable_tls_before, error);
    }
  yyjson_val * out_tracks_arr = yyjson_obj_iter_get (&it, "outTracks");
  if (out_tracks_arr)
    {
      action->num_out_tracks = yyjson_arr_size (out_tracks_arr);
      if (action->num_out_tracks > 0)
        {
          action->out_tracks = g_malloc0_n (
            (size_t) action->num_out_tracks, sizeof (unsigned int));
          yyjson_arr_iter out_tracks_it = yyjson_arr_iter_with (out_tracks_arr);
          yyjson_val *    out_track_obj = NULL;
          count = 0;
          while ((out_track_obj = yyjson_arr_iter_next (&out_tracks_it)))
            {
              action->out_tracks[count++] = yyjson_get_uint (out_track_obj);
            }
        }
    }
  yyjson_val * src_sends_arr = yyjson_obj_iter_get (&it, "srcSends");
  if (src_sends_arr)
    {
      action->src_sends_size = yyjson_arr_size (src_sends_arr);
      if (action->src_sends_size > 0)
        {
          action->src_sends =
            g_malloc0_n (action->src_sends_size, sizeof (ChannelSend *));
          yyjson_arr_iter src_send_it = yyjson_arr_iter_with (src_sends_arr);
          yyjson_val *    src_send_obj = NULL;
          while ((src_send_obj = yyjson_arr_iter_next (&src_send_it)))
            {
              ChannelSend * mn = object_new (ChannelSend);
              action->src_sends[action->num_src_sends++] = mn;
              channel_send_deserialize_from_json (doc, src_send_obj, mn, error);
            }
        }
    }
  action->edit_type = (EditTracksActionType) yyjson_get_int (
    yyjson_obj_iter_get (&it, "editType"));
  yyjson_val * ival_before_arr = yyjson_obj_iter_get (&it, "iValBefore");
  if (ival_before_arr)
    {
      if (action->num_tracks > 0)
        {
          action->ival_before =
            g_malloc0_n ((size_t) action->num_tracks, sizeof (int));
          yyjson_arr_iter ival_before_it =
            yyjson_arr_iter_with (ival_before_arr);
          yyjson_val * ival_before_obj = NULL;
          count = 0;
          while ((ival_before_obj = yyjson_arr_iter_next (&ival_before_it)))
            {
              action->ival_before[count++] = yyjson_get_int (ival_before_obj);
            }
        }
    }
  action->ival_after = yyjson_get_int (yyjson_obj_iter_get (&it, "iValAfter"));
  yyjson_val * colors_before_arr = yyjson_obj_iter_get (&it, "colorsBefore");
  if (colors_before_arr)
    {
      if (action->num_tracks > 0)
        {
          action->colors_before =
            g_malloc0_n ((size_t) action->num_tracks, sizeof (GdkRGBA));
          yyjson_arr_iter colors_before_it =
            yyjson_arr_iter_with (colors_before_arr);
          yyjson_val * colors_before_obj = NULL;
          count = 0;
          while ((colors_before_obj = yyjson_arr_iter_next (&colors_before_it)))
            {
              gdk_rgba_deserialize_from_json (
                doc, colors_before_obj, &action->colors_before[count++], error);
            }
        }
    }
  yyjson_val * new_color_obj = yyjson_obj_iter_get (&it, "newColor");
  gdk_rgba_deserialize_from_json (doc, new_color_obj, &action->new_color, error);
  yyjson_val * new_txt_obj = yyjson_obj_iter_get (&it, "newTxt");
  if (new_txt_obj)
    {
      action->new_txt = g_strdup (yyjson_get_str (new_txt_obj));
    }
  action->val_before =
    (float) yyjson_get_real (yyjson_obj_iter_get (&it, "valBefore"));
  action->val_after =
    (float) yyjson_get_real (yyjson_obj_iter_get (&it, "valAfter"));
  action->num_fold_change_tracks =
    yyjson_get_int (yyjson_obj_iter_get (&it, "numFoldChangeTracks"));
  yyjson_val * connections_mgr_before_obj =
    yyjson_obj_iter_get (&it, "connectionsManagerBefore");
  if (connections_mgr_before_obj)
    {
      action->connections_mgr_before = object_new (PortConnectionsManager);
      port_connections_manager_deserialize_from_json (
        doc, connections_mgr_before_obj, action->connections_mgr_before, error);
    }
  yyjson_val * connections_mgr_after_obj =
    yyjson_obj_iter_get (&it, "connectionsManagerAfter");
  if (connections_mgr_after_obj)
    {
      action->connections_mgr_after = object_new (PortConnectionsManager);
      port_connections_manager_deserialize_from_json (
        doc, connections_mgr_after_obj, action->connections_mgr_after, error);
    }
  return true;
}

bool
channel_send_action_deserialize_from_json (
  yyjson_doc *        doc,
  yyjson_val *        action_obj,
  ChannelSendAction * action,
  GError **           error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (action_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  undoable_action_deserialize_from_json (
    doc, base_obj, &action->parent_instance, error);
  action->type =
    (ChannelSendActionType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  yyjson_val * send_before_obj = yyjson_obj_iter_get (&it, "sendBefore");
  action->send_before = object_new (ChannelSend);
  channel_send_deserialize_from_json (
    doc, send_before_obj, action->send_before, error);
  yyjson_val * connections_mgr_before_obj =
    yyjson_obj_iter_get (&it, "connectionsManagerBefore");
  if (connections_mgr_before_obj)
    {
      action->connections_mgr_before = object_new (PortConnectionsManager);
      port_connections_manager_deserialize_from_json (
        doc, connections_mgr_before_obj, action->connections_mgr_before, error);
    }
  yyjson_val * connections_mgr_after_obj =
    yyjson_obj_iter_get (&it, "connectionsManagerAfter");
  if (connections_mgr_after_obj)
    {
      action->connections_mgr_after = object_new (PortConnectionsManager);
      port_connections_manager_deserialize_from_json (
        doc, connections_mgr_after_obj, action->connections_mgr_after, error);
    }
  return true;
}

bool
port_connection_action_deserialize_from_json (
  yyjson_doc *           doc,
  yyjson_val *           action_obj,
  PortConnectionAction * action,
  GError **              error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (action_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  undoable_action_deserialize_from_json (
    doc, base_obj, &action->parent_instance, error);
  action->type = (PortConnectionActionType) yyjson_get_int (
    yyjson_obj_iter_get (&it, "type"));
  yyjson_val * connection_obj = yyjson_obj_iter_get (&it, "connection");
  action->connection = object_new (PortConnection);
  port_connection_deserialize_from_json (
    doc, connection_obj, action->connection, error);
  action->val = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "val"));
  return true;
}

bool
port_action_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * action_obj,
  PortAction * action,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (action_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  undoable_action_deserialize_from_json (
    doc, base_obj, &action->parent_instance, error);
  action->type =
    (PortActionType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  yyjson_val * port_id_obj = yyjson_obj_iter_get (&it, "portId");
  port_identifier_deserialize_from_json (
    doc, port_id_obj, &action->port_id, error);
  action->val = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "val"));
  return true;
}

bool
midi_mapping_action_deserialize_from_json (
  yyjson_doc *        doc,
  yyjson_val *        action_obj,
  MidiMappingAction * action,
  GError **           error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (action_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  undoable_action_deserialize_from_json (
    doc, base_obj, &action->parent_instance, error);
  action->type =
    (MidiMappingActionType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  action->idx = yyjson_get_int (yyjson_obj_iter_get (&it, "index"));
  yyjson_val * dest_port_id_obj = yyjson_obj_iter_get (&it, "destPortId");
  if (dest_port_id_obj)
    {
      action->dest_port_id = object_new (PortIdentifier);
      port_identifier_deserialize_from_json (
        doc, dest_port_id_obj, action->dest_port_id, error);
    }
  yyjson_val * dev_port_obj = yyjson_obj_iter_get (&it, "devPort");
  if (dev_port_obj)
    {
      action->dev_port = object_new (ExtPort);
      ext_port_deserialize_from_json (
        doc, dev_port_obj, action->dev_port, error);
    }
  yyjson_val *    buf_arr = yyjson_obj_iter_get (&it, "buf");
  yyjson_arr_iter buf_it = yyjson_arr_iter_with (buf_arr);
  yyjson_val *    buf_obj = NULL;
  size_t          count = 0;
  while ((buf_obj = yyjson_arr_iter_next (&buf_it)))
    {
      action->buf[count++] = (midi_byte_t) yyjson_get_uint (buf_obj);
    }
  return true;
}

bool
range_action_deserialize_from_json (
  yyjson_doc *  doc,
  yyjson_val *  action_obj,
  RangeAction * action,
  GError **     error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (action_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  undoable_action_deserialize_from_json (
    doc, base_obj, &action->parent_instance, error);
  action->type =
    (RangeActionType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  yyjson_val * start_pos_obj = yyjson_obj_iter_get (&it, "startPos");
  position_deserialize_from_json (doc, start_pos_obj, &action->start_pos, error);
  yyjson_val * end_pos_obj = yyjson_obj_iter_get (&it, "endPos");
  position_deserialize_from_json (doc, end_pos_obj, &action->end_pos, error);
  yyjson_val * sel_before_obj = yyjson_obj_iter_get (&it, "selectionsBefore");
  action->sel_before = object_new (TimelineSelections);
  timeline_selections_deserialize_from_json (
    doc, sel_before_obj, action->sel_before, error);
  yyjson_val * sel_after_obj = yyjson_obj_iter_get (&it, "selectionsAfter");
  action->sel_after = object_new (TimelineSelections);
  timeline_selections_deserialize_from_json (
    doc, sel_after_obj, action->sel_after, error);
  action->first_run = yyjson_get_bool (yyjson_obj_iter_get (&it, "firstRun"));
  yyjson_val * transport_obj = yyjson_obj_iter_get (&it, "transport");
  action->transport = object_new (Transport);
  transport_deserialize_from_json (doc, transport_obj, action->transport, error);
  return true;
}

bool
transport_action_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      action_obj,
  TransportAction * action,
  GError **         error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (action_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  undoable_action_deserialize_from_json (
    doc, base_obj, &action->parent_instance, error);
  action->type =
    (TransportActionType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  action->bpm_before =
    (bpm_t) yyjson_get_real (yyjson_obj_iter_get (&it, "bpmBefore"));
  action->bpm_after =
    (bpm_t) yyjson_get_real (yyjson_obj_iter_get (&it, "bpmAfter"));
  action->int_before = yyjson_get_int (yyjson_obj_iter_get (&it, "intBefore"));
  action->int_after = yyjson_get_int (yyjson_obj_iter_get (&it, "intAfter"));
  action->musical_mode =
    yyjson_get_bool (yyjson_obj_iter_get (&it, "musicalMode"));
  return true;
}

bool
chord_action_deserialize_from_json (
  yyjson_doc *  doc,
  yyjson_val *  action_obj,
  ChordAction * action,
  GError **     error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (action_obj);
  yyjson_val *    base_obj = yyjson_obj_iter_get (&it, "base");
  undoable_action_deserialize_from_json (
    doc, base_obj, &action->parent_instance, error);
  action->type =
    (ChordActionType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  yyjson_val * chord_before_obj = yyjson_obj_iter_get (&it, "chordBefore");
  if (chord_before_obj)
    {
      action->chord_before = object_new (ChordDescriptor);
      chord_descriptor_deserialize_from_json (
        doc, chord_before_obj, action->chord_before, error);
    }
  yyjson_val * chord_after_obj = yyjson_obj_iter_get (&it, "chordAfter");
  if (chord_after_obj)
    {
      action->chord_after = object_new (ChordDescriptor);
      chord_descriptor_deserialize_from_json (
        doc, chord_after_obj, action->chord_after, error);
    }
  action->chord_idx = yyjson_get_int (yyjson_obj_iter_get (&it, "chordIndex"));
  yyjson_val * chords_before_arr = yyjson_obj_iter_get (&it, "chordsBefore");
  if (chords_before_arr)
    {
      action->chords_before =
        g_malloc0_n (CHORD_EDITOR_NUM_CHORDS, sizeof (ChordDescriptor *));
      size_t          count = 0;
      yyjson_arr_iter chords_before_it =
        yyjson_arr_iter_with (chords_before_arr);
      yyjson_val * descr_obj = NULL;
      while ((descr_obj = yyjson_arr_iter_next (&chords_before_it)))
        {
          ChordDescriptor * descr = object_new (ChordDescriptor);
          action->chords_before[count++] = descr;
          chord_descriptor_deserialize_from_json (doc, descr_obj, descr, error);
        }
    }
  yyjson_val * chords_after_arr = yyjson_obj_iter_get (&it, "chordsAfter");
  if (chords_after_arr)
    {
      action->chords_after =
        g_malloc0_n (CHORD_EDITOR_NUM_CHORDS, sizeof (ChordDescriptor *));
      size_t          count = 0;
      yyjson_arr_iter chords_after_it = yyjson_arr_iter_with (chords_after_arr);
      yyjson_val *    descr_obj = NULL;
      while ((descr_obj = yyjson_arr_iter_next (&chords_after_it)))
        {
          ChordDescriptor * descr = object_new (ChordDescriptor);
          action->chords_after[count++] = descr;
          chord_descriptor_deserialize_from_json (doc, descr_obj, descr, error);
        }
    }
  return true;
}
