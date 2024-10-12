#include "common/utils/directory_manager.h"
#include "common/utils/io.h"
#include "gui/backend/zrythm_application.h"
#include "project_manager.h"

#include <QtConcurrent>

using namespace zrythm::gui;

ProjectManager::ProjectManager (QObject * parent)
    : QObject (parent), recent_projects_model_ (new RecentProjectsModel (this))
{
  z_debug ("Initializing project manager...");
  init_templates ();

  QObject::connect (
    &project_watcher_, &QFutureWatcher<ProjectLoadResult>::finished, this,
    [this] () {
      auto project_res = project_watcher_.result ();
      if (project_res.index () == 0)
        {
          auto * project = std::get<Project *> (project_res);
          Q_EMIT projectLoaded (project);
        }
      else
        {
          auto error_msg = std::get<QString> (project_res);
          Q_EMIT projectLoadingFailed (error_msg);
        }
    });
}

void
ProjectManager::init_templates ()
{
  z_info ("Initializing templates...");

  auto *      dir_mgr = DirectoryManager::getInstance ();
  std::string user_templates_dir =
    dir_mgr->get_dir (DirectoryManager::DirectoryType::USER_TEMPLATES);
  if (fs::is_directory (user_templates_dir))
    {
      try
        {
          auto files = io_get_files_in_dir (user_templates_dir);
          std::transform (
            files.begin (), files.end (), std::back_inserter (templates_),
            [] (const auto &p) { return fs::path (p.toStdString ()); });
        }
      catch (const ZrythmException &e)
        {
          z_warning (
            "Failed to init user templates from {}", user_templates_dir);
        }
    }
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      std::string system_templates_dir =
        dir_mgr->get_dir (DirectoryManager::DirectoryType::SYSTEM_TEMPLATES);
      if (fs::is_directory (system_templates_dir))
        {
          try
            {
              auto files = io_get_files_in_dir (system_templates_dir);
              std::transform (
                files.begin (), files.end (), std::back_inserter (templates_),
                [] (const auto &p) { return fs::path (p.toStdString ()); });
            }
          catch (const ZrythmException &e)
            {
              z_warning (
                "Failed to init system templates from {}", system_templates_dir);
            }
        }
    }

  for (auto &tmpl : this->templates_)
    {
      z_info ("Template found: {}", tmpl);
      if (juce::String (tmpl).contains ("demo_zsong01"))
        {
          demo_template_ = tmpl;
        }
    }

  z_debug ("done");
}

ProjectManager *
ProjectManager::get_instance ()
{
  return dynamic_cast<ZrythmApplication *> (qApp)->get_project_manager ();
}

ProjectManager::TemplateList
ProjectManager::get_templates () const
{
  z_trace ("Returning {} templates", templates_.size ());
  return templates_;
}

QString
ProjectManager::getNextAvailableProjectName (
  const QUrl    &directory,
  const QString &name)
{
  /* get next available "Untitled Project" */
  if (!directory.isLocalFile ())
    {
      return name;
    }
  z_debug (
    "original dir: tostring {}, tolocalfile {}, todisplaystring {}",
    directory.toString (), directory.toLocalFile (),
    directory.toDisplayString ());
  auto tmp =
    fs::path (directory.toLocalFile ().toStdString ()) / name.toStdString ();
  auto dir = io_get_next_available_filepath (tmp);
  auto ret = QString::fromStdString (fs::path (dir).filename ().string ());
  z_debug ("Next available untitled project name for '{}': {}", tmp, ret);
  return ret;
}

void
ProjectManager::add_to_recent_projects (const QString &path)
{
  recent_projects_model_->addRecentProject (path);
}

RecentProjectsModel *
ProjectManager::getRecentProjects () const
{
  z_trace ("Getting recent projects...");
  return recent_projects_model_;
}

void
ProjectManager::createNewProject (
  const QUrl    &directory,
  const QString &name,
  const QUrl    &templateUrl)
{
  auto directory_file = directory.toLocalFile ();
  z_debug ("Creating new project in {}", directory_file);

  QFuture<ProjectLoadResult> future =
    QtConcurrent::run ([directory_file, name, templateUrl, this] () {
      // auto * project = new Project (this);
      Project * project = nullptr;
      (void) this;
      // Simulate long-running operation
      QThread::sleep (3);
      if (project)
        {
          return ProjectLoadResult (project);
        }

      return ProjectLoadResult (QString ("Failed to create project"));
    });
  project_watcher_.setFuture (future);
}

void
ProjectManager::loadProject (const QString &filepath)
{
  z_debug ("Loading project from {}", filepath);

  QFuture<ProjectLoadResult> future = QtConcurrent::run ([filepath, this] () {
    // auto * project = new Project (this);
    Project * project = nullptr;
    (void) this;
    // Simulate long-running operation
    QThread::sleep (3);
    if (project)
      {
        return ProjectLoadResult (project);
      }

    return ProjectLoadResult (QString ("Failed to load project"));
  });
  project_watcher_.setFuture (future);
}

Project *
ProjectManager::getActiveProject () const
{
  return active_project_;
}

void
ProjectManager::setActiveProject (Project * project)
{
  if (active_project_ == project)
    return;

  if (active_project_)
    {
      active_project_->setParent (nullptr);
      active_project_->deleteLater ();
    }

  active_project_ = project;
  if (active_project_)
    {
      active_project_->setParent (this);
    }

  Q_EMIT activeProjectChanged (active_project_);
}