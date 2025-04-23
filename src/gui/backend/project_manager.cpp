// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/zrythm.h"
#include "gui/backend/zrythm_application.h"
#include "gui/dsp/router.h"
#include "project_manager.h"
#include "utils/directory_manager.h"
#include "utils/gtest_wrapper.h"
#include "utils/io.h"

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
          setActiveProject (project);

          /* recalculate the routing graph & kick off engine processing */
          project->audio_engine_->router_->recalc_graph (false);
          project->audio_engine_->run_.store (true);

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

  auto &dir_mgr =
    dynamic_cast<ZrythmApplication *> (qApp)->get_directory_manager ();
  const auto user_templates_dir =
    dir_mgr.get_dir (IDirectoryManager::DirectoryType::USER_TEMPLATES);
  if (fs::is_directory (user_templates_dir))
    {
      try
        {
          auto files =
            utils::io::get_files_in_dir (user_templates_dir.string ());
          std::ranges::transform (
            files, std::back_inserter (templates_),
            [] (const auto &p) { return fs::path (p.toStdString ()); });
        }
      catch (const ZrythmException &e)
        {
          z_warning (
            "Failed to init user templates from {}",
            user_templates_dir.string ());
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
              auto files =
                utils::io::get_files_in_dir (system_templates_dir.string ());
              std::ranges::transform (
                files, std::back_inserter (templates_),
                [] (const auto &p) { return fs::path (p.toStdString ()); });
            }
          catch (const ZrythmException &e)
            {
              z_warning (
                "Failed to init system templates from {}",
                system_templates_dir.string ());
            }
        }
    }

  for (auto &tmpl : this->templates_)
    {
      z_info ("Template found: {}", tmpl);
      if (juce::String (tmpl.string ()).contains ("demo_zsong01"))
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
  auto dir = utils::io::get_next_available_filepath (tmp.string ());
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

Project *
ProjectManager::create_default (
  const fs::path    &prj_dir,
  const std::string &name,
  bool               with_engine)
{
  z_info ("Creating default project '{}' in {}", name, prj_dir);

  auto * prj = new Project ();
  prj->setTitle (QString::fromStdString (name));
  prj->add_default_tracks ();

  /* pre-setup engine */
  auto * engine = prj->audio_engine_.get ();
  z_return_val_if_fail (engine, nullptr);
  if (with_engine)
    {
      engine->pre_setup ();
    }

  engine->setup ();

  if (with_engine)
    {
      prj->tracklist_->get_track_span ().expose_ports_to_backend (*engine);
    }

  auto beats_per_bar = prj->tracklist_->getTempoTrack ()->get_beats_per_bar ();
  engine->update_frames_per_tick (
    beats_per_bar, prj->tracklist_->getTempoTrack ()->get_current_bpm (),
    engine->sample_rate_, true, true, false);

  /* set directory/title and create standard dirs */
  prj->dir_ = prj_dir / name;
  prj->make_project_dirs (false);

  prj->loaded_ = true;

  prj->clip_editor_->init ();

  prj->quantize_opts_timeline_->update_quantize_points (*prj->transport_);
  prj->quantize_opts_editor_->update_quantize_points (*prj->transport_);

  z_debug ("done creating default project");

  return prj;
}

void
ProjectManager::createNewProject (
  const QUrl    &directory,
  const QString &name,
  const QUrl    &templateUrl)
{
  auto directory_file = directory.toLocalFile ();
  auto template_file = templateUrl.toLocalFile ();
  z_debug (
    "Creating new project in {} (template {})", directory_file, template_file);

  QFuture<ProjectLoadResult> future =
    QtConcurrent::run ([directory_file, name, template_file, this] () {
      try
        {
          // const auto prj_dir =
          // fs::path (directory_file.toStdString ()) / name.toStdString ();
          Project * project = nullptr;
          if (template_file.isEmpty ())
            {
              project = create_default (
                directory_file.toStdString (), name.toStdString (), true);
            }
          else
            {
              // TODO create from template
            }

          // if we don't move the project to the main thread it will be
          // associated with this thread pool thread and we won't be able to
          // reparent the project instance to the main thread later
          project->moveToThread (this->thread ());

          // save the newly created project
          project->save (project->dir_, false, false, false);

          // setting the parent must be done on the same thread as the parent
          QMetaObject::invokeMethod (
            this, [this, project] () { project->setParent (this); },
            Qt::BlockingQueuedConnection);

          return ProjectLoadResult (project);
        }
      catch (const ZrythmException &e)
        {
          z_warning ("Failed to create project: {}", e.what ());
          return ProjectLoadResult (QString::fromUtf8 (e.what ()));
        }
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

    return ProjectLoadResult (tr ("Failed to load project"));
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
      active_project_->aboutToBeDeleted ();
      active_project_->deleteLater ();
    }

  active_project_ = project;
  if (active_project_)
    {
      active_project_->setParent (this);
    }

  Q_EMIT activeProjectChanged (active_project_);
}
