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
#include "audio/port_connections_manager.h"
#include "audio/quantize_options.h"
#include "audio/region.h"
#include "audio/region_link_group_manager.h"
#include "audio/tracklist.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/audio_selections.h"
#include "gui/backend/automation_selections.h"
#include "gui/backend/chord_selections.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/backend/mixer_selections.h"
#include "gui/backend/timeline.h"
#include "gui/backend/timeline_selections.h"
#include "gui/backend/tool.h"
#include "plugins/plugin.h"
#include "zrythm.h"

#include "ext/zix/zix/sem.h"

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

#define PROJECT_SCHEMA_VERSION 1

#define PROJECT                 ZRYTHM->project
#define DEFAULT_PROJECT_NAME    "Untitled Project"
#define PROJECT_FILE            "project.zpj"
#define PROJECT_BACKUPS_DIR     "backups"
#define PROJECT_PLUGINS_DIR     "plugins"
#define PROJECT_PLUGIN_STATES_DIR "states"
#define PROJECT_PLUGIN_EXT_COPIES_DIR "ext_file_copies"
#define PROJECT_PLUGIN_EXT_LINKS_DIR "ext_file_links"
#define PROJECT_EXPORTS_DIR     "exports"
#define PROJECT_STEMS_DIR       "stems"
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

  /* PROJECT_PATH_EXPORTS / "stems". */
  PROJECT_PATH_EXPORTS_STEMS,

  PROJECT_PATH_POOL,
} ProjectPath;

/**
 * Selection type, used for controlling which part
 * of the interface is selected, for copy-paste,
 * displaying info in the inspector, etc.
 */
typedef enum SelectionType
{
  /** Track selection in tracklist or mixer. */
  SELECTION_TYPE_TRACKLIST,

  /** Timeline or pinned timeline. */
  SELECTION_TYPE_TIMELINE,

  /** Insert selections in the mixer. */
  SELECTION_TYPE_INSERT,

  /** MIDI FX selections in the mixer. */
  SELECTION_TYPE_MIDI_FX,

  /** Instrument slot. */
  SELECTION_TYPE_INSTRUMENT,

  /** Modulator slot. */
  SELECTION_TYPE_MODULATOR,

  /** Editor arranger. */
  SELECTION_TYPE_EDITOR,
} SelectionType;

static const cyaml_strval_t
selection_type_strings[] =
{
  { "Tracklist", SELECTION_TYPE_TRACKLIST },
  { "Timeline", SELECTION_TYPE_TIMELINE },
  { "Insert", SELECTION_TYPE_INSERT },
  { "MIDI FX", SELECTION_TYPE_MIDI_FX },
  { "Instrument", SELECTION_TYPE_INSTRUMENT },
  { "Modulator", SELECTION_TYPE_MODULATOR },
  { "Editor", SELECTION_TYPE_EDITOR },
};

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
  int               schema_version;

  /** Project title. */
  char *            title;

  /** Datetime string to add to the project file. */
  char *            datetime_str;

  /** Path to save the project in. */
  char *            dir;

  /**
   * Backup dir to save the project during
   * the current save call.
   *
   * For example, \ref Project.dir
   * /backups/myproject.bak3.
   */
  char *            backup_dir;

  UndoManager *     undo_manager;

  Tracklist *       tracklist;

  /** Backend for the widget. */
  ClipEditor *      clip_editor;

  /** Timeline widget backend. */
  Timeline *        timeline;

  /** Snap/Grid info for the timeline. */
  SnapGrid *        snap_grid_timeline;

  /** Snap/Grid info for the editor. */
  SnapGrid *        snap_grid_editor;

  /** Quantize info for the timeline. */
  QuantizeOptions * quantize_opts_timeline;

  /** Quantize info for the piano roll. */
  QuantizeOptions * quantize_opts_editor;

  /**
   * Selected objects in the
   * AutomationArrangerWidget.
   */
  AutomationSelections * automation_selections;

  /**
   * Selected objects in the audio editor.
   */
  AudioSelections * audio_selections;

  /**
   * Selected objects in the
   * ChordObjectArrangerWidget.
   */
  ChordSelections * chord_selections;

  /**
   * Selected objects in the TimelineArrangerWidget.
   */
  TimelineSelections * timeline_selections;

  /**
   * Selected MidiNote's in the MidiArrangerWidget.
   */
  MidiArrangerSelections * midi_arranger_selections;

  /**
   * Selected Track's.
   */
  TracklistSelections * tracklist_selections;

  /**
   * Plugin selections in the Mixer.
   */
  MixerSelections * mixer_selections;

  /** Zoom levels. TODO & move to clip_editor */
  double            timeline_zoom;
  double            piano_roll_zoom;

  /** Manager for region link groups. */
  RegionLinkGroupManager * region_link_group_manager;

  PortConnectionsManager * port_connections_manager;

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
  SelectionType     last_selection;

  /** Zrythm version, for serialization */
  char *            version;

  /** Semaphore used to block saving. */
  ZixSem            save_sem;

  gint64            last_autosave_time;
} Project;

static const cyaml_schema_field_t
  project_fields_schema[] =
{
  YAML_FIELD_INT (
    Project, schema_version),
  YAML_FIELD_STRING_PTR (
    Project, title),
  YAML_FIELD_STRING_PTR (
    Project, datetime_str),
  YAML_FIELD_STRING_PTR (
    Project, version),
  YAML_FIELD_MAPPING_PTR (
    Project, tracklist, tracklist_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, clip_editor,
    clip_editor_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, timeline,
    timeline_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, snap_grid_timeline,
    snap_grid_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, snap_grid_editor,
    snap_grid_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, quantize_opts_timeline,
    quantize_options_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, quantize_opts_editor,
    quantize_options_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, audio_engine, engine_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, mixer_selections,
    mixer_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, timeline_selections,
    timeline_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, midi_arranger_selections,
    midi_arranger_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, chord_selections,
    chord_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, automation_selections,
    automation_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, audio_selections,
    audio_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, tracklist_selections,
    tracklist_selections_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, region_link_group_manager,
    region_link_group_manager_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, port_connections_manager,
    port_connections_manager_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    Project, midi_mappings,
    midi_mappings_fields_schema),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Project, undo_manager,
    undo_manager_fields_schema),
  YAML_FIELD_ENUM (
    Project, last_selection,
    selection_type_strings),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  project_schema =
{
  YAML_VALUE_PTR (
    Project, project_fields_schema),
};

/**
 * Project save data.
 */
typedef struct ProjectSaveData
{
  /** Project clone (with memcpy). */
  Project * project;

  /** Full path to save to. */
  char *    project_file_path;

  bool      is_backup;

  /** To be set to true when the thread finishes. */
  bool      finished;

  bool      show_notification;

  /** Whether an error occurred during saving. */
  bool      has_error;

  GenericProgressInfo progress_info;
} ProjectSaveData;

/**
 * Checks that everything is okay with the project.
 */
void
project_validate (Project * self);

ArrangerSelections *
project_get_arranger_selections_for_last_selection (
  Project * self);

/**
 * Creates a default project.
 *
 * This is only used internally or for generating
 * projects from scripts.
 *
 * @param prj_dir The directory of the project to
 *   create, including its title.
 * @param headless Create the project assuming we
 *   are running without a UI.
 * @param start_engine Whether to also start the
 *   engine after creating the project.
 */
COLD
Project *
project_create_default (
  Project *    self,
  const char * prj_dir,
  bool         headless,
  bool         with_engine);

/**
 * If project has a filename set, it loads that.
 * Otherwise it loads the default project.
 *
 * @param is_template Load the project as a
 *   template and create a new project from it.
 *
 * @return 0 if successful, non-zero otherwise.
 */
COLD
int
project_load (
  const char * filename,
  const bool   is_template);

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
MALLOC
NONNULL
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
COLD
void
project_init_selections (Project * self);

/**
 * Compresses/decompress a project from a file/data
 * to a file/data.
 *
 * @param compress True to compress, false to
 *   decompress.
 * @param[out] _dest Pointer to a location to allocate
 *   memory.
 * @param[out] _dest_size Pointer to a location to
 *   store the size of the allocated memory.
 * @param _src Input buffer or filepath.
 * @param _src_size Input buffer size, if not
 *   filepath.
 *
 * @return Whether successful.
 */
bool
_project_compress (
  bool                   compress,
  char **                _dest,
  size_t *               _dest_size,
  ProjectCompressionFlag dest_type,
  const char *           _src,
  const size_t           _src_size,
  ProjectCompressionFlag src_type,
  GError **              error);

#define project_compress(a,b,c,d,e,f,error) \
  _project_compress (true, a, b, c, d, e, f, error)

#define project_decompress(a,b,c,d,e,f,error) \
  _project_compress (false, a, b, c, d, e, f, error)

/**
 * Returns the YAML representation of the saved
 * project file.
 *
 * To be free'd with free().
 *
 * @param backup Whether to use the project file
 *   from the most recent backup.
 */
char *
project_get_existing_yaml (
  Project * self,
  bool      backup);

/**
 * Deep-clones the given project.
 *
 * To be used during save on the main thread.
 *
 * @param for_backup Whether the resulting project
 *   is for a backup.
 */
NONNULL
Project *
project_clone (
  const Project * src,
  bool            for_backup);

/**
 * Creates an empty project object.
 */
COLD
Project *
project_new (
  Zrythm * zrythm);

/**
 * Tears down the project.
 */
void
project_free (Project * self);

/**
 * @}
 */

#endif
