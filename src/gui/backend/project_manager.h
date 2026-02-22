// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/project_session.h"
#include "gui/backend/recent_projects_model.h"

#include <QFutureWatcher>
#include <QtQmlIntegration/qqmlintegration.h>

// DEPRECATED - do not use in new code
#define PROJECT \
  (zrythm::gui::ProjectManager::get_instance ()->activeSession ()->project ())

namespace zrythm::gui
{

class ProjectManager : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::gui::RecentProjectsModel * recentProjects READ getRecentProjects
      CONSTANT)
  Q_PROPERTY (
    zrythm::gui::ProjectSession * activeSession READ activeSession WRITE
      setActiveSession NOTIFY activeSessionChanged)
  QML_UNCREATABLE ("")

public:
  ProjectManager (utils::AppSettings &app_settings, QObject * parent = nullptr);

  using Template = fs::path;
  using TemplateList = std::vector<Template>;
  using ProjectLoadResult =
    std::variant<utils::QObjectUniquePtr<ProjectSession>, QString>;

  static ProjectManager * get_instance ();

  /**
   * @brief Gets a copy of the list of project templates.
   */
  TemplateList get_templates () const;

  /**
   * @brief Get the next available untitled project name for the given directory.
   *
   * @param directory Direcctory to check for untitled projects.
   */
  Q_INVOKABLE static QString
  getNextAvailableProjectName (const QUrl &directory, const QString &name);

  Q_INVOKABLE void createNewProject (
    const QUrl    &directory,
    const QString &name,
    const QUrl    &templateUrl = QUrl{});
  Q_INVOKABLE void loadProject (const QString &filepath);

  void add_to_recent_projects (const QString &path);

  RecentProjectsModel * getRecentProjects () const;

  ProjectSession * activeSession () const;
  void             setActiveSession (ProjectSession * project);

Q_SIGNALS:
  void projectLoaded (ProjectSession * project);
  void projectLoadingFailed (const QString &errorMessage);
  void activeSessionChanged (ProjectSession * project);

private:
  /**
   * Initializes the array of project templates.
   */
  void init_templates ();

  // void create_or_load_project (const QString &filepath, bool is_template);

  /**
   * @brief Loads a project from a file (either actual project or template).
   *
   */
  void load_from_file ();

  /**
   * Creates a default project.
   *
   * This is only used internally or for generating projects from scripts.
   *
   * @param prj_dir The parent directory of the project to create (without the
   * project name).
   * @param name The name/title of the project to create. This will be appended
   * to @p prj_dir.
   * @throw ZrythmException if an error occurred.
   */
  utils::QObjectUniquePtr<ProjectSession> create_default (
    const fs::path          &prj_dir,
    const utils::Utf8String &name,
    bool                     with_engine);

  /**
   * @brief Creates a toplevel window for the given plugin
   *
   * This method can be used as a PluginHostWindowFactory.
   */
  std::unique_ptr<plugins::IPluginHostWindow>
  create_window_for_plugin (plugins::Plugin &plugin) const;

private:
  utils::AppSettings &app_settings_;

  /** Array of project template paths. */
  TemplateList templates_;

  /**
   * Demo project template used when running for the first time.
   *
   * This is normally part of @ref templates_.
   */
  Template demo_template_;

  RecentProjectsModel * recent_projects_model_ = nullptr;

  /**
   * @brief Currently active project session.
   */
  utils::QObjectUniquePtr<ProjectSession> active_session_;

  /**
   * @brief Future watcher for when a project is loaded (or fails to load).
   *
   * This is used to notify the UI when the project is loaded.
   */
  QFutureWatcher<ProjectLoadResult> project_watcher_;
};

} /// namespace zrythm::gui
