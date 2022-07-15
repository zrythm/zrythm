/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * The backend for a timeline track.
 */

#ifndef __SCHEMAS_AUDIO_TRACK_H__
#define __SCHEMAS_AUDIO_TRACK_H__

typedef enum TrackType_v1
{
  TRACK_TYPE_INSTRUMENT_v1,
  TRACK_TYPE_AUDIO_v1,
  TRACK_TYPE_MASTER_v1,
  TRACK_TYPE_CHORD_v1,
  TRACK_TYPE_MARKER_v1,
  TRACK_TYPE_TEMPO_v1,
  TRACK_TYPE_MODULATOR_v1,
  TRACK_TYPE_AUDIO_BUS_v1,
  TRACK_TYPE_AUDIO_GROUP_v1,
  TRACK_TYPE_MIDI_v1,
  TRACK_TYPE_MIDI_BUS_v1,
  TRACK_TYPE_MIDI_GROUP_v1,
  TRACK_TYPE_FOLDER_v1,
} TrackType_v1;

static const cyaml_strval_t track_type_strings_v1[] = {
  {"Instrument",   TRACK_TYPE_INSTRUMENT_v1 },
  { "Audio",       TRACK_TYPE_AUDIO_v1      },
  { "Master",      TRACK_TYPE_MASTER_v1     },
  { "Chord",       TRACK_TYPE_CHORD_v1      },
  { "Marker",      TRACK_TYPE_MARKER_v1     },
  { "Tempo",       TRACK_TYPE_TEMPO_v1      },
  { "Modulator",   TRACK_TYPE_MODULATOR_v1  },
  { "Audio FX",    TRACK_TYPE_AUDIO_BUS_v1  },
  { "Audio Group", TRACK_TYPE_AUDIO_GROUP_v1},
  { "MIDI",        TRACK_TYPE_MIDI_v1       },
  { "MIDI FX",     TRACK_TYPE_MIDI_BUS_v1   },
  { "MIDI Group",  TRACK_TYPE_MIDI_GROUP_v1 },
  { "Folder",      TRACK_TYPE_FOLDER_v1     },
};

typedef struct Track_v1
{
  int                          schema_version;
  int                          pos;
  TrackType_v1                 type;
  char *                       name;
  char *                       icon_name;
  bool                         automation_visible;
  bool                         lanes_visible;
  bool                         visible;
  double                       main_height;
  Port_v1 *                    recording;
  bool                         active;
  GdkRGBA                      color;
  TrackLane_v1 **              lanes;
  int                          num_lanes;
  size_t                       lanes_size;
  uint8_t                      midi_ch;
  int                          passthrough_midi_input;
  ZRegion_v1 *                 recording_region;
  bool                         recording_start_sent;
  bool                         recording_stop_sent;
  bool                         recording_paused;
  int                          last_lane_idx;
  ZRegion_v1 **                chord_regions;
  int                          num_chord_regions;
  size_t                       chord_regions_size;
  ScaleObject_v1 **            scales;
  int                          num_scales;
  size_t                       scales_size;
  Marker_v1 **                 markers;
  int                          num_markers;
  size_t                       markers_size;
  Port_v1 *                    bpm_port;
  Port_v1 *                    time_sig_port;
  Plugin_v1 **                 modulators;
  int                          num_modulators;
  size_t                       modulators_size;
  ModulatorMacroProcessor_v1 * modulator_macros[128];
  int                          num_modulator_macros;
  int                          num_visible_modulator_macros;
  Channel_v1 *                 channel;
  TrackProcessor_v1 *          processor;
  AutomationTracklist_v1       automation_tracklist;
  bool                         trigger_midi_activity;
  PortType_v1                  in_signal_type;
  PortType_v1                  out_signal_type;
  char *                       comment;
  bool                         bounce;
  int *                        children;
  int                          num_children;
  size_t                       children_size;
  bool                         frozen;
  int                          pool_id;
  int                          magic;
  bool                         is_project;
} Track_v1;

static const cyaml_schema_field_t track_fields_schema_v1[] = {
  YAML_FIELD_INT (Track_v1, schema_version),
  YAML_FIELD_STRING_PTR (Track_v1, name),
  YAML_FIELD_STRING_PTR (Track_v1, icon_name),
  YAML_FIELD_ENUM (Track_v1, type, track_type_strings_v1),
  YAML_FIELD_INT (Track_v1, pos),
  YAML_FIELD_INT (Track_v1, lanes_visible),
  YAML_FIELD_INT (Track_v1, automation_visible),
  YAML_FIELD_INT (Track_v1, visible),
  YAML_FIELD_FLOAT (Track_v1, main_height),
  YAML_FIELD_INT (Track_v1, passthrough_midi_input),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Track_v1,
    recording,
    port_fields_schema_v1),
  YAML_FIELD_INT (Track_v1, enabled_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Track_v1,
    color,
    gdk_rgba_fields_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT (
    Track_v1,
    lanes,
    track_lane_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    Track_v1,
    chord_regions,
    region_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    Track_v1,
    scales,
    scale_object_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    Track_v1,
    markers,
    marker_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Track_v1,
    channel,
    channel_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Track_v1,
    bpm_port,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Track_v1,
    time_sig_port,
    port_fields_schema_v1),
  YAML_FIELD_DYN_ARRAY_VAR_COUNT (
    Track_v1,
    modulators,
    plugin_schema_v1),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    Track_v1,
    modulator_macros,
    modulator_macro_processor_schema_v1),
  YAML_FIELD_INT (Track_v1, num_visible_modulator_macros),
  YAML_FIELD_MAPPING_PTR (
    Track_v1,
    processor,
    track_processor_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Track_v1,
    automation_tracklist,
    automation_tracklist_fields_schema_v1),
  YAML_FIELD_ENUM (
    Track_v1,
    in_signal_type,
    port_type_strings_v1),
  YAML_FIELD_ENUM (
    Track_v1,
    out_signal_type,
    port_type_strings_v1),
  YAML_FIELD_UINT (Track_v1, midi_ch),
  YAML_FIELD_STRING_PTR (Track_v1, comment),
  YAML_FIELD_DYN_ARRAY_VAR_COUNT_PRIMITIVES (
    Track_v1,
    children,
    int_schema),
  YAML_FIELD_INT (Track_v1, frozen),
  YAML_FIELD_INT (Track_v1, pool_id),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t track_schema_v1 = {
  YAML_VALUE_PTR (Track_v1, track_fields_schema_v1),
};

#endif
