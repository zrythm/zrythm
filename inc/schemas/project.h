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
 * Project schema.
 */

#ifndef __SCHEMAS_PROJECT_H__
#define __SCHEMAS_PROJECT_H__

typedef enum SelectionType_v1
{
  SELECTION_TYPE_TRACKLIST_v1,
  SELECTION_TYPE_TIMELINE_v1,
  SELECTION_TYPE_INSERT_v1,
  SELECTION_TYPE_MIDI_FX_v1,
  SELECTION_TYPE_INSTRUMENT_v1,
  SELECTION_TYPE_MODULATOR_v1,
  SELECTION_TYPE_EDITOR_v1,
} SelectionType_v1;

static const cyaml_strval_t
selection_type_strings_v1[] =
{
  { "Tracklist", SELECTION_TYPE_TRACKLIST_v1 },
  { "Timeline", SELECTION_TYPE_TIMELINE_v1 },
  { "Insert", SELECTION_TYPE_INSERT_v1 },
  { "MIDI FX", SELECTION_TYPE_MIDI_FX_v1 },
  { "Instrument", SELECTION_TYPE_INSTRUMENT_v1 },
  { "Modulator", SELECTION_TYPE_MODULATOR_v1 },
  { "Editor", SELECTION_TYPE_EDITOR_v1 },
};

typedef struct Project_v1
{
  int                  schema_version;
  char *               title;
  char *               datetime_str;
  char *               version;
  Tracklist_v1 *       tracklist; /**/
  ClipEditor_v1 *      clip_editor;
  Timeline_v1 *        timeline;
  SnapGrid_v1          snap_grid_timeline;
  QuantizeOptions_v1   quantize_opts_timeline;
  SnapGrid_v1          snap_grid_midi;
  QuantizeOptions_v1   quantize_opts_editor;
  AutomationSelections_v1 automation_selections;
  AudioSelections_v1   audio_selections;
  ChordSelections_v1   chord_selections;
  TimelineSelections_v1 timeline_selections;
  MidiArrangerSelections_v1 midi_arranger_selections;
  TracklistSelections_v1 * tracklist_selections;
  MixerSelections_v1   mixer_selections;
  RegionLinkGroupManager_v1 region_link_group_manager;
  AudioEngine_v1 *     audio_engine;
  MidiMappings_v1 *    midi_mappings;
  SelectionType_v1     last_selection;
} Project_v1;

static const cyaml_schema_field_t
  project_fields_schema_v1[] =
{
  YAML_FIELD_INT (
    Project_v1, schema_version),
  YAML_FIELD_INT (
    Project_v1, title),
  YAML_FIELD_STRING_PTR (
    Project_v1, title),
  YAML_FIELD_STRING_PTR (
    Project_v1, datetime_str),
  YAML_FIELD_STRING_PTR (
    Project_v1, version),
  YAML_FIELD_MAPPING_PTR (
    Project_v1, tracklist, tracklist_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Project_v1, clip_editor,
    clip_editor_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Project_v1, timeline,
    timeline_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project_v1, snap_grid_timeline,
    snap_grid_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project_v1, quantize_opts_timeline,
    quantize_options_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Project_v1, audio_engine, engine_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project_v1, snap_grid_midi,
    snap_grid_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project_v1, quantize_opts_editor,
    quantize_options_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project_v1, mixer_selections,
    mixer_selections_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project_v1, timeline_selections,
    timeline_selections_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project_v1, midi_arranger_selections,
    midi_arranger_selections_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project_v1, chord_selections,
    chord_selections_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project_v1, automation_selections,
    automation_selections_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project_v1, audio_selections,
    audio_selections_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Project_v1, tracklist_selections,
    tracklist_selections_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project_v1, region_link_group_manager,
    region_link_group_manager_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Project_v1, midi_mappings,
    midi_mappings_fields_schema_v1),
  /* ignore undo history */
  CYAML_FIELD_IGNORE (
    "undo_manger", CYAML_FLAG_OPTIONAL),
  YAML_FIELD_ENUM (
    Project_v1, last_selection,
    selection_type_strings_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  project_schema_v1 =
{
  YAML_VALUE_PTR (
    Project_v1, project_fields_schema_v1),
};

Project *
schemas_project_upgrade_from_v1 (
  Project_v1 * v1);

Project *
schemas_project_deserialize (
  const char * yaml);

#endif
