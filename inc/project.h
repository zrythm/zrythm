/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * A project (or song), containing all the project
 * data as opposed to zrythm_app.h which manages
 * global things like plugin descriptors and global
 * settings.
 */

#ifndef __PROJECT_H__
#define __PROJECT_H__

#include "actions/undo_manager.h"
#include "audio/automation_curve.h"
#include "audio/automation_lane.h"
#include "audio/automation_point.h"
#include "audio/automation_track.h"
#include "audio/engine.h"
#include "audio/midi_note.h"
#include "audio/port.h"
#include "audio/quantize.h"
#include "audio/region.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/backend/mixer_selections.h"
#include "gui/backend/timeline_selections.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/backend/tool.h"
#include "plugins/plugin.h"
#include "zrythm.h"

#include <gtk/gtk.h>

#define PROJECT                 ZRYTHM->project
#define DEFAULT_PROJECT_NAME    "Untitled Project"
#define PROJECT_FILE            "project.yml"
#define PROJECT_REGIONS_FILE    "regions.yml"
#define PROJECT_PORTS_FILE      "ports.yml"
#define PROJECT_REGIONS_DIR     "Regions"
#define PROJECT_STATES_DIR      "states"
#define PROJECT_EXPORTS_DIR     "exports"
#define PROJECT_AUDIO_DIR       "audio"

typedef struct Timeline Timeline;
typedef struct Transport Transport;
typedef struct Port Port;
typedef struct Channel Channel;
typedef struct Plugin Plugin;
typedef struct Track Track;
typedef struct Region Region;
typedef struct AutomationPoint AutomationPoint;
typedef struct AutomationCurve AutomationCurve;
typedef struct MidiNote MidiNote;
typedef struct Track ChordTrack;
typedef struct Tracklist Tracklist;

typedef struct Project
{
  char              * title; ///< project title

  char              * dir; ///< path to save the project
  char              * project_file_path; ///< for convenience
  char              * regions_file_path;
  char              * regions_dir;
  char              * states_dir;
  char              * exports_dir;
  char *              audio_dir;

  UndoManager         undo_manager;

  Tracklist          tracklist;

  /** Backend for the widget. */
  ClipEditor         clip_editor;

  SnapGrid           snap_grid_timeline; ///< snap/grid info for timeline
  Quantize           quantize_timeline;
  SnapGrid          snap_grid_midi; ///< snap/grid info for midi/audio editor
  Quantize           quantize_midi;

  TimelineSelections       timeline_selections;
  MidiArrangerSelections   midi_arranger_selections;
  TracklistSelections   tracklist_selections;

  /**
   * Plugin selections in the Mixer.
   */
  MixerSelections    mixer_selections;

  /** Zoom levels. TODO & move to clip_editor */
  double             timeline_zoom;
  double             piano_roll_zoom;

  /**
   * Selected range.
   *
   * This is 2 points instead of start/end because the 2nd
   * point can be dragged past the 1st point so the order
   * gets swapped.
   *
   * Should be compared each time to see which one is first.
   */
  Position           range_1;
  Position           range_2;
  int                has_range; ///< if 0 range is not displayed

  /**
   * The audio backend
   */
  AudioEngine        audio_engine;

  /**
   * Currently selected tool (select - normal,
   * select - stretch, edit, delete, ramp, audition)
   */
  Tool               tool;

  /**
   * For serializing/deserializing.
   *
   * These are stored here when created so that they can
   * easily be serialized/deserialized.
   *
   * Explanation: the aggregated objects are to be
   * serialized because the normal arrays can
   * contain NULLs which means they can't be
   * serialized as is.
   *
   * Note: when objects get deleted they are responsible
   * for setting their position to NULL.
   */
  Plugin *          aggregated_plugins[2000];
  int               num_aggregated_plugins;
  Plugin *          plugins[8000];
  int               num_plugins;
  Region *          aggregated_regions [10000];
  int               num_aggregated_regions;
  Region *          regions[30000];
  int               num_regions;
  Automatable *     aggregated_automatables[60000];
  int               num_aggregated_automatables;
  Automatable *     automatables[20000];
  int               num_automatables;
  AutomationPoint * aggregated_automation_points[30000];
  int               num_aggregated_automation_points;
  AutomationPoint * automation_points[30000];
  int               num_automation_points;
  AutomationCurve * aggregated_automation_curves[30000];
  int               num_aggregated_automation_curves;
  AutomationCurve * automation_curves[30000];
  int               num_automation_curves;
  AutomationTrack * aggregated_automation_tracks[50000];
  int               num_aggregated_automation_tracks;
  AutomationTrack * automation_tracks[50000];
  int               num_automation_tracks;
  AutomationLane *  aggregated_automation_lanes[50000];
  int               num_aggregated_automation_lanes;
  AutomationLane *  automation_lanes[50000];
  int               num_automation_lanes;
  MidiNote *        aggregated_midi_notes[30000];
  int               num_aggregated_midi_notes;
  MidiNote *        midi_notes[100000];
  int               num_midi_notes;
  ZChord *           aggregated_chords[600];
  int               num_aggregated_chords;
  ZChord *           chords[60000];
  int               num_chords;
  Track *           aggregated_tracks[3000];
  int               num_aggregated_tracks;
  Track *           tracks[50000];
  int               num_tracks;
  Port *            aggregated_ports[600000];
  int               num_aggregated_ports;
  Port *            ports[600000];
  int               num_ports;

  /**
   * The chord track.
   *
   * This is a pointer because track_new, etc. do some things
   * when called.
   */
  int               chord_track_id;
  ChordTrack *      chord_track;

  /**
   * If a project is currently loaded or not.
   *
   * This is useful so that we know if we need to tear down
   * when loading a new project while another one is loaded.
   */
  int               loaded;
} Project;

static const cyaml_schema_field_t
  project_fields_schema[] =
{
  CYAML_FIELD_STRING_PTR (
    "title", CYAML_FLAG_POINTER,
    Project, title,
    0, CYAML_UNLIMITED),
  CYAML_FIELD_MAPPING (
    "tracklist", CYAML_FLAG_DEFAULT,
    Project, tracklist, tracklist_fields_schema),
  CYAML_FIELD_MAPPING (
    "clip_editor", CYAML_FLAG_DEFAULT,
    Project, clip_editor, clip_editor_fields_schema),
  CYAML_FIELD_MAPPING (
    "snap_grid_timeline", CYAML_FLAG_DEFAULT,
    Project, snap_grid_timeline, snap_grid_fields_schema),
  CYAML_FIELD_MAPPING (
    "quantize_timeline", CYAML_FLAG_DEFAULT,
    Project, quantize_timeline, quantize_fields_schema),
  CYAML_FIELD_MAPPING (
    "audio_engine", CYAML_FLAG_DEFAULT,
    Project, audio_engine, engine_fields_schema),
  CYAML_FIELD_MAPPING (
    "snap_grid_midi", CYAML_FLAG_DEFAULT,
    Project, snap_grid_midi, snap_grid_fields_schema),
  CYAML_FIELD_MAPPING (
    "quantize_midi", CYAML_FLAG_DEFAULT,
    Project, quantize_midi, quantize_fields_schema),
  CYAML_FIELD_MAPPING (
    "mixer_selections", CYAML_FLAG_DEFAULT,
    Project, mixer_selections,
    mixer_selections_fields_schema),
  CYAML_FIELD_MAPPING (
    "timeline_selections", CYAML_FLAG_DEFAULT,
    Project, timeline_selections,
    timeline_selections_fields_schema),
  CYAML_FIELD_MAPPING (
    "midi_arranger_selections", CYAML_FLAG_DEFAULT,
    Project, midi_arranger_selections,
    midi_arranger_selections_fields_schema),
  CYAML_FIELD_MAPPING (
    "tracklist_selections", CYAML_FLAG_DEFAULT,
    Project, tracklist_selections,
    tracklist_selections_fields_schema),
  CYAML_FIELD_MAPPING (
    "range_1", CYAML_FLAG_DEFAULT,
    Project, range_1, position_fields_schema),
  CYAML_FIELD_MAPPING (
    "range_2", CYAML_FLAG_DEFAULT,
    Project, range_1, position_fields_schema),
	CYAML_FIELD_INT (
    "has_range", CYAML_FLAG_DEFAULT,
    Project, has_range),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aggregated_plugins", CYAML_FLAG_DEFAULT,
    Project, aggregated_plugins, num_aggregated_plugins,
    &plugin_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aggregated_regions", CYAML_FLAG_DEFAULT,
    Project, aggregated_regions, num_aggregated_regions,
    &region_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aggregated_automation_points", CYAML_FLAG_DEFAULT,
    Project, aggregated_automation_points, num_aggregated_automation_points,
    &automation_point_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aggregated_automation_curves", CYAML_FLAG_DEFAULT,
    Project, aggregated_automation_curves, num_aggregated_automation_curves,
    &automation_curve_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aggregated_midi_notes", CYAML_FLAG_DEFAULT,
    Project, aggregated_midi_notes, num_aggregated_midi_notes,
    &midi_note_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aggregated_tracks", CYAML_FLAG_DEFAULT,
    Project, aggregated_tracks, num_aggregated_tracks,
    &track_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aggregated_ports", CYAML_FLAG_DEFAULT,
    Project, aggregated_ports, num_aggregated_ports,
    &port_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aggregated_chords", CYAML_FLAG_DEFAULT,
    Project, aggregated_chords, num_aggregated_chords,
    &chord_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aggregated_automatables", CYAML_FLAG_DEFAULT,
    Project, aggregated_automatables, num_aggregated_automatables,
    &automatable_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aggregated_automation_tracks", CYAML_FLAG_DEFAULT,
    Project, aggregated_automation_tracks,
    num_aggregated_automation_tracks,
    &automation_track_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "aggregated_automation_lanes", CYAML_FLAG_DEFAULT,
    Project, aggregated_automation_lanes,
    num_aggregated_automation_lanes,
    &automation_lane_schema, 0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "chord_track_id", CYAML_FLAG_DEFAULT,
    Project, chord_track_id),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
  project_schema =
{
	CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
  Project, project_fields_schema),
};

/**
 * Checks that everything is okay with the project.
 */
void
project_sanity_check (Project * self);

/**
 * If project has a filename set, it loads that. Otherwise
 * it loads the default project.
 */
int
project_load (char * filename);

/**
 * Saves project to a file.
 */
int
project_save (const char * dir);

/**
 * Sets if the project has range and updates UI.
 */
void
project_set_has_range (int has_range);

/**
 * Adds a region to the project struct and assigns it an
 * ID.
 */
#define PROJECT_ADD_X(camelcase, lowercase) \
  void \
  project_add_##lowercase (camelcase * lowercase);
#define PROJECT_GET_X(camelcase, lowercase) \
  camelcase * \
  project_get_##lowercase (int id);
#define PROJECT_REMOVE_X(camelcase, lowercase) \
  void \
  project_remove_##lowercase (camelcase * x);
/** Moves the object to the given index. */
#define PROJECT_MOVE_X_TO(camelcase, lowercase) \
  void \
  project_move_##lowercase (camelcase * x, int id);
#define P_DEFINE_FUNCS_X(camelcase, lowercase) \
  PROJECT_ADD_X (camelcase, lowercase); \
  PROJECT_GET_X (camelcase, lowercase); \
  PROJECT_REMOVE_X (camelcase, lowercase); \
  PROJECT_MOVE_X_TO (camelcase, lowercase);

P_DEFINE_FUNCS_X (Region, region);
P_DEFINE_FUNCS_X (Track, track);
P_DEFINE_FUNCS_X (Plugin, plugin);
P_DEFINE_FUNCS_X (AutomationPoint, automation_point);
P_DEFINE_FUNCS_X (AutomationCurve, automation_curve);
P_DEFINE_FUNCS_X (MidiNote, midi_note);
P_DEFINE_FUNCS_X (Port, port);
P_DEFINE_FUNCS_X (ZChord, chord);
P_DEFINE_FUNCS_X (Automatable, automatable)
P_DEFINE_FUNCS_X (AutomationTrack, automation_track)
P_DEFINE_FUNCS_X (AutomationLane, automation_lane)

#undef PROJECT_ADD_X
#undef PROJECT_GET_X
#undef PROJECT_REMOVE_X
#undef PROJECT_MOVE_X_TO
#undef P_DEFINE_FUNCS_X

SERIALIZE_INC (Project, project)
DESERIALIZE_INC (Project, project)
PRINT_YAML_INC (Project, project)

#endif
