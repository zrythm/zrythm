// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/track.h"
#include "io/serialization/arranger_objects.h"
#include "io/serialization/channel.h"
#include "io/serialization/extra.h"
#include "io/serialization/plugin.h"
#include "io/serialization/port.h"
#include "io/serialization/track.h"
#include "utils/objects.h"

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

bool
modulator_macro_processor_deserialize_from_json (
  yyjson_doc *              doc,
  yyjson_val *              mmp_obj,
  ModulatorMacroProcessor * mmp,
  GError **                 error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (mmp_obj);
  mmp->name = g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "name")));
  yyjson_val * cv_in_obj = yyjson_obj_iter_get (&it, "cvIn");
  mmp->cv_in = object_new (Port);
  port_deserialize_from_json (doc, cv_in_obj, mmp->cv_in, error);
  yyjson_val * cv_out_obj = yyjson_obj_iter_get (&it, "cvOut");
  mmp->cv_out = object_new (Port);
  port_deserialize_from_json (doc, cv_out_obj, mmp->cv_out, error);
  yyjson_val * macro_obj = yyjson_obj_iter_get (&it, "macro");
  mmp->macro = object_new (Port);
  port_deserialize_from_json (doc, macro_obj, mmp->macro, error);
  return true;
}

bool
track_processor_deserialize_from_json (
  yyjson_doc *     doc,
  yyjson_val *     tp_obj,
  TrackProcessor * tp,
  GError **        error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (tp_obj);
  yyjson_val *    mono_obj = yyjson_obj_iter_get (&it, "mono");
  if (mono_obj)
    {
      tp->mono = object_new (Port);
      port_deserialize_from_json (doc, mono_obj, tp->mono, error);
    }
  yyjson_val * input_gain_obj = yyjson_obj_iter_get (&it, "inputGain");
  if (input_gain_obj)
    {
      tp->input_gain = object_new (Port);
      port_deserialize_from_json (doc, input_gain_obj, tp->input_gain, error);
    }
  yyjson_val * output_gain_obj = yyjson_obj_iter_get (&it, "outputGain");
  if (output_gain_obj)
    {
      tp->output_gain = object_new (Port);
      port_deserialize_from_json (doc, output_gain_obj, tp->output_gain, error);
    }
  yyjson_val * midi_in_obj = yyjson_obj_iter_get (&it, "midiIn");
  if (midi_in_obj)
    {
      tp->midi_in = object_new (Port);
      port_deserialize_from_json (doc, midi_in_obj, tp->midi_in, error);
    }
  yyjson_val * midi_out_obj = yyjson_obj_iter_get (&it, "midiOut");
  if (midi_out_obj)
    {
      tp->midi_out = object_new (Port);
      port_deserialize_from_json (doc, midi_out_obj, tp->midi_out, error);
    }
  yyjson_val * piano_roll_obj = yyjson_obj_iter_get (&it, "pianoRoll");
  if (piano_roll_obj)
    {
      tp->piano_roll = object_new (Port);
      port_deserialize_from_json (doc, piano_roll_obj, tp->piano_roll, error);
    }
  yyjson_val * monitor_audio_obj = yyjson_obj_iter_get (&it, "monitorAudio");
  if (monitor_audio_obj)
    {
      tp->monitor_audio = object_new (Port);
      port_deserialize_from_json (
        doc, monitor_audio_obj, tp->monitor_audio, error);
    }
  yyjson_val * stereo_in_obj = yyjson_obj_iter_get (&it, "stereoIn");
  if (stereo_in_obj)
    {
      tp->stereo_in = object_new (Port);
      stereo_ports_deserialize_from_json (
        doc, stereo_in_obj, tp->stereo_in, error);
    }
  yyjson_val * stereo_out_obj = yyjson_obj_iter_get (&it, "stereoOut");
  if (stereo_out_obj)
    {
      tp->stereo_out = object_new (StereoPorts);
      stereo_ports_deserialize_from_json (
        doc, stereo_out_obj, tp->stereo_out, error);
    }
  yyjson_val * midi_cc_arr = yyjson_obj_iter_get (&it, "midiCc");
  if (midi_cc_arr)
    {
      yyjson_arr_iter midi_cc_it = yyjson_arr_iter_with (midi_cc_arr);
      yyjson_val *    midi_cc_obj = NULL;
      size_t          count = 0;
      while ((midi_cc_obj = yyjson_arr_iter_next (&midi_cc_it)))
        {
          Port * port = object_new (Port);
          tp->midi_cc[count++] = port;
          port_deserialize_from_json (doc, midi_cc_obj, port, error);
        }
      yyjson_val *    pitch_bend_arr = yyjson_obj_iter_get (&it, "pitchBend");
      yyjson_arr_iter pitch_bend_it = yyjson_arr_iter_with (pitch_bend_arr);
      yyjson_val *    pitch_bend_obj = NULL;
      count = 0;
      while ((pitch_bend_obj = yyjson_arr_iter_next (&pitch_bend_it)))
        {
          Port * port = object_new (Port);
          tp->pitch_bend[count++] = port;
          port_deserialize_from_json (doc, pitch_bend_obj, port, error);
        }
      yyjson_val * poly_key_pressure_arr =
        yyjson_obj_iter_get (&it, "polyKeyPressure");
      yyjson_arr_iter poly_key_pressure_it =
        yyjson_arr_iter_with (poly_key_pressure_arr);
      yyjson_val * poly_key_pressure_obj = NULL;
      count = 0;
      while (
        (poly_key_pressure_obj = yyjson_arr_iter_next (&poly_key_pressure_it)))
        {
          Port * port = object_new (Port);
          tp->poly_key_pressure[count++] = port;
          port_deserialize_from_json (doc, poly_key_pressure_obj, port, error);
        }
      yyjson_val * channel_pressure_arr =
        yyjson_obj_iter_get (&it, "channelPressure");
      yyjson_arr_iter channel_pressure_it =
        yyjson_arr_iter_with (channel_pressure_arr);
      yyjson_val * channel_pressure_obj = NULL;
      count = 0;
      while (
        (channel_pressure_obj = yyjson_arr_iter_next (&channel_pressure_it)))
        {
          Port * port = object_new (Port);
          tp->channel_pressure[count++] = port;
          port_deserialize_from_json (doc, channel_pressure_obj, port, error);
        }
    }
  return true;
}

bool
automation_track_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      at_obj,
  AutomationTrack * at,
  GError **         error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (at_obj);
  at->index = yyjson_get_int (yyjson_obj_iter_get (&it, "index"));
  yyjson_val * port_id_obj = yyjson_obj_iter_get (&it, "portId");
  port_identifier_deserialize_from_json (doc, port_id_obj, &at->port_id, error);
  yyjson_val * regions_arr = yyjson_obj_iter_get (&it, "regions");
  if (regions_arr)
    {
      at->regions_size = yyjson_arr_size (regions_arr);
      if (at->regions_size > 0)
        {
          at->regions = g_malloc0_n (at->regions_size, sizeof (ZRegion *));
          yyjson_arr_iter region_it = yyjson_arr_iter_with (regions_arr);
          yyjson_val *    r_obj = NULL;
          while ((r_obj = yyjson_arr_iter_next (&region_it)))
            {
              ZRegion * r = object_new (ZRegion);
              at->regions[at->num_regions++] = r;
              region_deserialize_from_json (doc, r_obj, r, error);
            }
        }
    }
  at->created = yyjson_get_bool (yyjson_obj_iter_get (&it, "created"));
  at->automation_mode = (AutomationMode) yyjson_get_int (
    yyjson_obj_iter_get (&it, "automationMode"));
  at->record_mode = (AutomationRecordMode) yyjson_get_int (
    yyjson_obj_iter_get (&it, "recordMode"));
  at->visible = yyjson_get_bool (yyjson_obj_iter_get (&it, "visible"));
  at->height = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "height"));
  return true;
}

bool
track_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * track_obj,
  Track *      track,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (track_obj);
  track->name = g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "name")));
  track->icon_name =
    g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "iconName")));
  track->type = (TrackType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  track->pos = yyjson_get_int (yyjson_obj_iter_get (&it, "pos"));
  track->lanes_visible =
    yyjson_get_bool (yyjson_obj_iter_get (&it, "lanesVisible"));
  track->automation_visible =
    yyjson_get_bool (yyjson_obj_iter_get (&it, "automationVisible"));
  track->visible = yyjson_get_bool (yyjson_obj_iter_get (&it, "visible"));
  track->main_height = yyjson_get_real (yyjson_obj_iter_get (&it, "mainHeight"));
  track->passthrough_midi_input =
    yyjson_get_bool (yyjson_obj_iter_get (&it, "passthroughMidiInput"));
  yyjson_val * recording_obj = yyjson_obj_iter_get (&it, "recording");
  if (recording_obj)
    {
      track->recording = object_new (Port);
      port_deserialize_from_json (doc, recording_obj, track->recording, error);
    }
  track->enabled = yyjson_get_bool (yyjson_obj_iter_get (&it, "enabled"));
  gdk_rgba_deserialize_from_json (
    doc, yyjson_obj_iter_get (&it, "color"), &track->color, error);
  yyjson_val * lanes_arr = yyjson_obj_iter_get (&it, "lanes");
  track->lanes_size = yyjson_arr_size (lanes_arr);
  if (track->lanes_size > 0)
    {
      track->lanes = g_malloc0_n (track->lanes_size, sizeof (TrackLane *));
      yyjson_arr_iter lanes_it = yyjson_arr_iter_with (lanes_arr);
      yyjson_val *    lane_obj = NULL;
      while ((lane_obj = yyjson_arr_iter_next (&lanes_it)))
        {
          yyjson_obj_iter lane_it = yyjson_obj_iter_with (lane_obj);
          TrackLane *     lane = object_new (TrackLane);
          track->lanes[track->num_lanes++] = lane;
          lane->pos = yyjson_get_int (yyjson_obj_iter_get (&lane_it, "pos"));
          lane->name =
            g_strdup (yyjson_get_str (yyjson_obj_iter_get (&lane_it, "name")));
          lane->height =
            yyjson_get_real (yyjson_obj_iter_get (&lane_it, "height"));
          lane->mute = yyjson_get_bool (yyjson_obj_iter_get (&lane_it, "mute"));
          lane->solo = yyjson_get_bool (yyjson_obj_iter_get (&lane_it, "solo"));
          yyjson_val * regions_arr = yyjson_obj_iter_get (&lane_it, "regions");
          lane->regions_size = yyjson_arr_size (regions_arr);
          if (lane->regions_size > 0)
            {
              lane->regions =
                g_malloc0_n (lane->regions_size, sizeof (ZRegion *));
              yyjson_arr_iter regions_it = yyjson_arr_iter_with (regions_arr);
              yyjson_val *    region_obj = NULL;
              while ((region_obj = yyjson_arr_iter_next (&regions_it)))
                {
                  ZRegion * r = object_new (ZRegion);
                  lane->regions[lane->num_regions++] = r;
                  region_deserialize_from_json (doc, region_obj, r, error);
                }
            } /* endif regions_size > 0 */
          lane->midi_ch = (uint8_t) yyjson_get_uint (
            yyjson_obj_iter_get (&lane_it, "midiCh"));
        } /* end lanes */
    }
  if (track->type == TRACK_TYPE_CHORD)
    {
      yyjson_val * chord_regions_arr = yyjson_obj_iter_get (&it, "chordRegions");
      track->chord_regions_size = yyjson_arr_size (chord_regions_arr);
      if (track->chord_regions_size > 0)
        {
          track->chord_regions =
            g_malloc0_n (track->chord_regions_size, sizeof (ZRegion *));
          yyjson_arr_iter chord_region_it =
            yyjson_arr_iter_with (chord_regions_arr);
          yyjson_val * region_obj = NULL;
          while ((region_obj = yyjson_arr_iter_next (&chord_region_it)))
            {
              ZRegion * r = object_new (ZRegion);
              track->chord_regions[track->num_chord_regions++] = r;
              region_deserialize_from_json (doc, region_obj, r, error);
            }
        } /* endif chord_regions_size > 0 */
      yyjson_val * scales_arr = yyjson_obj_iter_get (&it, "scaleObjects");
      track->scales_size = yyjson_arr_size (scales_arr);
      if (track->scales_size > 0)
        {
          track->scales =
            g_malloc0_n (track->scales_size, sizeof (ScaleObject *));
          yyjson_arr_iter scale_it = yyjson_arr_iter_with (scales_arr);
          yyjson_val *    scale_obj = NULL;
          while ((scale_obj = yyjson_arr_iter_next (&scale_it)))
            {
              ScaleObject * so = object_new (ScaleObject);
              track->scales[track->num_scales++] = so;
              scale_object_deserialize_from_json (doc, scale_obj, so, error);
            }
        } /* endif scalse_size > 0 */
    }     /* end if track type chord */
  if (track->type == TRACK_TYPE_MARKER)
    {
      yyjson_val * markers_arr = yyjson_obj_iter_get (&it, "markers");
      track->markers_size = yyjson_arr_size (markers_arr);
      if (track->markers_size > 0)
        {
          track->markers = g_malloc0_n (track->markers_size, sizeof (Marker *));
          yyjson_arr_iter markers_it = yyjson_arr_iter_with (markers_arr);
          yyjson_val *    marker_obj = NULL;
          while ((marker_obj = yyjson_arr_iter_next (&markers_it)))
            {
              Marker * m = object_new (Marker);
              track->markers[track->num_markers++] = m;
              marker_deserialize_from_json (doc, marker_obj, m, error);
            }
        } /* endif markers_size > 0 */
    }
  if (track_type_has_channel (track->type))
    {
      yyjson_val * ch_obj = yyjson_obj_iter_get (&it, "channel");
      track->channel = object_new (Channel);
      channel_deserialize_from_json (doc, ch_obj, track->channel, error);
    }
  if (track->type == TRACK_TYPE_TEMPO)
    {
      yyjson_val * port_obj = yyjson_obj_iter_get (&it, "bpmPort");
      track->bpm_port = object_new (Port);
      port_deserialize_from_json (doc, port_obj, track->bpm_port, error);
      port_obj = yyjson_obj_iter_get (&it, "beatsPerBarPort");
      track->beats_per_bar_port = object_new (Port);
      port_deserialize_from_json (
        doc, port_obj, track->beats_per_bar_port, error);
      port_obj = yyjson_obj_iter_get (&it, "beatUnitPort");
      track->beat_unit_port = object_new (Port);
      port_deserialize_from_json (doc, port_obj, track->beat_unit_port, error);
    }
  yyjson_val * modulators_arr = yyjson_obj_iter_get (&it, "modulators");
  if (modulators_arr)
    {
      track->modulators_size = yyjson_arr_size (modulators_arr);
      if (track->modulators_size > 0)
        {
          track->modulators =
            g_malloc0_n (track->modulators_size, sizeof (Plugin *));
          yyjson_arr_iter modulator_it = yyjson_arr_iter_with (modulators_arr);
          yyjson_val *    modulator_obj = NULL;
          while ((modulator_obj = yyjson_arr_iter_next (&modulator_it)))
            {
              Plugin * pl = object_new (Plugin);
              track->modulators[track->num_modulators++] = pl;
              plugin_deserialize_from_json (doc, modulator_obj, pl, error);
            }
        }
    } /* end modulators array */
  yyjson_val * modulator_macros_arr =
    yyjson_obj_iter_get (&it, "modulatorMacros");
  yyjson_arr_iter modulator_macro_it =
    yyjson_arr_iter_with (modulator_macros_arr);
  yyjson_val * mmp_obj = NULL;
  while ((mmp_obj = yyjson_arr_iter_next (&modulator_macro_it)))
    {
      ModulatorMacroProcessor * mmp = object_new (ModulatorMacroProcessor);
      track->modulator_macros[track->num_modulator_macros++] = mmp;
      modulator_macro_processor_deserialize_from_json (doc, mmp_obj, mmp, error);
    }
  track->num_visible_modulator_macros =
    yyjson_get_int (yyjson_obj_iter_get (&it, "numVisibleModulatorMacros"));
  yyjson_val * tp_obj = yyjson_obj_iter_get (&it, "processor");
  track->processor = object_new (TrackProcessor);
  track_processor_deserialize_from_json (doc, tp_obj, track->processor, error);
  yyjson_val *    atl_obj = yyjson_obj_iter_get (&it, "automationTracklist");
  yyjson_obj_iter atl_it = yyjson_obj_iter_with (atl_obj);
  yyjson_val *    ats_arr = yyjson_obj_iter_get (&atl_it, "automationTracks");
  track->automation_tracklist.ats_size = yyjson_arr_size (ats_arr);
  if (track->automation_tracklist.ats_size > 0)
    {
      track->automation_tracklist.ats = g_malloc0_n (
        track->automation_tracklist.ats_size, sizeof (AutomationTrack *));
      yyjson_arr_iter at_it = yyjson_arr_iter_with (ats_arr);
      yyjson_val *    at_obj = NULL;
      while ((at_obj = yyjson_arr_iter_next (&at_it)))
        {
          AutomationTrack * at = object_new (AutomationTrack);
          track->automation_tracklist
            .ats[track->automation_tracklist.num_ats++] = at;
          automation_track_deserialize_from_json (doc, at_obj, at, error);
        }
    }
  track->in_signal_type =
    (PortType) yyjson_get_int (yyjson_obj_iter_get (&it, "inSignalType"));
  track->out_signal_type =
    (PortType) yyjson_get_int (yyjson_obj_iter_get (&it, "outSignalType"));
  track->midi_ch =
    (uint8_t) yyjson_get_uint (yyjson_obj_iter_get (&it, "midiCh"));
  track->comment =
    g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "comment")));
  yyjson_val * children_arr = yyjson_obj_iter_get (&it, "children");
  track->children_size = yyjson_arr_size (children_arr);
  if (track->children_size > 0)
    {
      track->children =
        g_malloc0_n (track->children_size, sizeof (unsigned int));
      yyjson_arr_iter children_it = yyjson_arr_iter_with (children_arr);
      yyjson_val *    child_obj = NULL;
      while ((child_obj = yyjson_arr_iter_next (&children_it)))
        {
          track->children[track->num_children++] = yyjson_get_uint (child_obj);
        }
    }
  track->frozen = yyjson_get_bool (yyjson_obj_iter_get (&it, "frozen"));
  track->pool_id = yyjson_get_int (yyjson_obj_iter_get (&it, "poolId"));
  track->size = yyjson_get_int (yyjson_obj_iter_get (&it, "size"));
  track->folded = yyjson_get_bool (yyjson_obj_iter_get (&it, "folded"));
  track->record_set_automatically =
    yyjson_get_bool (yyjson_obj_iter_get (&it, "recordSetAutomatically"));
  track->drum_mode = yyjson_get_bool (yyjson_obj_iter_get (&it, "drumMode"));
  return true;
}
