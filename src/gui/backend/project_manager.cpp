#include "common/utils/directory_manager.h"
#include "common/utils/io.h"
#include "gui/backend/zrythm_application.h"
#include "project_manager.h"

using namespace zrythm::gui;

ProjectManager::ProjectManager (QObject * parent)
    : QObject (parent), recent_projects_model_ (new RecentProjectsModel (this))
{
  z_debug ("Initializing project manager...");
  init_templates ();
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