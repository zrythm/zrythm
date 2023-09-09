// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * The main Zrythm struct.
 */

#ifndef __ZRYTHM_H__
#define __ZRYTHM_H__

#include "zrythm-config.h"

#include <stdbool.h>

#include <glib.h>

#include "zix/sem.h"
#include <gio/gio.h>

typedef struct Project                Project;
typedef struct Symap                  Symap;
typedef struct RecordingManager       RecordingManager;
typedef struct EventManager           EventManager;
typedef struct PluginManager          PluginManager;
typedef struct FileManager            FileManager;
typedef struct ChordPresetPackManager ChordPresetPackManager;
typedef struct Settings               Settings;
typedef struct Log                    Log;
typedef struct CairoCaches            CairoCaches;
typedef struct PCGRand                PCGRand;

/**
 * @addtogroup general
 *
 * @{
 */

#define ZRYTHM (zrythm)

#define ZRYTHM_PROJECTS_DIR "projects"

#define MAX_RECENT_PROJECTS 20
#define DEBUGGING (G_UNLIKELY (ZRYTHM && ZRYTHM->debug))
#define ZRYTHM_TESTING (G_UNLIKELY (ZRYTHM && ZRYTHM->testing))
#define ZRYTHM_GENERATING_PROJECT (ZRYTHM->generating_project)
#define ZRYTHM_HAVE_UI (ZRYTHM && ZRYTHM->have_ui)

#ifdef HAVE_LSP_DSP
#  define ZRYTHM_USE_OPTIMIZED_DSP (G_LIKELY (ZRYTHM->use_optimized_dsp))
#else
#  define ZRYTHM_USE_OPTIMIZED_DSP false
#endif

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

  /** libdir name under
   * \ref ZRYTHM_DIR_SYSTEM_PREFIX. */
  ZRYTHM_DIR_SYSTEM_PARENT_LIBDIR,

  /** libdir/zrythm */
  ZRYTHM_DIR_SYSTEM_ZRYTHM_LIBDIR,

  /** libdir/zrythm/lv2 */
  ZRYTHM_DIR_SYSTEM_BUNDLED_PLUGINSDIR,

  /** Localization under "share". */
  ZRYTHM_DIR_SYSTEM_LOCALEDIR,

  /**
   * "gtksourceview-5/language-specs" under
   * "share".
   */
  ZRYTHM_DIR_SYSTEM_SOURCEVIEW_LANGUAGE_SPECS_DIR,

  /**
   * "gtksourceview-5/language-specs" under
   * "share/zrythm".
   */
  ZRYTHM_DIR_SYSTEM_BUNDLED_SOURCEVIEW_LANGUAGE_SPECS_DIR,

  /** share/zrythm */
  ZRYTHM_DIR_SYSTEM_ZRYTHM_DATADIR,

  /** Samples. */
  ZRYTHM_DIR_SYSTEM_SAMPLESDIR,

  /** Scripts. */
  ZRYTHM_DIR_SYSTEM_SCRIPTSDIR,

  /** Themes. */
  ZRYTHM_DIR_SYSTEM_THEMESDIR,

  /** CSS themes. */
  ZRYTHM_DIR_SYSTEM_THEMES_CSS_DIR,

  /** Icon themes. */
  ZRYTHM_DIR_SYSTEM_THEMES_ICONS_DIR,

  /**
   * Special external Zrythm plugins path (not
   * part of the Zrythm source code).
   *
   * Used for ZLFO and other plugins.
   */
  ZRYTHM_DIR_SYSTEM_SPECIAL_LV2_PLUGINS_DIR,

  /** The directory fonts/zrythm under datadir. */
  ZRYTHM_DIR_SYSTEM_FONTSDIR,

  /** Project templates. */
  ZRYTHM_DIR_SYSTEM_TEMPLATES,

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

  /** User CSS themes. */
  ZRYTHM_DIR_USER_THEMES_CSS,

  /** User icon themes. */
  ZRYTHM_DIR_USER_THEMES_ICONS,

  /** User scripts. */
  ZRYTHM_DIR_USER_SCRIPTS,

  /** Log files. */
  ZRYTHM_DIR_USER_LOG,

  /** Profiling files. */
  ZRYTHM_DIR_USER_PROFILING,

  /** Gdb backtrace files. */
  ZRYTHM_DIR_USER_GDB,

  /** Backtraces. */
  ZRYTHM_DIR_USER_BACKTRACE,

} ZrythmDirType;

/**
 * To be used throughout the program.
 *
 * Everything here should be global and function
 * regardless of the project.
 */
typedef struct Zrythm
{
  /** argv[0]. */
  const char * exe_path;

  /**
   * Manages plugins (loading, instantiating, etc.)
   */
  PluginManager * plugin_manager;

  /**
   * Application settings
   */
  Settings * settings;

  /**
   * Project data.
   *
   * This is what should be exported/imported when
   * saving/loading projects.
   *
   * The only reason this is a pointer is to easily
   * deserialize.
   */
  Project * project;

  /** +1 to ensure last element is NULL in case full. */
  char * recent_projects[MAX_RECENT_PROJECTS + 1];
  int    num_recent_projects;

  /** NULL terminated array of project template absolute
   * paths. */
  char ** templates;

  /**
   * Demo project template used when running for the first
   * time.
   *
   * This is a copy of one of the strings in Zrythm.templates.
   */
  char * demo_template;

  /** Whether the open file is a template to be used
   * to create a new project from. */
  bool opening_template;

  /** Whether creating a new project, either from
   * a template or blank. */
  bool creating_project;

  /** Path to create a project in, including its
   * title. */
  char * create_project_path;

  /**
   * Filename to open passed through the command
   * line.
   *
   * Used only when a filename is passed.
   * E.g., zrytm myproject.xml
   */
  char * open_filename;

  EventManager * event_manager;

  /** Recording manager. */
  RecordingManager * recording_manager;

  /** File manager. */
  FileManager * file_manager;

  /** Chord preset pack manager. */
  ChordPresetPackManager * chord_preset_pack_manager;

  /**
   * String interner for internal things.
   */
  Symap * symap;

  /**
   * String interner for error domains.
   */
  Symap * error_domain_symap;

  /** Random number generator. */
  PCGRand * rand;

  /**
   * In debug mode or not (determined by GSetting).
   */
  bool debug;

  /**
   * Used when running the tests.
   *
   * This is set by the TESTING environment variable.
   */
  bool testing;

  /** Whether this is a dummy instance used when
   * generating projects. */
  bool generating_project;

  /** Log settings. */
  //Log *               log;

  /**
   * Progress done (0.0 ~ 1.0).
   *
   * To be used in things like the splash screen,
   * loading projects, etc.
   */
  double progress;

  /** 1 if Zrythm has a UI, 0 if headless (eg, when
   * unit-testing). */
  bool have_ui;

  /** Whether to use optimized DSP when
   * available. */
  bool use_optimized_dsp;

  CairoCaches * cairo_caches;

  /** Zrythm directory used during unit tests. */
  char * testing_dir;

  /** Undo stack length, used during tests. */
  int undo_stack_len;

  /** Cached version (without 'v'). */
  char * version;

  /**
   * Whether to open a newer backup if found.
   *
   * This is only used during tests where there
   * is no UI to choose.
   */
  bool open_newer_backup;

  /**
   * Whether to use pipewire in tests.
   *
   * If this is false, the dummy engine will be used.
   *
   * Some tests do sample rate changes so it's more convenient
   * to use the dummy engine instead.
   */
  bool use_pipewire_in_tests;

  /** Process ID for pipewire (used in tests). */
  GPid pipewire_pid;
} Zrythm;

/**
 * Global variable, should be available to all files.
 */
extern Zrythm * zrythm;

void
zrythm_add_to_recent_projects (Zrythm * self, const char * filepath);

void
zrythm_remove_recent_project (char * filepath);

/**
 * Returns the version string.
 *
 * Must be g_free()'d.
 *
 * @param with_v Include a starting "v".
 */
MALLOC
char *
zrythm_get_version (bool with_v);

/**
 * Returns whether the current Zrythm version is a
 * release version.
 *
 * @note This only does regex checking.
 */
bool
zrythm_is_release (bool official);

char *
zrythm_fetch_latest_release_ver_finish (GAsyncResult * result, GError ** error);

/**
 * @param callback A GAsyncReadyCallback to call when the
 *   request is satisfied.
 * @param callback_data Data to pass to @p callback.
 */
void
zrythm_fetch_latest_release_ver_async (
  GAsyncReadyCallback callback,
  gpointer            callback_data);

/**
 * Returns whether the given release string is the latest
 * release.
 */
bool
zrythm_is_latest_release (const char * remote_latest_release);

/**
 * Returns the version and the capabilities.
 *
 * @param buf Buffer to write the string to.
 * @param include_system_info Whether to include
 *   additional system info (for bug reports).
 */
void
zrythm_get_version_with_capabilities (char * buf, bool include_system_info);

/**
 * Returns system info (mainly used for bug
 * reports).
 */
char *
zrythm_get_system_info (void);

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
zrythm_get_dir (ZrythmDirType type);

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
 * Must be free'd by caller.
 */
char *
zrythm_get_user_dir (bool force_default);

/**
 * Initializes/creates the default dirs/files in the user
 * directory.
 *
 * @return Whether successful.
 */
NONNULL bool
zrythm_init_user_dirs_and_files (Zrythm * self, GError ** error);

/**
 * Initializes the array of project templates.
 */
NONNULL void
zrythm_init_templates (Zrythm * self);

/**
 * Creates a new Zrythm instance.
 *
 * @param have_ui Whether Zrythm is instantiated
 *   with a UI (false if headless).
 * @param testing Whether this is a unit test.
 */
Zrythm *
zrythm_new (const char * exe_path, bool have_ui, bool testing, bool optimized_dsp);

/**
 * Frees the instance and any unfreed members.
 */
void
zrythm_free (Zrythm * self);

/**
 * @}
 */

#endif /* __ZRYTHM_H__ */
