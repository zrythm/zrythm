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
 * The main Zrythm struct.
 */

#ifndef __ZRYTHM_H__
#define __ZRYTHM_H__

#include "zix/sem.h"

typedef struct Project Project;
typedef struct Symap Symap;
typedef struct RecordingManager RecordingManager;
typedef struct EventManager EventManager;
typedef struct ObjectUtils ObjectUtils;
typedef struct PluginManager PluginManager;
typedef struct FileManager FileManager;
typedef struct Settings Settings;
typedef struct Log Log;
typedef struct CairoCaches CairoCaches;

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
 * Type of Zrythm directory.
 */
typedef enum ZrythmDirType
{
  /*
   * ----
   * System directories that ship with Zrythm
   * and must not be changed.
   * ----
   */

  /**
   * The prefix, or in the case of windows installer
   * the root dir (C/program files/zrythm), or in the
   * case of macos installer the bundle path.
   *
   * In all cases, "share" is expected to be found
   * in this dir.
   */
  ZRYTHM_DIR_SYSTEM_PREFIX,

  /** "bin" under \ref ZRYTHM_DIR_SYSTEM_PREFIX. */
  ZRYTHM_DIR_SYSTEM_BINDIR,

  /** "share" under \ref ZRYTHM_DIR_SYSTEM_PREFIX. */
  ZRYTHM_DIR_SYSTEM_PARENT_DATADIR,

  /** Localization under "share". */
  ZRYTHM_DIR_SYSTEM_LOCALEDIR,

  /** share/zrythm */
  ZRYTHM_DIR_SYSTEM_ZRYTHM_DATADIR,

  /** Samples. */
  ZRYTHM_DIR_SYSTEM_SAMPLESDIR,

  /** Themes. */
  ZRYTHM_DIR_SYSTEM_THEMESDIR,

  /* ************************************ */

  /*
   * ----
   * Zrythm user directories that contain
   * user-modifiable data.
   * ----
   */

  /** Main zrythm directory from gsettings. */
  ZRYTHM_DIR_USER_TOP,

  /** Subdirs of \ref ZRYTHM_DIR_USER_TOP. */
  ZRYTHM_DIR_USER_PROJECTS,
  ZRYTHM_DIR_USER_TEMPLATES,
  ZRYTHM_DIR_USER_THEMES,

  /** Log files. */
  ZRYTHM_DIR_USER_LOG,

  /** Profiling files. */
  ZRYTHM_DIR_USER_PROFILING,

  /** Gdb backtrace files. */
  ZRYTHM_DIR_USER_GDB,

} ZrythmDirType;

/**
 * To be used throughout the program.
 *
 * Everything here should be global and function
 * regardless of the project.
 */
typedef struct Zrythm
{
  /**
   * Manages plugins (loading, instantiating, etc.)
   */
  PluginManager *     plugin_manager;

  /**
   * Application settings
   */
  Settings *          settings;

  /**
   * Project data.
   *
   * This is what should be exported/imported when
   * saving/loading projects.
   *
   * The only reason this is a pointer is to easily
   * deserialize.
   */
  Project *           project;

  /** +1 to ensure last element is NULL in case
   * full. */
  char *
    recent_projects[MAX_RECENT_PROJECTS + 1];
  int                 num_recent_projects;

  /** NULL terminated array of project template
   * absolute paths. */
  char **             templates;

  /** Whether the open file is a template to be used
   * to create a new project from. */
  bool                opening_template;

  /** Whether creating a new project, either from
   * a template or blank. */
  bool                creating_project;

  /** Path to create a project in, including its
   * title. */
  char *              create_project_path;

  /**
   * Filename to open passed through the command
   * line.
   *
   * Used only when a filename is passed.
   * E.g., zrytm myproject.xml
   */
  char *              open_filename;

  EventManager *      event_manager;

  /** Recording manager. */
  RecordingManager *  recording_manager;

  /** File manager. */
  FileManager *       file_manager;

  /**
   * String interner for internal things.
   */
  Symap *             symap;

  /** Object utils. */
  ObjectUtils *       object_utils;

  /**
   * In debug mode or not (determined by GSetting).
   */
  bool                debug;

  /**
   * Used when running the tests.
   *
   * This is set by the TESTING environment variable.
   */
  bool                testing;

  /** Log settings. */
  Log *               log;

  /**
   * Progress done (0.0 ~ 1.0).
   *
   * To be used in things like the splash screen,
   * loading projects, etc.
   */
  double              progress;

  /** 1 if Zrythm has a UI, 0 if headless (eg, when
   * unit-testing). */
  bool                have_ui;

  CairoCaches *       cairo_caches;

  /** Zrythm directory used during unit tests. */
  char *              testing_dir;
} Zrythm;

/**
 * Global variable, should be available to all files.
 */
extern Zrythm * zrythm;

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
 * Returns the default user "zrythm" dir.
 *
 * This is used when resetting or when the
 * dir is not selected by the user yet.
 */
char *
zrythm_get_default_user_dir (void);

/**
 * Returns a Zrythm directory specified by
 * \ref type.
 *
 * @return A newly allocated string.
 */
char *
zrythm_get_dir (
  ZrythmDirType type);

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
zrythm_get_user_dir (
  bool  force_default);

/**
 * Creates a new Zrythm instance.
 *
 * @param have_ui Whether Zrythm is instantiated
 *   with a UI (false if headless).
 * @param testing Whether this is a unit test.
 */
Zrythm *
zrythm_new (
  bool have_ui,
  bool testing);

/**
 * Frees the instance and any unfreed members.
 */
void
zrythm_free (
  Zrythm * self);

/**
 * @}
 */

#endif /* __ZRYTHM_H__ */
