// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/backend/project.h"

#include <QtQmlIntegration>

#include "recent_projects_model.h"

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
    Project * activeProject READ getActiveProject WRITE setActiveProject NOTIFY
      activeProjectChanged)

public:
  ProjectManager (QObject * parent = nullptr);

  using Template = fs::path;
  using TemplateList = std::vector<Template>;
  using ProjectLoadResult = std::variant<Project *, QString>;

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

  Project * getActiveProject () const;
  void      setActiveProject (Project * project);

Q_SIGNALS:
  void projectLoaded (Project * project);
  void projectLoadingFailed (const QString &errorMessage);
  void activeProjectChanged (Project * project);

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
  Project * create_default (
    const fs::path          &prj_dir,
    const utils::Utf8String &name,
    bool                     with_engine);

private:
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
   * @brief Currently active project.
   *
   * @note We manage lifetime via Qt, so remember to use `setParent()`.
   */
  Project * active_project_ = nullptr;

  /**
   * @brief Future watcher for when a project is loaded (or fails to load).
   *
   * This is used to notify the UI when the project is loaded.
   */
  QFutureWatcher<ProjectLoadResult> project_watcher_;
};

} /// namespace zrythm::gui
