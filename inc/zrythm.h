/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * The Zrythm GTK application.
 */

#ifndef __ZRYTHM_H__
#define __ZRYTHM_H__

#include "audio/metronome.h"
#include "audio/snap_grid.h"
#include "gui/backend/events.h"
#include "gui/backend/file_manager.h"
#include "plugins/plugin_manager.h"
#include "settings/settings.h"
#include "utils/log.h"

#include <gtk/gtk.h>

#define ZRYTHM_APP_TYPE (zrythm_app_get_type ())
G_DECLARE_FINAL_TYPE (
  ZrythmApp, zrythm_app, ZRYTHM, APP,
  GtkApplication)

typedef struct _MainWindowWidget MainWindowWidget;
typedef struct Project Project;
typedef struct Symap Symap;
typedef struct CairoCaches CairoCaches;
typedef struct UiCaches UiCaches;
typedef struct MPMCQueue MPMCQueue;
typedef struct ObjectPool ObjectPool;
typedef struct RecordingManager RecordingManager;

/**
 * @addtogroup general
 *
 * @{
 */

#define ZRYTHM (zrythm)

#define ZRYTHM_PROJECTS_DIR "Projects"

#define MAX_RECENT_PROJECTS 20
#define DEBUGGING (ZRYTHM->debug)
#define ZRYTHM_TESTING (ZRYTHM->testing)
#define ZRYTHM_HAVE_UI (ZRYTHM->have_ui)

/**
 * To be used throughout the program.
 *
 * Everything here should be global and function regardless
 * of the project.
 */
typedef struct Zrythm
{
  /**
   * Manages plugins (loading, instantiating, etc.)
   */
  PluginManager           plugin_manager;

  MainWindowWidget *      main_window; ///< main window

  /**
   * Application settings
   */
  Settings                settings;

  /**
   * Project data.
   *
   * This is what should be exported/imported when saving/
   * loading projects.
   *
   * The only reason this is a pointer is to easily
   * deserialize.
   */
  Project *               project;

  /** /home/user/zrythm */
  char *                  zrythm_dir;
  /** \ref Zrythm.zrythm_dir /zrythm_dir/Projects */
  char *                  projects_dir;
  /** \ref Zrythm.zrythm_dir /Templates TODO */
  char *                  templates_dir;
  /** \ref Zrythm.zrythm_dir /log */
  char *                  log_dir;

  /** +1 to ensure last element is NULL in case
   * full. */
  char *
    recent_projects[MAX_RECENT_PROJECTS + 1];
  int                     num_recent_projects;

  /** NULL terminated array of project template
   * absolute paths. */
  char **                 templates;

  /** The metronome. */
  Metronome               metronome;

  /** 1 if the open file is a template to be used
   * to create a new project from. */
  int                     opening_template;

  /** 1 if creating a new project, either from
   * a template or blank. */
  int                     creating_project;

  /** Path to create a project in, including its
   * title. */
  char *                  create_project_path;

  /**
   * Filename to open passed through the command
   * line.
   *
   * Used only when a filename is passed.
   * E.g., zrytm myproject.xml
   */
  char *                  open_filename;

  /**
   * Event queue, mainly for GUI events.
   */
  MPMCQueue *             event_queue;

  /**
   * Object pool of event structs to avoid real time
   * allocation.
   */
  ObjectPool *            event_obj_pool;

  /** Recording manager. */
  RecordingManager *      recording_manager;

  /** File manager. */
  FileManager             file_manager;

  /**
   * String interner for internal things.
   */
  Symap *                 symap;

  CairoCaches *           cairo_caches;

  UiCaches *              ui_caches;

  /**
   * In debug mode or not (determined by GSetting).
   */
  int                     debug;

  /**
   * Used when running the tests.
   *
   * This is set by the TESTING environment variable.
   */
  int                     testing;

  /** Initialization thread. */
  GThread *               init_thread;

  /**
   * The GTK thread where the main GUI loop runs.
   *
   * This is stored for identification purposes
   * in other threads.
   */
  GThread *               gtk_thread;

  /** Status text to be used in the splash screen. */
  char                    status[800];

  /** Semaphore for setting the progress in the
   * splash screen from a non-gtk thread. */
  ZixSem                  progress_status_lock;

  /** Log settings. */
  Log                     log;

  /** Flag to set when initialization has
   * finished. */
  int                     init_finished;

  /**
   * Progress done (0.0 ~ 1.0).
   *
   * To be used in things like the splash screen,
   * loading projects, etc.
   */
  double                  progress;

  /** 1 if Zrythm has a UI, 0 if headless (eg, when
   * unit-testing). */
  int                     have_ui;
} Zrythm;

/**
 * The global struct.
 *
 * Contains data that is irrelevant to the project.
 */
struct _ZrythmApp
{
  GtkApplication      parent;

  Zrythm *            zrythm;
};

/**
 * Global variable, should be available to all files.
 */
extern Zrythm * zrythm;
extern ZrythmApp * zrythm_app;

ZrythmApp *
zrythm_app_new (void);

void
zrythm_add_to_recent_projects (
  Zrythm * self,
  const char * filepath);

void
zrythm_remove_recent_project (
  char * filepath);

/**
 * Returns the version string.
 *
 * Must be g_free()'d.
 *
 * @param with_v Include a starting "v".
 */
char *
zrythm_get_version (
  int with_v);

/**
 * Returns the veresion and the capabilities.
 */
void
zrythm_get_version_with_capabilities (
  char * str);

/**
 * Gets the zrythm directory, either from the
 * settings if non-empty, or the default
 * ($XDG_DATA_DIR/zrythm).
 *
 * @param force_default Ignore the settings and get
 *   the default dir.
 *
 * Must be free'd by caler.
 */
char *
zrythm_get_dir (
  bool  force_default);

/**
 * Returns the prefix or in the case of windows
 * the root dir (C/program files/zrythm) or in the
 * case of macos the bundle path.
 *
 * In all cases, "share" is expected to be found
 * in this dir.
 *
 * @return A newly allocated string.
 */
char *
zrythm_get_prefix (void);

/**
 * Returns the datadir ("share" under whatever is
 * returned by zrythm_get_prefix().
 *
 * @return A newly allocated string.
 */
char *
zrythm_get_datadir (void);

/**
 * Returns the bindir ("bin" under whatever is
 * returned by zrythm_get_prefix().
 *
 * @return A newly allocated string.
 */
char *
zrythm_get_bindir (void);

/**
 * Returns the samples directory.
 *
 * @return A newly allocated string.
 */
char *
zrythm_get_samplesdir (void);

/**
 * Returns the locale directory (share/locale).
 *
 * @return A newly allocated string.
 */
char *
zrythm_get_localedir (void);

/**
 * Sets the current status and progress percentage
 * during loading.
 *
 * The splash screen then reads these values from
 * the Zrythm struct.
 */
void
zrythm_set_progress_status (
  Zrythm *     self,
  const char * text,
  const double perc);

/**
 * Called immediately after the main GTK loop
 * terminates.
 *
 * This is also called manually on SIGINT.
 */
void
zrythm_on_shutdown (
  GApplication * application,
  ZrythmApp *    self);

/**
 * @}
 */

#endif /* __ZRYTHM_H__ */
