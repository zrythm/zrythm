// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/zrythm.h"
#include "gui/backend/plugin_host_window.h"
#include "gui/backend/project_manager.h"
#include "gui/backend/project_session.h"
#include "gui/backend/zrythm_application.h"
#include "structure/project/project_json_serializer.h"
#include "structure/project/project_loader.h"
#include "structure/project/project_saver.h"
#include "utils/directory_manager.h"
#include "utils/gtest_wrapper.h"
#include "utils/io_utils.h"

#include <QtConcurrentRun>

using namespace std::chrono_literals;
namespace zrythm::gui
{

ProjectManager::ProjectManager (
  utils::AppSettings &app_settings,
  QObject *           parent)
    : QObject (parent), app_settings_ (app_settings),
      recent_projects_model_ (new RecentProjectsModel (app_settings, this))
{
  z_debug ("Initializing project manager...");
  init_templates ();
}

void
ProjectManager::init_templates ()
{
  z_info ("Initializing templates...");

  auto &dir_mgr =
    dynamic_cast<ZrythmApplication *> (qApp)->get_directory_manager ();
  const auto user_templates_dir =
    dir_mgr.get_dir (IDirectoryManager::DirectoryType::USER_TEMPLATES);
  if (fs::is_directory (user_templates_dir))
    {
      try
        {
          auto files = utils::io::get_files_in_dir (user_templates_dir);
          templates_.insert (templates_.end (), files.begin (), files.end ());
        }
      catch (const ZrythmException &e)
        {
          z_warning (
            "Failed to init user templates from {}",
            utils::Utf8String::from_path (user_templates_dir));
        }
    }
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      const auto system_templates_dir =
        dir_mgr.get_dir (IDirectoryManager::DirectoryType::SYSTEM_TEMPLATES);
      if (fs::is_directory (system_templates_dir))
        {
          try
            {
              auto files = utils::io::get_files_in_dir (system_templates_dir);
              templates_.insert (
                templates_.end (), files.begin (), files.end ());
            }
          catch (const ZrythmException &e)
            {
              z_warning (
                "Failed to init system templates from {}",
                utils::Utf8String::from_path (system_templates_dir));
            }
        }
    }

  for (auto &tmpl : this->templates_)
    {
      z_info ("Template found: {}", tmpl);
      if (utils::Utf8String::from_path (tmpl).contains_substr (u8"demo_zsong01"))
        {
          demo_template_ = tmpl;
        }
    }

  z_debug ("done");
}

ProjectManager *
ProjectManager::get_instance ()
{
  if (dynamic_cast<ZrythmApplication *> (qApp) == nullptr)
    return nullptr;

  return dynamic_cast<ZrythmApplication *> (qApp)->projectManager ();
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
    utils::Utf8String::from_qstring (directory.toLocalFile ()).to_path ()
    / utils::Utf8String::from_qstring (name).to_path ();
  auto dir = utils::io::get_next_available_filepath (tmp);
  auto ret = utils::Utf8String::from_path (dir.filename ()).to_qstring ();
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

std::unique_ptr<plugins::IPluginHostWindow>
ProjectManager::create_window_for_plugin (plugins::Plugin &plugin) const
{
  auto track_ref =
    this->active_session_->project ()->tracklist ()->get_track_for_plugin (
      plugin.get_uuid ());
  auto ret = std::make_unique<plugins::JuceDocumentPluginHostWindow> (
    utils::Utf8String::from_utf8_encoded_string (
      fmt::format (
        "{} - {} [{}]",
        track_ref.has_value ()
          ? structure::tracks::from_variant (track_ref->get_object ())->name ()
          : QObject::tr ("<no track>"),
        plugin.get_node_name (),
        plugin.configuration ()->descriptor ()->format ())),
    [&plugin] () {
      z_debug (
        "close button pressed on '{}' plugin window", plugin.get_node_name ());
      plugin.setUiVisible (false);
    });
  return ret;
}

utils::QObjectUniquePtr<ProjectSession>
ProjectManager::create_default (
  const fs::path          &prj_dir,
  const utils::Utf8String &name,
  bool                     with_engine)
{
  z_info ("Creating default project '{}' in {}", name, prj_dir);

  const auto project_dir_path = prj_dir / name;

  utils::QObjectUniquePtr<ProjectSession> project_session;
  QMetaObject::invokeMethod (
    qApp,
    [this, &project_session, &name, project_dir_path] {
      // registries and registry objects must be created on the main thread
      {
        auto * zapp = dynamic_cast<ZrythmApplication *> (qApp);
        auto   prj = utils::make_qobject_unique<structure::project::Project> (
          app_settings_,
          [project_dir_path] (bool for_backup) { return project_dir_path; },
          zapp->hw_audio_interface (),
          zapp->pluginManager ()->get_format_manager (),
          [this] (plugins::Plugin &plugin) {
            return create_window_for_plugin (plugin);
          },
          *zapp->controlRoom ()->metronome (),
          *zapp->controlRoom ()->monitorFader ());
        project_session = utils::make_qobject_unique<ProjectSession> (
          app_settings_, std::move (prj));
      }
      project_session->setTitle (name.to_qstring ());
      project_session->project ()->add_default_tracks ();
    },
    Qt::BlockingQueuedConnection);
  project_session->moveToThread (this->thread ());

  {
    /* create standard dirs */
    structure::project::ProjectSaver::make_project_dirs (project_dir_path);
  }

  project_session->uiState ()->clipEditor ()->init ();

  z_debug ("done creating default project");

  return project_session;
}

void
ProjectManager::createNewProject (
  const QUrl    &directory,
  const QString &name,
  const QUrl    &templateUrl)
{
  const auto directory_file = directory.toLocalFile ();
  const auto template_file = templateUrl.toLocalFile ();
  z_debug (
    "Creating new project in {} (template {})", directory_file, template_file);

  const auto project_dir_path =
    utils::Utf8String::from_qstring (directory_file).to_path ()
    / utils::Utf8String::from_qstring (name).to_path ();

  QtConcurrent::run ([directory_file, name, template_file, this] {
    auto project_session =
      template_file.isEmpty ()
        ? create_default (
            utils::Utf8String::from_qstring (directory_file).to_path (),
            utils::Utf8String::from_qstring (name), true)
        : nullptr; // TODO: template handling
    return project_session;
  })
    .then (
      [this, project_dir_path] (utils::QObjectUniquePtr<ProjectSession> session) {
        structure::project::ProjectSaver saver (
          *session->project (), Zrythm::get_app_version ());
        auto future = saver.save (project_dir_path, false);
        try
          {
            // This will throw on failure
            future.waitForFinished ();
          }
        catch (...)
          {
            // ... so catch the exception here, delete the ProjectSession object
            // in the main thread (which owns it), and re-throw to trigger the
            // onFailed() block.
            QMetaObject::invokeMethod (
              this,
              [session_ptr = session.release ()] () { delete session_ptr; });
            throw;
          }
        session->setTitle (future.result ());
        return session;
      })
    .then (
      this,
      [this, project_dir_path] (
        utils::QObjectUniquePtr<ProjectSession> session_unique_ptr) {
        auto * session = session_unique_ptr.release ();
        session->setParent (this);
        session->setProjectDirectory (
          utils::Utf8String::from_path (project_dir_path).to_qstring ());
        setActiveSession (session);
        session->project ()->engine ()->graph_dispatcher ().recalc_graph (false);
        session->project ()->engine ()->set_running (true);
        Q_EMIT projectLoaded (session);
      })
    .onFailed (this, [this] (const ZrythmException &e) {
      z_warning ("Failed to create project: {}", e.what ());
      Q_EMIT projectLoadingFailed (e.what_string ());
    });
}

void
ProjectManager::loadProject (const QString &filepath)
{
  const auto project_dir = utils::Utf8String::from_qstring (filepath).to_path ();

  z_debug ("Loading project from {}", project_dir);

  // Step 1: Load and validate JSON (background thread - file I/O only)
  QtConcurrent::run ([project_dir] () {
    return structure::project::ProjectLoader::load_from_directory (project_dir);
  })
    .then (
      this,
      [this,
       project_dir] (structure::project::ProjectLoader::LoadResult load_result) {
        // Steps 2-6: All on main thread

        // Create project with all required dependencies
        auto * zapp = dynamic_cast<ZrythmApplication *> (qApp);
        auto   prj = utils::make_qobject_unique<structure::project::Project> (
          app_settings_,
          [project_dir] (bool /*for_backup*/) { return project_dir; },
          zapp->hw_audio_interface (),
          zapp->pluginManager ()->get_format_manager (),
          [this] (plugins::Plugin &plugin) {
            return create_window_for_plugin (plugin);
          },
          *zapp->controlRoom ()->metronome (),
          *zapp->controlRoom ()->monitorFader ());

        auto project_session = utils::make_qobject_unique<ProjectSession> (
          app_settings_, std::move (prj));

        // Deserialize JSON into Project
        structure::project::ProjectJsonSerializer::deserialize (
          load_result.json, *project_session->project ());

        // Set title from loaded project
        project_session->setTitle (load_result.title.to_qstring ());

        // Initialize clip editor
        project_session->uiState ()->clipEditor ()->init ();

        // Set up project
        auto * session = project_session.release ();
        session->setParent (this);
        session->setProjectDirectory (
          utils::Utf8String::from_path (project_dir).to_qstring ());

        // Set as active project
        setActiveSession (session);

        // Rebuild graph and start engine
        session->project ()->engine ()->graph_dispatcher ().recalc_graph (false);
        session->project ()->engine ()->set_running (true);

        // Add to recent projects
        add_to_recent_projects (
          utils::Utf8String::from_path (project_dir).to_qstring ());

        // Emit success signal
        Q_EMIT projectLoaded (session);
      })
    .onFailed (this, [this] (const ZrythmException &e) {
      z_warning ("Failed to load project: {}", e.what ());
      Q_EMIT projectLoadingFailed (e.what_string ());
    });
}

ProjectSession *
ProjectManager::activeSession () const
{
  return active_session_.get ();
}

void
ProjectManager::setActiveSession (ProjectSession * project)
{
  if (active_session_.get () == project)
    return;

  if (active_session_)
    {
      active_session_->project ()->aboutToBeDeleted ();
      active_session_.reset ();
    }

  active_session_ = project;
  if (active_session_)
    {
      active_session_->setParent (this);
    }

  Q_EMIT activeSessionChanged (active_session_.get ());
}
}
