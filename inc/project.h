// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
#include "dsp/engine.h"
#include "dsp/midi_mapping.h"
#include "dsp/midi_note.h"
#include "dsp/port.h"
#include "dsp/port_connections_manager.h"
#include "dsp/quantize_options.h"
#include "dsp/region.h"
#include "dsp/region_link_group_manager.h"
#include "dsp/tracklist.h"
#include "gui/backend/audio_selections.h"
#include "gui/backend/automation_selections.h"
#include "gui/backend/chord_selections.h"
#include "gui/backend/clip_editor.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/backend/mixer_selections.h"
#include "gui/backend/timeline.h"
#include "gui/backend/timeline_selections.h"
#include "gui/backend/tool.h"
#include "plugins/plugin.h"
#include "zrythm.h"

#include <gtk/gtk.h>

#include <zix/sem.h>

typedef struct Timeline            Timeline;
typedef struct Transport           Transport;
typedef struct Tracklist           Tracklist;
typedef struct TracklistSelections TracklistSelections;

/**
 * @addtogroup project Project
 *
 * @{
 */

#define PROJECT_FORMAT_MAJOR 1
#define PROJECT_FORMAT_MINOR 7

#define PROJECT ZRYTHM->project
#define DEFAULT_PROJECT_NAME "Untitled Project"
#define PROJECT_FILE "project.zpj"
#define PROJECT_BACKUPS_DIR "backups"
#define PROJECT_PLUGINS_DIR "plugins"
#define PROJECT_PLUGIN_STATES_DIR "states"
#define PROJECT_PLUGIN_EXT_COPIES_DIR "ext_file_copies"
#define PROJECT_PLUGIN_EXT_LINKS_DIR "ext_file_links"
#define PROJECT_EXPORTS_DIR "exports"
#define PROJECT_STEMS_DIR "stems"
#define PROJECT_POOL_DIR "pool"
#define PROJECT_FINISHED_FILE "FINISHED"

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

  PROJECT_PATH_FINISHED_FILE,
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

static const cyaml_strval_t selection_type_strings[] = {
  {"Tracklist",   SELECTION_TYPE_TRACKLIST },
  { "Timeline",   SELECTION_TYPE_TIMELINE  },
  { "Insert",     SELECTION_TYPE_INSERT    },
  { "MIDI FX",    SELECTION_TYPE_MIDI_FX   },
  { "Instrument", SELECTION_TYPE_INSTRUMENT},
  { "Modulator",  SELECTION_TYPE_MODULATOR },
  { "Editor",     SELECTION_TYPE_EDITOR    },
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

#define PROJECT_DECOMPRESS_FILE PROJECT_COMPRESS_FILE
#define PROJECT_DECOMPRESS_DATA PROJECT_COMPRESS_DATA

/**
 * Contains all of the info that will be serialized into a
 * project file.
 *
 * TODO create a UserInterface struct and move things that are
 * only relevant to the UI there.
 */
typedef struct Project
{
  /** Project title. */
  char * title;

  /** Datetime string to add to the project file. */
  char * datetime_str;

  /** Path to save the project in. */
  char * dir;

  /**
   * Backup dir to save the project during
   * the current save call.
   *
   * For example, \ref Project.dir
   * /backups/myproject.bak3.
   */
  char * backup_dir;

  UndoManager * undo_manager;

  Tracklist * tracklist;

  /** Backend for the widget. */
  ClipEditor * clip_editor;

  /** Timeline widget backend. */
  Timeline * timeline;

  /** Snap/Grid info for the timeline. */
  SnapGrid * snap_grid_timeline;

  /** Snap/Grid info for the editor. */
  SnapGrid * snap_grid_editor;

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
  double timeline_zoom;
  double piano_roll_zoom;

  /** Manager for region link groups. */
  RegionLinkGroupManager * region_link_group_manager;

  PortConnectionsManager * port_connections_manager;

  /**
   * The audio backend.
   */
  AudioEngine * audio_engine;

  /** MIDI bindings. */
  MidiMappings * midi_mappings;

  /**
   * Currently selected tool (select - normal,
   * select - stretch, edit, delete, ramp, audition)
   */
  Tool tool;

  /**
   * Whether the current is currently being loaded
   * from a backup file.
   *
   * This is useful when instantiating plugins from
   * state and should be set to false after the
   * project is loaded.
   */
  bool loading_from_backup;

  /**
   * If a project is currently loaded or not.
   *
   * This is useful so that we know if we need to
   * tear down when loading a new project while
   * another one is loaded.
   */
  bool loaded;

  /**
   * The last thing selected in the GUI.
   *
   * Used in inspector_widget_refresh.
   */
  SelectionType last_selection;

  /** Zrythm version, for serialization */
  char * version;

  /** Semaphore used to block saving. */
  ZixSem save_sem;

  /** Used to check if the project has unsaved
   * changes. */
  UndoableAction * last_saved_action;

  /** Last successful autosave timestamp. */
  gint64 last_successful_autosave_time;

  /**
   * Last undoable action in the previous successful autosave.
   *
   * This is used to avoid saving unnecessary backups.
   */
  UndoableAction * last_action_in_last_successful_autosave;

  /** Used when deserializing projects. */
  int format_major;
  int format_minor;
} Project;

/**
 * Project save data.
 */
typedef struct ProjectSaveData
{
  /** Project clone (with memcpy). */
  Project * project;

  /** Full path to save to. */
  char * project_file_path;

  bool is_backup;

  /** To be set to true when the thread finishes. */
  bool finished;

  bool show_notification;

  /** Whether an error occurred during saving. */
  bool has_error;

  ProgressInfo * progress_info;
} ProjectSaveData;

ProjectSaveData *
project_save_data_new (void);

void
project_save_data_free (ProjectSaveData * self);

/**
 * Checks that everything is okay with the project.
 */
void
project_validate (Project * self);

/**
 * @return Whether positions were adjusted.
 */
bool
project_fix_audio_regions (Project * self);

ArrangerSelections *
project_get_arranger_selections_for_last_selection (Project * self);

void
project_init_common (Project * self);

/**
 * Saves the project to a project file in the given dir.
 *
 * @param is_backup 1 if this is a backup. Backups
 *   will be saved as <original filename>.bak<num>.
 * @param show_notification Show a notification
 *   in the UI that the project was saved.
 * @param async Save asynchronously in another
 *   thread.
 *
 * @return Whether successful.
 */
WARN_UNUSED_RESULT bool
project_save (
  Project *    self,
  const char * _dir,
  const bool   is_backup,
  const bool   show_notification,
  const bool   async,
  GError **    error);

/**
 * Autosave callback.
 *
 * This will keep getting called at regular short intervals,
 * and if enough time has passed and it's okay to save it will
 * autosave, otherwise it will wait until the next interval and
 * check again.
 */
int
project_autosave_cb (void * data);

bool
project_make_project_dirs (Project * self, bool is_backup, GError ** error);

/**
 * Returns the requested project path as a newly
 * allocated string.
 *
 * @param backup Whether to get the path for the
 *   current backup instead of the main project.
 */
MALLOC
NONNULL char *
project_get_path (Project * self, ProjectPath path, bool backup);

/**
 * Initializes the selections in the project.
 *
 * @note
 * Not meant to be used anywhere besides tests and project.c
 */
COLD void
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

#define project_compress(a, b, c, d, e, f, error) \
  _project_compress (true, a, b, c, d, e, f, error)

#define project_decompress(a, b, c, d, e, f, error) \
  _project_compress (false, a, b, c, d, e, f, error)

/**
 * Returns the uncompressed text representation of the saved project file.
 *
 * To be free'd with free().
 *
 * @param backup Whether to use the project file from the most recent backup.
 */
char *
project_get_existing_uncompressed_text (
  Project * self,
  bool      backup,
  GError ** error);

/**
 * @memberof Project
 */
NONNULL bool
project_has_unsaved_changes (const Project * self);

/**
 * Deep-clones the given project.
 *
 * To be used during save on the main thread.
 *
 * @param for_backup Whether the resulting project
 *   is for a backup.
 */
NONNULL Project *
project_clone (const Project * src, bool for_backup, GError ** error);

/**
 * Creates an empty project object.
 */
COLD Project *
project_new (Zrythm * zrythm);

/**
 * Tears down the project.
 */
void
project_free (Project * self);

/**
 * @}
 */

#endif
