#pragma once

#include <QtQmlIntegration>

#include "recent_projects_model.h"

namespace zrythm::gui
{

class ProjectManager : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    RecentProjectsModel * recentProjects READ getRecentProjects CONSTANT)

public:
  ProjectManager (QObject * parent = nullptr);

  using Template = fs::path;
  using TemplateList = std::vector<Template>;

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

  void add_to_recent_projects (const QString &path);

  RecentProjectsModel * getRecentProjects () const;

private:
  /**
   * Initializes the array of project templates.
   */
  void init_templates ();

private:
  /** Array of project template paths. */
  TemplateList templates_;

  /**
   * Demo project template used when running for the first time.
   *
   * This is normally part of @ref templates_.
   */
  Template demo_template_;

  RecentProjectsModel * recent_projects_model_;
};

} /// namespace zrythm::gui