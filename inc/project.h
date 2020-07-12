/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/region_link_group_manager.h"
#include "audio/tracklist.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/automation_selections.h"
#include "gui/backend/chord_selections.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/backend/mixer_selections.h"
#include "gui/backend/timeline_selections.h"
#include "gui/backend/tool.h"
#include "plugins/plugin.h"
#include "zrythm.h"

#include <gtk/gtk.h>

typedef struct Timeline Timeline;
typedef struct Transport Transport;
typedef struct Tracklist Tracklist;
typedef struct TracklistSelections
  TracklistSelections;

/**
 * @addtogroup project Project
 *
 * @{
 */

#define PROJECT                 ZRYTHM->project
#define DEFAULT_PROJECT_NAME    "Untitled Project"
#define PROJECT_FILE            "project.zpj"
#define PROJECT_BACKUPS_DIR     "backups"
#define PROJECT_PLUGINS_DIR     "plugins"
#define PROJECT_PLUGIN_STATES_DIR "states"
#define PROJECT_PLUGIN_EXT_COPIES_DIR "ext_file_copies"
#define PROJECT_PLUGIN_EXT_LINKS_DIR "ext_file_links"
#define PROJECT_EXPORTS_DIR     "exports"
#define PROJECT_POOL_DIR        "pool"

typedef enum ProjectPath
{
  PROJECT_PATH_PROJECT_FILE,
  PROJECT_PATH_BACKUPS,

  /** Plugins path. */
  PROJECT_PATH_PLUGINS,

  /** Path for state .ttl files. */
  PROJECT_PATH_PLUGIN_STATES,

  /** External files for plugin states, under the
   * STATES dir. */
  PROJECT_PATH_PLUGIN_EXT_COPIES,

  /** External files for plugin states, under the
   * STATES dir. */
  PROJECT_PATH_PLUGIN_EXT_LINKS,

  PROJECT_PATH_EXPORTS,

  PROJECT_PATH_POOL,
} ProjectPath;

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
 * Flag to pass to project_compress() and
 * project_decompress().
 */
typedef enum ProjectCompressionFlag
{
  PROJECT_COMPRESS_FILE,
  PROJECT_COMPRESS_DATA,
} ProjectCompressionFlag;

#define PROJECT_DECOMPRESS_FILE \
  PROJECT_COMPRESS_FILE
#define PROJECT_DECOMPRESS_DATA \
  PROJECT_COMPRESS_DATA

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

  UndoManager *      undo_manager;

  Tracklist *        tracklist;

  /** Backend for the widget. */
  ClipEditor *       clip_editor;

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
  TracklistSelections * tracklist_selections;

  /**
   * Plugin selections in the Mixer.
   */
  MixerSelections    mixer_selections;

  /** Zoom levels. TODO & move to clip_editor */
  double             timeline_zoom;
  double             piano_roll_zoom;

  /** Manager for region link groups. */
  RegionLinkGroupManager region_link_group_manager;

  /**
   * The audio backend
   */
  AudioEngine *     audio_engine;

  /** MIDI bindings. */
  MidiMappings *    midi_mappings;

  /**
   * Currently selected tool (select - normal,
   * select - stretch, edit, delete, ramp, audition)
   */
  Tool              tool;

  /**
   * Whether the current is currently being loaded
   * from a backup file.
   *
   * This is useful when instantiating plugins from
   * state and should be set to false after the
   * project is loaded.
   */
  bool              loading_from_backup;

  /**
   * If a project is currently loaded or not.
   *
   * This is useful so that we know if we need to
   * tear down when loading a new project while
   * another one is loaded.
   */
  bool              loaded;

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
  YAML_FIELD_STRING_PTR (
    Project, title),
  YAML_FIELD_STRING_PTR (
    Project, datetime_str),
  YAML_FIELD_STRING_PTR (
    Project, version),
  YAML_FIELD_MAPPING_PTR (
    Project, tracklist, tracklist_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, clip_editor, clip_editor_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project, snap_grid_timeline,
    snap_grid_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project, quantize_opts_timeline,
    quantize_options_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, audio_engine, engine_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project, snap_grid_midi,
    snap_grid_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project, quantize_opts_editor,
    quantize_options_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project, mixer_selections,
    mixer_selections_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project, timeline_selections,
    timeline_selections_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project, midi_arranger_selections,
    midi_arranger_selections_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project, chord_selections,
    chord_selections_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project, automation_selections,
    automation_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, tracklist_selections,
    tracklist_selections_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    Project, region_link_group_manager,
    region_link_group_manager_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, midi_mappings,
    midi_mappings_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, undo_manager,
    undo_manager_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  project_schema =
{
  YAML_VALUE_PTR (
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
 * @param async Save asynchronously in another
 *   thread.
 *
 * @return Non-zero if error.
 */
int
project_save (
  Project *    self,
  const char * _dir,
  const bool   is_backup,
  const bool   show_notification,
  const bool   async);

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
 * Returns the requested project path as a newly
 * allocated string.
 *
 * @param backup Whether to get the path for the
 *   current backup instead of the main project.
 */
char *
project_get_path (
  Project *     self,
  ProjectPath   path,
  bool          backup);

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
 * Compresses/decompress a project from a file/data
 * to a file/data.
 *
 * @param compress True to compress, false to
 *   decompress.
 * @param[out] dest Pointer to a location to allocate
 *   memory.
 * @param[out] dest_size Pointer to a location to
 *   store the size of the allocated memory.
 * @param src Input buffer or filepath.
 * @param src_size Input buffer size, if not
 *   filepath.
 *
 * @return Error message if error, otherwise NULL.
 */
char *
_project_compress (
  bool                   compress,
  char **                dest,
  size_t *               dest_size,
  ProjectCompressionFlag dest_type,
  const char *           src,
  const size_t           src_size,
  ProjectCompressionFlag src_type);

#define project_compress(a,b,c,d,e,f) \
  _project_compress (true, a, b, c, d, e, f)

#define project_decompress(a,b,c,d,e,f) \
  _project_compress (false, a, b, c, d, e, f)

/**
 * Creates an empty project object.
 */
Project *
project_new (
  Zrythm * zrythm);

/**
 * Tears down the project.
 */
void
project_free (Project * self);

SERIALIZE_INC (Project, project)
DESERIALIZE_INC (Project, project)
PRINT_YAML_INC (Project, project)

/**
 * @}
 */

#endif
