// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Project helper.
 */

#ifndef __PROJECT_PROJECT_INIT_FLOW_MANAGER_H__
#define __PROJECT_PROJECT_INIT_FLOW_MANAGER_H__

#include "zrythm-config.h"

#include <functional>
#include <string>
#include <string_view>

#include "utils/types.h"

class Project;

/**
 * @addtogroup project
 *
 * @{
 */

/**
 * This struct handles the initialization flow when creating a new project
 * or loading a project.
 *
 * It uses callbacks to notify an interested party when it finishes each step.
 */
class ProjectInitFlowManager
{
public:
  /**
   * Callback to call when project initialization (loading or creating new)
   * finishes.
   */
  using ProjectInitDoneCallback =
    std::function<void (bool, std::string, void *)>;

  /**
   * If filename set, it loads that, otherwise it loads the default project.
   *
   * @note This function is async when running normally.
   *
   * @param filename The filename to open. This will be the template in the case
   * of template, or the actual project otherwise.
   * @param is_template Load the project as a template and create a new project
   * from it.
   * @param cb Callback to call when finished.
   * @param user_data User data to pass to @p cb.
   */
  ProjectInitFlowManager (
    std::string_view        filename,
    const bool              is_template,
    ProjectInitDoneCallback cb,
    void *                  user_data);
  ~ProjectInitFlowManager ();

  /**
   * @brief This is public because it's called on a flow manager from a GTK
   * callback.
   */
  void continue_load_from_file_after_open_backup_response ();

private:
  static void
  load_from_file_ready_cb (bool success, std::string error, void * user_data);

  static void cleanup_flow_mgr_cb (
    bool                     success,
    std::string              error,
    ProjectInitFlowManager * flow_mgr);

  void append_callback (ProjectInitDoneCallback cb, void * user_data);

  void call_last_callback (bool success, std::string error);
  void call_last_callback_fail (std::string error);
  void call_last_callback_success ();

  static void recreate_main_window ();

  // static void replace_main_window (MainWindowWidget * mww);

  // static MainWindowWidget * hide_prev_main_window (void);

  // static void destroy_prev_main_window (MainWindowWidget * mww);

  // static void setup_main_window (Project &project);

  /**
   * Called when a new project is created or one is loaded to save the project
   * and, if succeeded, activate it.
   */
  void save_and_activate_after_successful_load_or_create ();

#if HAVE_CYAML
  /**
   * Upgrades the given project YAML's schema if needed.
   *
   * @throw ZrythmException if an error occurred.
   */
  [[gnu::cold]] static void upgrade_schema (char ** yaml, int src_ver);

  /**
   * @brief Upgrades from YAML to JSON.
   *
   * @param txt
   * @throw ZrythmException if an error occurred.
   */
  [[gnu::cold]] static void upgrade_to_json (char ** txt);
#endif

  /**
   * Creates a default project.
   *
   * This is only used internally or for generating projects from scripts.
   *
   * @param prj_dir The directory of the project to create, including its title.
   * @param headless Create the project assuming we are running without a UI.
   * @param with_engine Whether to also start the engine after creating the
   * project.
   * @throw ZrythmException if an error occurred.
   */
  static void create_default (
    std::unique_ptr<Project> &self,
    std::string              &prj_dir,
    bool                      headless = false,
    bool                      with_engine = true);

  /**
   * @brief Loads a project from a file (either actual project or template).
   *
   */
  void load_from_file ();

public:
  unsigned long open_backup_response_cb_id_ = 0;

private:
  /**
   * @brief The filename to open. This will be the template in the case of
   * template, or the actual project otherwise.
   */
  std::string filename_;

  /**
   * @brief Load the project as a template and create a new project from it.
   */
  bool is_template_ = false;

  /** Same as Project.dir. */
  std::string dir_;

  /** Callback/user data pairs. */
  std::vector<std::pair<ProjectInitDoneCallback, void *>> callbacks_;
};

/**
 * @}
 */

#endif
