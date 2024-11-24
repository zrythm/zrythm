// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Project schema.
 */

#ifndef __SCHEMAS_PROJECT_H__
#define __SCHEMAS_PROJECT_H__

#include "zrythm-config.h"

#ifdef HAVE_CYAML

#  include "gui/backend/backend/cyaml_schemas/dsp/engine.h"
#  include "gui/backend/backend/cyaml_schemas/dsp/midi_mapping.h"
#  include "gui/backend/backend/cyaml_schemas/dsp/port_connections_manager.h"
#  include "gui/backend/backend/cyaml_schemas/dsp/quantize_options.h"
#  include "gui/backend/backend/cyaml_schemas/dsp/region_link_group_manager.h"
#  include "gui/backend/backend/cyaml_schemas/dsp/snap_grid.h"
#  include "gui/backend/backend/cyaml_schemas/dsp/tracklist.h"
#  include "gui/backend/backend/cyaml_schemas/gui/backend/audio_selections.h"
#  include "gui/backend/backend/cyaml_schemas/gui/backend/automation_selections.h"
#  include "gui/backend/backend/cyaml_schemas/gui/backend/chord_selections.h"
#  include "gui/backend/backend/cyaml_schemas/gui/backend/clip_editor.h"
#  include "gui/backend/backend/cyaml_schemas/gui/backend/midi_arranger_selections.h"
#  include "gui/backend/backend/cyaml_schemas/gui/backend/mixer_selections.h"
#  include "gui/backend/backend/cyaml_schemas/gui/backend/timeline.h"
#  include "gui/backend/backend/cyaml_schemas/gui/backend/timeline_selections.h"
#  include "gui/backend/backend/cyaml_schemas/gui/backend/tracklist_selections.h"

typedef enum SelectionType_v1
{
  Z_PROJECT_SELECTION_TYPE_TRACKLIST_v1,
  Z_PROJECT_SELECTION_TYPE_TIMELINE_v1,
  Z_PROJECT_SELECTION_TYPE_INSERT_v1,
  Z_PROJECT_SELECTION_TYPE_MIDI_FX_v1,
  Z_PROJECT_SELECTION_TYPE_INSTRUMENT_v1,
  Z_PROJECT_SELECTION_TYPE_MODULATOR_v1,
  Z_PROJECT_SELECTION_TYPE_EDITOR_v1,
} SelectionType_v1;

static const cyaml_strval_t selection_type_strings_v1[] = {
  { "Tracklist",  Z_PROJECT_SELECTION_TYPE_TRACKLIST_v1  },
  { "Timeline",   Z_PROJECT_SELECTION_TYPE_TIMELINE_v1   },
  { "Insert",     Z_PROJECT_SELECTION_TYPE_INSERT_v1     },
  { "MIDI FX",    Z_PROJECT_SELECTION_TYPE_MIDI_FX_v1    },
  { "Instrument", Z_PROJECT_SELECTION_TYPE_INSTRUMENT_v1 },
  { "Modulator",  Z_PROJECT_SELECTION_TYPE_MODULATOR_v1  },
  { "Editor",     Z_PROJECT_SELECTION_TYPE_EDITOR_v1     },
};

typedef struct Project_v1
{
  int                         schema_version;
  char *                      title;
  char *                      datetime_str;
  char *                      version;
  Tracklist_v1 *              tracklist;
  ClipEditor_v1 *             clip_editor;
  Timeline_v1 *               timeline;
  SnapGrid_v1 *               snap_grid_timeline;
  QuantizeOptions_v1 *        quantize_opts_timeline;
  SnapGrid_v1 *               snap_grid_editor;
  QuantizeOptions_v1 *        quantize_opts_editor;
  TracklistSelections_v1 *    tracklist_selections;
  RegionLinkGroupManager_v1 * region_link_group_manager;
  PortConnectionsManager_v1 * port_connections_manager;
  AudioEngine_v1 *            audio_engine;
  MidiMappings_v1 *           midi_mappings;
  SelectionType_v1            last_selection;
} Project_v1;

static const cyaml_schema_field_t project_fields_schema_v1[] = {
  YAML_FIELD_INT (Project_v1, schema_version),
  YAML_FIELD_STRING_PTR (Project_v1, title),
  YAML_FIELD_STRING_PTR (Project_v1, datetime_str),
  YAML_FIELD_STRING_PTR (Project_v1, version),
  YAML_FIELD_MAPPING_PTR (Project_v1, tracklist, tracklist_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Project_v1, clip_editor, clip_editor_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Project_v1, timeline, timeline_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Project_v1,
    snap_grid_timeline,
    snap_grid_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Project_v1, snap_grid_editor, snap_grid_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Project_v1,
    quantize_opts_timeline,
    quantize_options_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Project_v1,
    quantize_opts_editor,
    quantize_options_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Project_v1, audio_engine, engine_fields_schema_v1),
  YAML_FIELD_IGNORE_OPT ("mixer_selections"),
  YAML_FIELD_IGNORE_OPT ("timeline_selections"),
  YAML_FIELD_IGNORE_OPT ("midi_arranger_selections"),
  YAML_FIELD_IGNORE_OPT ("automation_selections"),
  YAML_FIELD_IGNORE_OPT ("chord_selections"),
  YAML_FIELD_IGNORE_OPT ("audio_selections"),
  YAML_FIELD_MAPPING_PTR (
    Project_v1,
    tracklist_selections,
    tracklist_selections_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Project_v1,
    region_link_group_manager,
    region_link_group_manager_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Project_v1,
    port_connections_manager,
    port_connections_manager_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Project_v1, midi_mappings, midi_mappings_fields_schema_v1),
  /* ignore undo history */
  YAML_FIELD_IGNORE_OPT ("undo_manager"),
  YAML_FIELD_ENUM (Project_v1, last_selection, selection_type_strings_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t project_schema_v1 = {
  YAML_VALUE_PTR (Project_v1, project_fields_schema_v1),
};

/** Latest YAML project. */
typedef struct Project_v5
{
  int                         schema_version;
  char *                      title;
  char *                      datetime_str;
  char *                      version;
  Tracklist_v2 *              tracklist;
  ClipEditor_v1 *             clip_editor;
  Timeline_v1 *               timeline;
  SnapGrid_v1 *               snap_grid_timeline;
  QuantizeOptions_v1 *        quantize_opts_timeline;
  SnapGrid_v1 *               snap_grid_editor;
  QuantizeOptions_v1 *        quantize_opts_editor;
  TracklistSelections_v2 *    tracklist_selections;
  RegionLinkGroupManager_v1 * region_link_group_manager;
  PortConnectionsManager_v1 * port_connections_manager;
  AudioEngine_v2 *            audio_engine;
  MidiMappings_v1 *           midi_mappings;
  SelectionType_v1            last_selection;
} Project_v5;

static const cyaml_schema_field_t project_fields_schema_v5[] = {
  YAML_FIELD_INT (Project_v5, schema_version),
  YAML_FIELD_STRING_PTR (Project_v5, title),
  YAML_FIELD_STRING_PTR (Project_v5, datetime_str),
  YAML_FIELD_STRING_PTR (Project_v5, version),
  YAML_FIELD_MAPPING_PTR (Project_v5, tracklist, tracklist_fields_schema_v2),
  YAML_FIELD_MAPPING_PTR (Project_v5, clip_editor, clip_editor_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Project_v5, timeline, timeline_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Project_v5,
    snap_grid_timeline,
    snap_grid_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Project_v5, snap_grid_editor, snap_grid_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Project_v5,
    quantize_opts_timeline,
    quantize_options_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Project_v5,
    quantize_opts_editor,
    quantize_options_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Project_v5, audio_engine, engine_fields_schema_v2),
  YAML_FIELD_IGNORE_OPT ("mixer_selections"),
  YAML_FIELD_IGNORE_OPT ("timeline_selections"),
  YAML_FIELD_IGNORE_OPT ("midi_arranger_selections"),
  YAML_FIELD_IGNORE_OPT ("automation_selections"),
  YAML_FIELD_IGNORE_OPT ("chord_selections"),
  YAML_FIELD_IGNORE_OPT ("audio_selections"),
  YAML_FIELD_MAPPING_PTR (
    Project_v5,
    tracklist_selections,
    tracklist_selections_fields_schema_v2),
  YAML_FIELD_MAPPING_PTR (
    Project_v5,
    region_link_group_manager,
    region_link_group_manager_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Project_v5,
    port_connections_manager,
    port_connections_manager_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Project_v5, midi_mappings, midi_mappings_fields_schema_v1),
  /* ignore undo history */
  YAML_FIELD_IGNORE_OPT ("undo_manager"),
  YAML_FIELD_ENUM (Project_v5, last_selection, selection_type_strings_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t project_schema_v5 = {
  YAML_VALUE_PTR (Project_v5, project_fields_schema_v5),
};

/**
 * Serializes the project into a JSON string.
 *
 * Used when converting old projects to the newer JSON format.
 *
 * @throw ZrythmException on error.
 */
zrythm::utils::string::CStringRAII
project_v5_serialize_to_json_str (const Project_v5 * prj);

#endif /* HAVE_CYAML */

#endif /* __SCHEMAS_PROJECT_H__ */
