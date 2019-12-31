/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * A project (or song), containing all the project
 * data as opposed to zrythm_app.h which manages
 * global things like plugin descriptors and global
 * settings.
 */

#ifndef __PROJECT_H__
#define __PROJECT_H__

#include "actions/undo_manager.h"
#include "audio/engine.h"
#include "audio/midi_mapping.h"
#include "audio/midi_note.h"
#include "audio/port.h"
#include "audio/quantize_options.h"
#include "audio/region.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/automation_selections.h"
#include "gui/backend/chord_selections.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/backend/mixer_selections.h"
#include "gui/backend/timeline_selections.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/backend/tool.h"
#include "plugins/plugin.h"
#include "zrythm.h"

#include <gtk/gtk.h>

typedef struct Timeline Timeline;
typedef struct Transport Transport;
typedef struct Port Port;
typedef struct Channel Channel;
typedef struct Plugin Plugin;
typedef struct Track Track;
typedef struct ZRegion ZRegion;
typedef struct MidiNote MidiNote;
typedef struct Track ChordTrack;

/**
 * @addtogroup project Project
 *
 * @{
 */

#define PROJECT                 ZRYTHM->project
#define DEFAULT_PROJECT_NAME    "Untitled Project"
#define PROJECT_FILE            "project.zpj"
#define PROJECT_BACKUPS_DIR     "backups"
#define PROJECT_STATES_DIR      "states"
#define PROJECT_EXPORTS_DIR     "exports"
#define PROJECT_POOL_DIR        "pool"

/**
 * Selection type, used for displaying info in the
 * inspector.
 */
typedef enum SelectionType
{
  SELECTION_TYPE_TRACK,
  SELECTION_TYPE_PLUGIN,
  SELECTION_TYPE_EDITOR,
} SelectionType;


/**
 * Contains all of the info that will be serialized
 * into a project file.
 */
typedef struct Project
{
  /** Project title. */
  char *             title;

  /** Datetime string to add to the project file. */
  char *             datetime_str;

  /** Path to save the project in. */
  char *             dir;

  /**
   * Backup dir to save the project during
   * the current save call.
   *
   * For example, \ref Project.dir
   * /backups/myproject.bak3.
   */
  char *             backup_dir;

  UndoManager        undo_manager;

  Tracklist          tracklist;

  /** Backend for the widget. */
  ClipEditor         clip_editor;

  /** Snap/Grid info for the timeline. */
  SnapGrid           snap_grid_timeline;

  /** Quantize info for the timeline. */
  QuantizeOptions    quantize_opts_timeline;

  /** Snap/Grid info for the piano roll. */
  SnapGrid          snap_grid_midi;

  /** Quantize info for the piano roll. */
  QuantizeOptions    quantize_opts_editor;

  /**
   * Selected objects in the
   * AutomationArrangerWidget.
   */
  AutomationSelections automation_selections;

  /**
   * Selected objects in the
   * ChordObjectArrangerWidget.
   */
  ChordSelections      chord_selections;

  /**
   * Selected objects in the TimelineArrangerWidget.
   */
  TimelineSelections timeline_selections;

  /**
   * Selected MidiNote's in the MidiArrangerWidget.
   */
  MidiArrangerSelections midi_arranger_selections;

  /**
   * Selected Track's.
   */
  TracklistSelections tracklist_selections;

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

  /** MIDI bindings. */
  MidiMappings *    midi_mappings;

  /**
   * Currently selected tool (select - normal,
   * select - stretch, edit, delete, ramp, audition)
   */
  Tool               tool;

  /**
   * If a project is currently loaded or not.
   *
   * This is useful so that we know if we need to
   * tear down when loading a new project while
   * another one is loaded.
   */
  int               loaded;

  /**
   * The last thing selected in the GUI.
   *
   * Used in inspector_widget_refresh.
   */
  SelectionType   last_selection;

  /** Zrythm version, for zerialization */
  char *            version;

  gint64            last_autosave_time;
} Project;

static const cyaml_schema_field_t
  project_fields_schema[] =
{
  CYAML_FIELD_STRING_PTR (
    "title", CYAML_FLAG_POINTER,
    Project, title,
    0, CYAML_UNLIMITED),
  CYAML_FIELD_STRING_PTR (
    "datetime_str", CYAML_FLAG_POINTER,
    Project, datetime_str,
    0, CYAML_UNLIMITED),
  CYAML_FIELD_STRING_PTR (
    "version", CYAML_FLAG_POINTER,
    Project, version,
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
    "quantize_opts_timeline", CYAML_FLAG_DEFAULT,
    Project, quantize_opts_timeline,
    quantize_options_fields_schema),
  CYAML_FIELD_MAPPING (
    "audio_engine", CYAML_FLAG_DEFAULT,
    Project, audio_engine, engine_fields_schema),
  CYAML_FIELD_MAPPING (
    "snap_grid_midi", CYAML_FLAG_DEFAULT,
    Project, snap_grid_midi, snap_grid_fields_schema),
  CYAML_FIELD_MAPPING (
    "quantize_opts_editor", CYAML_FLAG_DEFAULT,
    Project, quantize_opts_editor,
    quantize_options_fields_schema),
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
    "chord_selections", CYAML_FLAG_DEFAULT,
    Project, chord_selections,
    chord_selections_fields_schema),
  CYAML_FIELD_MAPPING (
    "automation_selections", CYAML_FLAG_DEFAULT,
    Project, automation_selections,
    automation_selections_fields_schema),
  CYAML_FIELD_MAPPING (
    "tracklist_selections", CYAML_FLAG_DEFAULT,
    Project, tracklist_selections,
    tracklist_selections_fields_schema),
  CYAML_FIELD_MAPPING (
    "range_1", CYAML_FLAG_DEFAULT,
    Project, range_1, position_fields_schema),
  CYAML_FIELD_MAPPING (
    "range_2", CYAML_FLAG_DEFAULT,
    Project, range_2, position_fields_schema),
	CYAML_FIELD_INT (
    "has_range", CYAML_FLAG_DEFAULT,
    Project, has_range),
  CYAML_FIELD_MAPPING_PTR (
    "midi_mappings", CYAML_FLAG_POINTER,
    Project, midi_mappings,
    midi_mappings_fields_schema),

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
 * If project has a filename set, it loads that.
 * Otherwise it loads the default project.
 *
 * @param is_template Load the project as a
 *   template and create a new project from it.
 *
 * @return 0 if successful, non-zero otherwise.
 */
int
project_load (
  char *    filename,
  const int is_template);

/**
 * Saves the project to a project file in the
 * given dir.
 *
 * @param is_backup 1 if this is a backup. Backups
 *   will be saved as <original filename>.bak<num>.
 * @param show_notification Show a notification
 *   in the UI that the project was saved.
 */
int
project_save (
  Project *    self,
  const char * _dir,
  const int    is_backup,
  const int    show_notification);

/**
 * Autosave callback.
 *
 * This will keep getting called at regular short
 * intervals, and if enough time has passed and
 * it's okay to save it will autosave, otherwise it
 * will wait until the next interval and check
 * again.
 */
int
project_autosave_cb (
  void * data);

/**
 * Returns the backups dir for the given Project.
 */
char *
project_get_backups_dir (
  Project * self);

/**
 * Returns the exports dir for the given Project.
 */
char *
project_get_exports_dir (
  Project * self);

/**
 * Returns the states dir for the given Project.
 *
 * @param is_backup 1 to get the states dir of the
 *   current backup instead of the main project.
 */
char *
project_get_states_dir (
  Project * self,
  const int is_backup);

/**
 * Returns the pool dir for the given Project.
 */
char *
project_get_pool_dir (
  Project * self);

/**
 * Returns the full project file (project.yml)
 * path.
 *
 * @param is_backup 1 to get the project file of the
 *   current backup instead of the main project.
 */
char *
project_get_project_file_path (
  Project * self,
  const int is_backup);

/**
 * Initializes the selections in the project.
 *
 * @note
 * Not meant to be used anywhere besides
 * tests and project.c
 */
void
project_init_selections (Project * self);

/**
 * Sets if the project has range and updates UI.
 */
void
project_set_has_range (int has_range);

SERIALIZE_INC (Project, project)
DESERIALIZE_INC (Project, project)
PRINT_YAML_INC (Project, project)

/**
 * @}
 */

#endif
