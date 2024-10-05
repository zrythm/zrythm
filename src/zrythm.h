// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __ZRYTHM_H__
#define __ZRYTHM_H__

#include "zrythm-config.h"

#include <memory>

#include "dsp/recording_manager.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/file_manager.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/chord_preset_pack_manager.h"
#include "settings/settings.h"
#include "utils/lsp_dsp_context.h"
#include "utils/networking.h"
#include "utils/string_array.h"
#include "utils/symap.h"

#include <gio/gio.h>
#include <glib.h>

#include "juce/juce.h"

/**
 * @addtogroup general
 *
 * @{
 */

#define ZRYTHM_PROJECTS_DIR "projects"

#define MAX_RECENT_PROJECTS 20
#define DEBUGGING (G_UNLIKELY (gZrythm && gZrythm->debug_))
#define ZRYTHM_BENCHMARKING (gZrythm && gZrythm->benchmarking_)
#define ZRYTHM_GENERATING_PROJECT (gZrythm->generating_project_)
#define ZRYTHM_HAVE_UI (gZrythm && gZrythm->have_ui_)
#define ZRYTHM_BREAK_ON_ERROR (gZrythm && gZrythm->break_on_error_)

#if HAVE_LSP_DSP
#  define ZRYTHM_USE_OPTIMIZED_DSP (G_LIKELY (gZrythm->use_optimized_dsp_))
#else
#  define ZRYTHM_USE_OPTIMIZED_DSP false
#endif

/**
 * To be used throughout the program.
 *
 * Everything here should be global and function regardless of the project.
 */
class Zrythm
{
public:
  ~Zrythm ();

  /**
   * @brief Called before @ref init().
   *
   * TODO: check if can be merged into init().
   *
   * @param have_ui Whether Zrythm is instantiated with a UI (false if headless).
   * @param testing Whether this is a unit test.
   */
  void pre_init (const char * exe_path, bool have_ui, bool optimized_dsp);

  void init ();

  void add_to_recent_projects (const std::string &filepath);

  void remove_recent_project (const std::string &filepath);

  /**
   * Returns the version string.
   *
   * @param with_v Include a starting "v".
   */
  static std::string get_version (bool with_v);

  /**
   * Returns whether the current Zrythm version is a
   * release version.
   *
   * @note This only does regex checking.
   */
  static bool is_release (bool official);

  /**
   * @param callback A GAsyncReadyCallback to call when the
   *   request is satisfied.
   * @param callback_data Data to pass to @p callback.
   */
  static void fetch_latest_release_ver_async (
    networking::URL::GetContentsAsyncCallback callback);

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
  static std::string get_system_info ();

  /**
   * Initializes/creates the default dirs/files in the user directory.
   *
   * @throw ZrythmException If an error occured.
   */
  void init_user_dirs_and_files ();

  /**
   * Initializes the array of project templates.
   */
  void init_templates ();

  FileManager &get_file_manager () { return file_manager_; }

private:
  Zrythm () = default;

public:
  /** argv[0]. */
  std::string exe_path_;

  /**
   * Application settings
   */
  std::unique_ptr<Settings> settings_;

  /** +1 to ensure last element is NULL in case full. */
  StringArray recent_projects_;

  /** NULL terminated array of project template absolute paths. */
  StringArray templates_;

  /**
   * Demo project template used when running for the first time.
   *
   * This is a copy of one of the strings in Zrythm.templates.
   */
  std::string demo_template_;

  /** Whether the open file is a template to be used to create a new project
   * from. */
  bool opening_template_ = false;

  /** Whether creating a new project, either from a template or blank. */
  bool creating_project_ = false;

  /** Path to create a project in, including its title. */
  std::string create_project_path_;

  /**
   * Filename to open passed through the command line.
   *
   * Used only when a filename is passed, eg, zrytm myproject.zpj
   */
  std::string open_filename_;

  /** File manager. */
  FileManager file_manager_;

  /**
   * String interner for internal things.
   */
  Symap symap_;

  /**
   * In debug mode or not (determined by GSetting).
   */
  bool debug_ = false;

  /** Whether to abort() on an error log message. */
  bool break_on_error_ = false;

  /** Whether this is a dummy instance used when
   * generating projects. */
  bool generating_project_ = false;

  /** 1 if Zrythm has a UI, 0 if headless (eg, when
   * unit-testing). */
  bool have_ui_ = false;

  /** Whether to use optimized DSP when available. */
  bool use_optimized_dsp_ = false;

  /** Undo stack length, used during tests. */
  int undo_stack_len_ = 0;

  /**
   * Whether to open a newer backup if found.
   *
   * This is only used during tests where there is no UI to choose.
   */
  bool open_newer_backup_ = false;

  /**
   * Whether to use pipewire in tests.
   *
   * If this is false, the dummy engine will be used.
   *
   * Some tests do sample rate changes so it's more convenient
   * to use the dummy engine instead.
   */
  bool use_pipewire_in_tests_ = false;

  /** Process ID for pipewire (used in tests). */
  GPid pipewire_pid_ = 0;

  /** Chord preset pack manager. */
  std::unique_ptr<ChordPresetPackManager> chord_preset_pack_manager_;

  std::unique_ptr<EventManager> event_manager_;

  /**
   * Manages plugins (loading, instantiating, etc.)
   */
  std::unique_ptr<PluginManager> plugin_manager_;

  /** Recording manager. */
  std::unique_ptr<RecordingManager> recording_manager_;

  /**
   * Project data.
   *
   * This is what should be exported/imported when saving/loading projects.
   *
   * The only reason this is a pointer is to easily deserialize.
   *
   * Should be free'd first, therefore it is the last member.
   */
  std::unique_ptr<Project> project_;

  /**
   * @brief LSP DSP context for the main thread.
   */
  std::unique_ptr<LspDspContextRAII> lsp_dsp_context_;

  /**
   * @brief Whether currently running under the benchmarker.
   */
  bool benchmarking_ = false;

  JUCE_DECLARE_SINGLETON_SINGLETHREADED (Zrythm, false)

  JUCE_HEAVYWEIGHT_LEAK_DETECTOR (Zrythm)
};

#define gZrythm (Zrythm::getInstanceWithoutCreating ())

/**
 * @}
 */

#endif /* __ZRYTHM_H__ */
