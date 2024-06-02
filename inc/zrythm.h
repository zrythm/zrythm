// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * The main Zrythm struct.
 */

#ifndef __ZRYTHM_H__
#define __ZRYTHM_H__

#include "zrythm-config.h"

#include <memory>

#include <gio/gio.h>
#include <glib.h>

#include "ext/juce/juce.h"
#include "zix/sem.h"

struct Project;
typedef struct Symap                  Symap;
typedef struct RecordingManager       RecordingManager;
typedef struct EventManager           EventManager;
typedef struct PluginManager          PluginManager;
typedef struct FileManager            FileManager;
typedef struct ChordPresetPackManager ChordPresetPackManager;
class Settings;
typedef struct Log         Log;
typedef struct CairoCaches CairoCaches;
typedef struct PCGRand     PCGRand;
class StringArray;

/**
 * @addtogroup general
 *
 * @{
 */

#define ZRYTHM_PROJECTS_DIR "projects"

#define MAX_RECENT_PROJECTS 20
#define DEBUGGING (G_UNLIKELY (gZrythm && gZrythm->debug))
#define ZRYTHM_TESTING (g_test_initialized ())
#define ZRYTHM_GENERATING_PROJECT (gZrythm->generating_project)
#define ZRYTHM_HAVE_UI (gZrythm && gZrythm->have_ui_)

#ifdef HAVE_LSP_DSP
#  define ZRYTHM_USE_OPTIMIZED_DSP (G_LIKELY (gZrythm->use_optimized_dsp))
#else
#  define ZRYTHM_USE_OPTIMIZED_DSP false
#endif

/**
 * Type of Zrythm directory.
 *
 * System* directories are directories that are determined during installation
 * that contain immutable data.
 *
 * User* directories are directories determined based on the user or the user's
 * preferences that contain user-modifiable data.
 */
typedef enum ZrythmDirType
{
  /**
   * The prefix, or in the case of windows installer the root dir (C/program
   * files/zrythm), or in the case of macos installer the bundle path.
   *
   * In all cases, "share" is expected to be found in this dir.
   */
  SYSTEM_PREFIX,

  /** "bin" under \ref SYSTEM_PREFIX. */
  SYSTEM_BINDIR,

  /** "share" under \ref SYSTEM_PREFIX. */
  SYSTEM_PARENT_DATADIR,

  /** libdir name under
   * \ref SYSTEM_PREFIX. */
  SYSTEM_PARENT_LIBDIR,

  /** libdir/zrythm */
  SYSTEM_ZRYTHM_LIBDIR,

  /** libdir/zrythm/lv2 */
  SYSTEM_BUNDLED_PLUGINSDIR,

  /** Localization under "share". */
  SYSTEM_LOCALEDIR,

  /**
   * "gtksourceview-5/language-specs" under "share".
   */
  SYSTEM_SOURCEVIEW_LANGUAGE_SPECS_DIR,

  /**
   * "gtksourceview-5/language-specs" under "share/zrythm".
   */
  SYSTEM_BUNDLED_SOURCEVIEW_LANGUAGE_SPECS_DIR,

  /** share/zrythm */
  SYSTEM_ZRYTHM_DATADIR,

  /** Samples. */
  SYSTEM_SAMPLESDIR,

  /** Scripts. */
  SYSTEM_SCRIPTSDIR,

  /** Themes. */
  SYSTEM_THEMESDIR,

  /** CSS themes. */
  SYSTEM_THEMES_CSS_DIR,

  /** Icon themes. */
  SYSTEM_THEMES_ICONS_DIR,

  /**
   * Special external Zrythm plugins path (not part of the Zrythm source code).
   *
   * Used for ZLFO and other plugins.
   */
  SYSTEM_SPECIAL_LV2_PLUGINS_DIR,

  /** The directory fonts/zrythm under datadir. */
  SYSTEM_FONTSDIR,

  /** Project templates. */
  SYSTEM_TEMPLATES,

  /* ************************************ */

  /** Main zrythm directory from gsettings. */
  USER_TOP,

  /** Subdirs of \ref USER_TOP. */
  USER_PROJECTS,
  USER_TEMPLATES,
  USER_THEMES,

  /** User CSS themes. */
  USER_THEMES_CSS,

  /** User icon themes. */
  USER_THEMES_ICONS,

  /** User scripts. */
  USER_SCRIPTS,

  /** Log files. */
  USER_LOG,

  /** Profiling files. */
  USER_PROFILING,

  /** Gdb backtrace files. */
  USER_GDB,

  /** Backtraces. */
  USER_BACKTRACE,

} ZrythmDirType;

class ZrythmDirectoryManager
{
public:
  /**
   * Gets the zrythm directory, either from the settings if non-empty, or the
   * default ($XDG_DATA_DIR/zrythm).
   *
   * @param force_default Ignore the settings and get the default dir.
   *
   * Must be free'd by caller.
   */
  char * get_user_dir (bool force_default);

  /**
   * Returns the default user "zrythm" dir.
   *
   * This is used when resetting or when the dir is not selected by the user yet.
   */
  char * get_default_user_dir ();

  /**
   * Returns a Zrythm directory specified by
   * \ref type.
   *
   * @return A newly allocated string.
   */
  char * get_dir (ZrythmDirType type);

  /** Zrythm directory used during unit tests. */
  char * testing_dir = nullptr;
};

/**
 * To be used throughout the program.
 *
 * Everything here should be global and function regardless of the project.
 */
class Zrythm
{
public:
  /**
   * @param have_ui Whether Zrythm is instantiated with a UI (false if headless).
   * @param testing Whether this is a unit test.
   */
  explicit Zrythm (const char * exe_path, bool have_ui, bool optimized_dsp);

  ~Zrythm ();

  void init ();

  void add_to_recent_projects (const char * filepath);

  void remove_recent_project (char * filepath);

  /**
   * Returns the version string.
   *
   * Must be g_free()'d.
   *
   * @param with_v Include a starting "v".
   */
  static char * get_version (bool with_v);

  /**
   * Returns whether the current Zrythm version is a
   * release version.
   *
   * @note This only does regex checking.
   */
  static bool is_release (bool official);

  static char *
  fetch_latest_release_ver_finish (GAsyncResult * result, GError ** error);

  /**
   * @param callback A GAsyncReadyCallback to call when the
   *   request is satisfied.
   * @param callback_data Data to pass to @p callback.
   */
  static void fetch_latest_release_ver_async (
    GAsyncReadyCallback callback,
    gpointer            callback_data);

  /**
   * Returns whether the given release string is the latest
   * release.
   */
  static bool is_latest_release (const char * remote_latest_release);

  /**
   * Returns the version and the capabilities.
   *
   * @param buf Buffer to write the string to.
   * @param include_system_info Whether to include
   *   additional system info (for bug reports).
   */
  static void
  get_version_with_capabilities (char * buf, bool include_system_info);

  /**
   * Returns system info (mainly used for bug
   * reports).
   */
  static char * get_system_info ();

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
  static char * get_prefix ();

  /**
   * Initializes/creates the default dirs/files in the user
   * directory.
   *
   * @return Whether successful.
   */
  bool init_user_dirs_and_files (GError ** error);

  /**
   * Initializes the array of project templates.
   */
  void init_templates ();

  /** argv[0]. */
  const char * exe_path_ = nullptr;

  /**
   * Manages plugins (loading, instantiating, etc.)
   */
  PluginManager * plugin_manager = nullptr;

  /**
   * Application settings
   */
  std::unique_ptr<Settings> settings;

  /**
   * Project data.
   *
   * This is what should be exported/imported when saving/loading projects.
   *
   * The only reason this is a pointer is to easily deserialize.
   */
  Project * project = nullptr;

  /** +1 to ensure last element is NULL in case full. */
  std::unique_ptr<StringArray> recent_projects_;

  /** NULL terminated array of project template absolute paths. */
  char ** templates = nullptr;

  /**
   * Demo project template used when running for the first time.
   *
   * This is a copy of one of the strings in Zrythm.templates.
   */
  char * demo_template = nullptr;

  /** Whether the open file is a template to be used to create a new project
   * from. */
  bool opening_template = false;

  /** Whether creating a new project, either from a template or blank. */
  bool creating_project = false;

  /** Path to create a project in, including its title. */
  char * create_project_path = nullptr;

  /**
   * Filename to open passed through the command
   * line.
   *
   * Used only when a filename is passed.
   * E.g., zrytm myproject.xml
   */
  char * open_filename = nullptr;

  EventManager * event_manager = nullptr;

  /** Recording manager. */
  RecordingManager * recording_manager = nullptr;

  /** File manager. */
  FileManager * file_manager = nullptr;

  /** Chord preset pack manager. */
  ChordPresetPackManager * chord_preset_pack_manager = nullptr;

  std::unique_ptr<ZrythmDirectoryManager> dir_mgr;

  /**
   * String interner for internal things.
   */
  Symap * symap = nullptr;

  /**
   * String interner for error domains.
   */
  Symap * error_domain_symap = nullptr;

  /** Random number generator. */
  PCGRand * rand = nullptr;

  /**
   * In debug mode or not (determined by GSetting).
   */
  bool debug = false;

  /** Whether this is a dummy instance used when
   * generating projects. */
  bool generating_project = false;

  /** 1 if Zrythm has a UI, 0 if headless (eg, when
   * unit-testing). */
  bool have_ui_ = false;

  /** Whether to use optimized DSP when available. */
  bool use_optimized_dsp = false;

  CairoCaches * cairo_caches = nullptr;

  /** Undo stack length, used during tests. */
  int undo_stack_len = 0;

  /** Cached version (without 'v'). */
  char * version = nullptr;

  /**
   * Whether to open a newer backup if found.
   *
   * This is only used during tests where there is no UI to choose.
   */
  bool open_newer_backup = false;

  /**
   * Whether to use pipewire in tests.
   *
   * If this is false, the dummy engine will be used.
   *
   * Some tests do sample rate changes so it's more convenient
   * to use the dummy engine instead.
   */
  bool use_pipewire_in_tests = false;

  /** Process ID for pipewire (used in tests). */
  GPid pipewire_pid = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Zrythm)
};

/**
 * Global variable, should be available to all files.
 */
extern std::unique_ptr<Zrythm> gZrythm;

/**
 * @}
 */

#endif /* __ZRYTHM_H__ */
