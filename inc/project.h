/*
 * project.h - A project (or song), containing all the project data
 *   as opposed to zrythm_app.h which manages things not project-dependent
 *   like plugins and overall settings
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#ifndef __PROJECT_H__
#define __PROJECT_H__

#include "actions/undo_manager.h"
#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/piano_roll.h"
#include "audio/quantize.h"
#include "audio/tracklist.h"
#include "gui/backend/timeline_selections.h"
#include "zrythm.h"

#include <gtk/gtk.h>

#define PROJECT                 ZRYTHM->project
#define DEFAULT_PROJECT_NAME    "Untitled Project"
#define PROJECT_FILE            "project.xml"
#define PROJECT_REGIONS_FILE    "regions.xml"
#define PROJECT_PORTS_FILE      "ports.xml"
#define PROJECT_REGIONS_DIR     "Regions"
#define PROJECT_STATES_DIR      "states"

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
typedef struct ChordTrack ChordTrack;
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
  char              * ports_file_path;
  char              * regions_dir;
  char              * states_dir;

  UndoManager         undo_manager;

  Tracklist          tracklist;

  PianoRoll          piano_roll;

  SnapGrid           snap_grid_timeline; ///< snap/grid info for timeline
  Quantize           quantize_timeline;
  SnapGrid          snap_grid_midi; ///< snap/grid info for midi editor
  Quantize           quantize_midi;

  TimelineSelections       timeline_selections;

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
  AutomationPoint * automation_points[30000];
  int               num_automation_points;
  AutomationCurve * automation_curves[30000];
  int               num_automation_curves;
  MidiNote *        midi_notes[30000];
  int               num_midi_notes;
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
  ChordTrack *      chord_track;

  /**
   * If a project is currently loaded or not.
   *
   * This is useful so that we know if we need to tear down
   * when loading a new project while another one is loaded.
   */
  int               loaded;
} Project;

/**
 * If project has a filename set, it loads that. Otherwise
 * it loads the default project.
 */
void
project_load (char * filename);

/**
 * Saves project to a file.
 */
void
project_save (const char * dir);

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

#undef PROJECT_ADD_X
#undef PROJECT_GET_X

#endif
