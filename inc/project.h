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
#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/midi_note.h"
#include "audio/port.h"
#include "audio/quantize.h"
#include "audio/region.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/backend/timeline_selections.h"
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

/* project xml version */
#define PROJECT_XML_VER "0.1"

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

  /**
   * Absolute path to project xml.
   */
  char              * filename;

  char              * dir; ///< path to save the project
  char              * project_file_path; ///< for convenience
  char              * regions_file_path;
  char              * regions_dir;
  char              * states_dir;
  char              * exports_dir;

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
   * For serializing/deserializing.
   *
   * These are stored here when created so that they can
   * easily be serialized/deserialized.
   *
   * Note: when objects get deleted they are responsible
   * for setting their position to NULL.
   */
  Plugin *          plugins[2000];
  int               num_plugins;
  Region            * regions [30000];
  int               num_regions;
  Automatable *     automatables[20000];
  int               num_automatables;
  AutomationPoint * automation_points[30000];
  int               num_automation_points;
  AutomationCurve * automation_curves[30000];
  int               num_automation_curves;
  AutomationTrack * automation_tracks[50000];
  int               num_automation_tracks;
  AutomationLane *  automation_lanes[50000];
  int               num_automation_lanes;
  MidiNote *        midi_notes[30000];
  int               num_midi_notes;
  Chord *           chords[600];
  int               num_chords;
  Channel *         channels[3000];
  int               num_channels;
  Track *           tracks[3000];
  int               num_tracks;
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
    "timeline_selections", CYAML_FLAG_DEFAULT,
    Project, timeline_selections, timeline_selections_fields_schema),
  CYAML_FIELD_MAPPING (
    "midi_arranger_selections", CYAML_FLAG_DEFAULT,
    Project, midi_arranger_selections, midi_arranger_selections_fields_schema),
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
    "plugins", CYAML_FLAG_DEFAULT,
    Project, plugins, num_plugins,
    &plugin_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "regions", CYAML_FLAG_DEFAULT,
    Project, regions, num_regions,
    &region_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "automation_points", CYAML_FLAG_DEFAULT,
    Project, automation_points, num_automation_points,
    &automation_point_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "automation_curves", CYAML_FLAG_DEFAULT,
    Project, automation_curves, num_automation_curves,
    &automation_curve_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "midi_notes", CYAML_FLAG_DEFAULT,
    Project, midi_notes, num_midi_notes,
    &midi_note_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "channels", CYAML_FLAG_DEFAULT,
    Project, channels, num_channels,
    &channel_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "tracks", CYAML_FLAG_DEFAULT,
    Project, tracks, num_tracks,
    &track_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "ports", CYAML_FLAG_DEFAULT,
    Project, ports, num_ports,
    &port_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "chords", CYAML_FLAG_DEFAULT,
    Project, chords, num_chords,
    &chord_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "automatables", CYAML_FLAG_DEFAULT,
    Project, automatables, num_automatables,
    &automatable_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "automation_tracks", CYAML_FLAG_DEFAULT,
    Project, automation_tracks,
    num_automation_tracks,
    &automation_track_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "automation_lanes", CYAML_FLAG_DEFAULT,
    Project, automation_lanes,
    num_automation_lanes,
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

PROJECT_ADD_X (Region, region);
PROJECT_GET_X (Region, region);
PROJECT_ADD_X (Track, track);
PROJECT_GET_X (Track, track);
PROJECT_ADD_X (Channel, channel);
PROJECT_GET_X (Channel, channel);
PROJECT_ADD_X (Plugin, plugin);
PROJECT_GET_X (Plugin, plugin);
PROJECT_ADD_X (AutomationPoint, automation_point);
PROJECT_GET_X (AutomationPoint, automation_point);
PROJECT_ADD_X (AutomationCurve, automation_curve);
PROJECT_GET_X (AutomationCurve, automation_curve);
PROJECT_ADD_X (MidiNote, midi_note);
PROJECT_GET_X (MidiNote, midi_note);
PROJECT_ADD_X (Port, port);
PROJECT_GET_X (Port, port);
PROJECT_ADD_X (Chord, chord);
PROJECT_GET_X (Chord, chord);
PROJECT_ADD_X (Automatable, automatable)
PROJECT_GET_X (Automatable, automatable)
PROJECT_ADD_X (AutomationTrack, automation_track)
PROJECT_GET_X (AutomationTrack, automation_track)
PROJECT_ADD_X (AutomationLane, automation_lane)
PROJECT_GET_X (AutomationLane, automation_lane)

#undef PROJECT_ADD_X
#undef PROJECT_GET_X

SERIALIZE_INC (Project, project)
DESERIALIZE_INC (Project, project)
PRINT_YAML_INC (Project, project)

#endif
