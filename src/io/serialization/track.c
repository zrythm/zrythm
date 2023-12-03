// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/track.h"
#include "io/serialization/arranger_objects.h"
#include "io/serialization/channel.h"
#include "io/serialization/extra.h"
#include "io/serialization/plugin.h"
#include "io/serialization/port.h"
#include "io/serialization/track.h"

typedef enum
{
  Z_IO_SERIALIZATION_TRACK_ERROR_FAILED,
} ZIOSerializationTrackError;

#define Z_IO_SERIALIZATION_TRACK_ERROR z_io_serialization_track_error_quark ()
GQuark
z_io_serialization_track_error_quark (void);
G_DEFINE_QUARK (z - io - serialization - track - error - quark, z_io_serialization_track_error)

bool
modulator_macro_processor_serialize_to_json (
  yyjson_mut_doc *                doc,
  yyjson_mut_val *                mmp_obj,
  const ModulatorMacroProcessor * mmp,
  GError **                       error)
{
  yyjson_mut_obj_add_str (doc, mmp_obj, "name", mmp->name);
  yyjson_mut_val * cv_in_obj = yyjson_mut_obj_add_obj (doc, mmp_obj, "cvIn");
  port_serialize_to_json (doc, cv_in_obj, mmp->cv_in, error);
  yyjson_mut_val * cv_out_obj = yyjson_mut_obj_add_obj (doc, mmp_obj, "cvOut");
  port_serialize_to_json (doc, cv_out_obj, mmp->cv_out, error);
  yyjson_mut_val * macro_obj = yyjson_mut_obj_add_obj (doc, mmp_obj, "macro");
  port_serialize_to_json (doc, macro_obj, mmp->macro, error);
  return true;
}

bool
track_processor_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       tp_obj,
  const TrackProcessor * tp,
  GError **              error)
{
  if (tp->mono)
    {
      yyjson_mut_val * mono_obj = yyjson_mut_obj_add_obj (doc, tp_obj, "mono");
      port_serialize_to_json (doc, mono_obj, tp->mono, error);
    }
  if (tp->input_gain)
    {
      yyjson_mut_val * input_gain_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "inputGain");
      port_serialize_to_json (doc, input_gain_obj, tp->input_gain, error);
    }
  if (tp->output_gain)
    {
      yyjson_mut_val * output_gain_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "outputGain");
      port_serialize_to_json (doc, output_gain_obj, tp->output_gain, error);
    }
  if (tp->midi_in)
    {
      yyjson_mut_val * midi_in_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "midiIn");
      port_serialize_to_json (doc, midi_in_obj, tp->midi_in, error);
    }
  if (tp->midi_out)
    {
      yyjson_mut_val * midi_out_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "midiOut");
      port_serialize_to_json (doc, midi_out_obj, tp->midi_out, error);
    }
  if (tp->piano_roll)
    {
      yyjson_mut_val * piano_roll_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "pianoRoll");
      port_serialize_to_json (doc, piano_roll_obj, tp->piano_roll, error);
    }
  if (tp->monitor_audio)
    {
      yyjson_mut_val * monitor_audio_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "monitorAudio");
      port_serialize_to_json (doc, monitor_audio_obj, tp->monitor_audio, error);
    }
  if (tp->stereo_in)
    {
      yyjson_mut_val * stereo_in_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "stereoIn");
      stereo_ports_serialize_to_json (doc, stereo_in_obj, tp->stereo_in, error);
    }
  if (tp->stereo_out)
    {
      yyjson_mut_val * stereo_out_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "stereoOut");
      stereo_ports_serialize_to_json (
        doc, stereo_out_obj, tp->stereo_out, error);
    }
  if (track_type_has_piano_roll (tp->track->type))
    {
      yyjson_mut_val * midi_cc_arr =
        yyjson_mut_obj_add_arr (doc, tp_obj, "midiCc");
      for (int j = 0; j < 128 * 16; j++)
        {
          Port *           port = tp->midi_cc[j];
          yyjson_mut_val * port_obj = yyjson_mut_arr_add_obj (doc, midi_cc_arr);
          port_serialize_to_json (doc, port_obj, port, error);
        }
      yyjson_mut_val * pitch_bend_arr =
        yyjson_mut_obj_add_arr (doc, tp_obj, "pitchBend");
      for (int j = 0; j < 16; j++)
        {
          Port *           port = tp->pitch_bend[j];
          yyjson_mut_val * port_obj =
            yyjson_mut_arr_add_obj (doc, pitch_bend_arr);
          port_serialize_to_json (doc, port_obj, port, error);
        }
      yyjson_mut_val * poly_key_pressure_arr =
        yyjson_mut_obj_add_arr (doc, tp_obj, "polyKeyPressure");
      for (int j = 0; j < 16; j++)
        {
          Port *           port = tp->poly_key_pressure[j];
          yyjson_mut_val * port_obj =
            yyjson_mut_arr_add_obj (doc, poly_key_pressure_arr);
          port_serialize_to_json (doc, port_obj, port, error);
        }
      yyjson_mut_val * channel_pressure_arr =
        yyjson_mut_obj_add_arr (doc, tp_obj, "channelPressure");
      for (int j = 0; j < 16; j++)
        {
          Port *           port = tp->channel_pressure[j];
          yyjson_mut_val * port_obj =
            yyjson_mut_arr_add_obj (doc, channel_pressure_arr);
          port_serialize_to_json (doc, port_obj, port, error);
        }
    }
  return true;
}

bool
automation_track_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        at_obj,
  const AutomationTrack * at,
  GError **               error)
{
  yyjson_mut_obj_add_int (doc, at_obj, "index", at->index);
  yyjson_mut_val * port_id_obj = yyjson_mut_obj_add_obj (doc, at_obj, "portId");
  port_identifier_serialize_to_json (doc, port_id_obj, &at->port_id, error);
  if (at->regions)
    {
      yyjson_mut_val * regions_arr =
        yyjson_mut_obj_add_arr (doc, at_obj, "regions");
      for (int i = 0; i < at->num_regions; i++)
        {
          ZRegion *        r = at->regions[i];
          yyjson_mut_val * r_obj = yyjson_mut_arr_add_obj (doc, regions_arr);
          region_serialize_to_json (doc, r_obj, r, error);
        }
    }
  yyjson_mut_obj_add_bool (doc, at_obj, "created", at->created);
  yyjson_mut_obj_add_int (doc, at_obj, "automationMode", at->automation_mode);
  yyjson_mut_obj_add_int (doc, at_obj, "recordMode", at->record_mode);
  yyjson_mut_obj_add_bool (doc, at_obj, "visible", at->visible);
  yyjson_mut_obj_add_real (doc, at_obj, "height", at->height);
  return true;
}

bool
track_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * track_obj,
  const Track *    track,
  GError **        error)
{
  yyjson_mut_obj_add_str (doc, track_obj, "name", track->name);
  yyjson_mut_obj_add_str (doc, track_obj, "iconName", track->icon_name);
  yyjson_mut_obj_add_int (doc, track_obj, "type", track->type);
  yyjson_mut_obj_add_int (doc, track_obj, "pos", track->pos);
  yyjson_mut_obj_add_bool (doc, track_obj, "lanesVisible", track->lanes_visible);
  yyjson_mut_obj_add_bool (
    doc, track_obj, "automationVisible", track->automation_visible);
  yyjson_mut_obj_add_bool (doc, track_obj, "visible", track->visible);
  yyjson_mut_obj_add_real (doc, track_obj, "mainHeight", track->main_height);
  yyjson_mut_obj_add_bool (
    doc, track_obj, "passthroughMidiInput", track->passthrough_midi_input);
  if (track->recording)
    {
      yyjson_mut_val * recording_obj =
        yyjson_mut_obj_add_obj (doc, track_obj, "recording");
      port_serialize_to_json (doc, recording_obj, track->recording, error);
    }
  yyjson_mut_obj_add_bool (doc, track_obj, "enabled", track->enabled);
  yyjson_mut_val * color_obj = yyjson_mut_obj_add_obj (doc, track_obj, "color");
  gdk_rgba_serialize_to_json (doc, color_obj, &track->color, error);
  yyjson_mut_val * lanes_arr = yyjson_mut_obj_add_arr (doc, track_obj, "lanes");
  for (int j = 0; j < track->num_lanes; j++)
    {
      TrackLane *      lane = track->lanes[j];
      yyjson_mut_val * lane_obj = yyjson_mut_arr_add_obj (doc, lanes_arr);
      yyjson_mut_obj_add_int (doc, lane_obj, "pos", lane->pos);
      yyjson_mut_obj_add_str (doc, lane_obj, "name", lane->name);
      yyjson_mut_obj_add_real (doc, lane_obj, "height", lane->height);
      yyjson_mut_obj_add_bool (doc, lane_obj, "mute", lane->mute);
      yyjson_mut_obj_add_bool (doc, lane_obj, "solo", lane->solo);
      yyjson_mut_val * regions_arr =
        yyjson_mut_obj_add_arr (doc, lane_obj, "regions");
      for (int k = 0; k < lane->num_regions; k++)
        {
          ZRegion *        region = lane->regions[k];
          yyjson_mut_val * region_obj =
            yyjson_mut_arr_add_obj (doc, regions_arr);
          region_serialize_to_json (doc, region_obj, region, error);
        }
      yyjson_mut_obj_add_uint (doc, lane_obj, "midiCh", lane->midi_ch);
    }
  if (track->type == TRACK_TYPE_CHORD)
    {
      yyjson_mut_val * chord_regions_arr =
        yyjson_mut_obj_add_arr (doc, track_obj, "chordRegions");
      for (int j = 0; j < track->num_chord_regions; j++)
        {
          ZRegion *        r = track->chord_regions[j];
          yyjson_mut_val * r_obj =
            yyjson_mut_arr_add_obj (doc, chord_regions_arr);
          region_serialize_to_json (doc, r_obj, r, error);
        }
      yyjson_mut_val * scales_arr =
        yyjson_mut_obj_add_arr (doc, track_obj, "scaleObjects");
      for (int j = 0; j < track->num_scales; j++)
        {
          ScaleObject *    so = track->scales[j];
          yyjson_mut_val * so_obj = yyjson_mut_arr_add_obj (doc, scales_arr);
          scale_object_serialize_to_json (doc, so_obj, so, error);
        }
    }
  if (track->type == TRACK_TYPE_MARKER)
    {
      yyjson_mut_val * markers_arr =
        yyjson_mut_obj_add_arr (doc, track_obj, "markers");
      for (int j = 0; j < track->num_markers; j++)
        {
          Marker *         m = track->markers[j];
          yyjson_mut_val * m_obj = yyjson_mut_arr_add_obj (doc, markers_arr);
          marker_serialize_to_json (doc, m_obj, m, error);
        }
    }
  if (track_type_has_channel (track->type))
    {
      yyjson_mut_val * ch_obj =
        yyjson_mut_obj_add_obj (doc, track_obj, "channel");
      channel_serialize_to_json (doc, ch_obj, track->channel, error);
    }
  if (track->type == TRACK_TYPE_TEMPO)
    {
      yyjson_mut_val * port_obj =
        yyjson_mut_obj_add_obj (doc, track_obj, "bpmPort");
      port_serialize_to_json (doc, port_obj, track->bpm_port, error);
      port_obj = yyjson_mut_obj_add_obj (doc, track_obj, "beatsPerBarPort");
      port_serialize_to_json (doc, port_obj, track->beats_per_bar_port, error);
      port_obj = yyjson_mut_obj_add_obj (doc, track_obj, "beatUnitPort");
      port_serialize_to_json (doc, port_obj, track->beat_unit_port, error);
    }
  if (track->modulators)
    {
      yyjson_mut_val * modulators_arr =
        yyjson_mut_obj_add_arr (doc, track_obj, "modulators");
      for (int j = 0; j < track->num_modulators; j++)
        {
          Plugin * pl = track->modulators[j];
          yyjson_mut_val * pl_obj = yyjson_mut_arr_add_obj (doc, modulators_arr);
          plugin_serialize_to_json (doc, pl_obj, pl, error);
        }
    }
  yyjson_mut_val * modulator_macros_arr =
    yyjson_mut_obj_add_arr (doc, track_obj, "modulatorMacros");
  for (int j = 0; j < track->num_modulator_macros; j++)
    {
      ModulatorMacroProcessor * mmp = track->modulator_macros[j];
      yyjson_mut_val *          mmp_obj =
        yyjson_mut_arr_add_obj (doc, modulator_macros_arr);
      modulator_macro_processor_serialize_to_json (doc, mmp_obj, mmp, error);
    }
  yyjson_mut_obj_add_int (
    doc, track_obj, "numVisibleModulatorMacros",
    track->num_visible_modulator_macros);
  yyjson_mut_val * tp_obj = yyjson_mut_obj_add_obj (doc, track_obj, "processor");
  track_processor_serialize_to_json (doc, tp_obj, track->processor, error);
  yyjson_mut_val * atl_obj =
    yyjson_mut_obj_add_obj (doc, track_obj, "automationTracklist");
  yyjson_mut_val * ats_arr =
    yyjson_mut_obj_add_arr (doc, atl_obj, "automationTracks");
  for (int j = 0; j < track->automation_tracklist.num_ats; j++)
    {
      AutomationTrack * at = track->automation_tracklist.ats[j];
      yyjson_mut_val *  at_obj = yyjson_mut_arr_add_obj (doc, ats_arr);
      automation_track_serialize_to_json (doc, at_obj, at, error);
    }
  yyjson_mut_obj_add_int (doc, track_obj, "inSignalType", track->in_signal_type);
  yyjson_mut_obj_add_int (
    doc, track_obj, "outSignalType", track->out_signal_type);
  yyjson_mut_obj_add_uint (doc, track_obj, "midiCh", track->midi_ch);
  yyjson_mut_obj_add_str (doc, track_obj, "comment", track->comment);
  yyjson_mut_val * children_arr =
    yyjson_mut_obj_add_arr (doc, track_obj, "children");
  for (int j = 0; j < track->num_children; j++)
    {
      yyjson_mut_arr_add_uint (doc, children_arr, track->children[j]);
    }
  yyjson_mut_obj_add_bool (doc, track_obj, "frozen", track->frozen);
  yyjson_mut_obj_add_int (doc, track_obj, "poolId", track->pool_id);
  yyjson_mut_obj_add_int (doc, track_obj, "size", track->size);
  yyjson_mut_obj_add_bool (doc, track_obj, "folded", track->folded);
  yyjson_mut_obj_add_bool (
    doc, track_obj, "recordSetAutomatically", track->record_set_automatically);
  yyjson_mut_obj_add_bool (doc, track_obj, "drumMode", track->drum_mode);
  return true;
}
