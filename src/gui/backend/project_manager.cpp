// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"
#include <fmt/std.h>

#include "controllers/project_json_serializer.h"
#include "controllers/project_loader.h"
#include "controllers/project_saver.h"
#include "gui/backend/project_manager.h"
#include "gui/backend/project_session.h"
#include "gui/backend/qt_plugin_host_window.h"
#include "gui/backend/x11_plugin_host_window.h"
#include "gui/backend/zrythm_application.h"
#include "structure/tracks/track.h"
#include "structure/tracks/tracklist.h"
#include "utils/directory_manager.h"
#include "utils/io_utils.h"
#include "utils/version.h"

#include <QCoreApplication>
#include <QGuiApplication>
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
  if (std::filesystem::is_directory (user_templates_dir))
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

  {
    const auto system_templates_dir =
      dir_mgr.get_dir (IDirectoryManager::DirectoryType::SYSTEM_TEMPLATES);
    if (std::filesystem::is_directory (system_templates_dir))
      {
        try
          {
            auto files = utils::io::get_files_in_dir (system_templates_dir);
            templates_.insert (templates_.end (), files.begin (), files.end ());
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

RecentProjectsModel *
ProjectManager::getRecentProjects () const
{
  z_trace ("Getting recent projects...");
  return recent_projects_model_;
}

std::unique_ptr<plugins::PluginHostWindow>
ProjectManager::create_window_for_plugin (plugins::Plugin &plugin) const
{
  // While a project is being loaded, the active session is null or still
  // points at the previous project, but a plugin editor restore queued
  // during deserialization can already arrive here; the track lookup then
  // misses and the window title falls back to "<no track>"
  const auto track_ref =
    active_session_ != nullptr
      ? active_session_->project ()->tracklist ()->get_track_for_plugin (
          plugin.get_uuid ())
      : std::optional<structure::tracks::TrackUuidReference>{};
  const auto title = utils::Utf8String::from_utf8_encoded_string (
    fmt::format (
      "{} - {} [{}]",
      track_ref.has_value ()
        ? track_ref->get ()->name ()
        : QObject::tr ("<no track>"),
      plugin.get_node_name (),
      plugin.configuration ()->descriptor ()->format ()));
  // Plugin editors need X11 windows. Under Wayland sessions Qt would create
  // Wayland surfaces, so use a raw X11 window (via XWayland); everywhere else
  // Qt's platform matches the plugin windowing system (xcb, Win32, Cocoa)
  std::unique_ptr<plugins::PluginHostWindow> ret;
  if (QGuiApplication::platformName ().startsWith (u"wayland"))
    {
      auto x11_window = std::make_unique<X11PluginHostWindow> (plugin);
      if (!x11_window->is_valid ())
        {
          // No usable X connection - the plugin falls back to the generic UI
          z_warning ("X11 host window unavailable (no X server?)");
          return nullptr;
        }
      ret = std::move (x11_window);
    }
  else
    {
      ret = std::make_unique<QtPluginHostWindow> (plugin);
    }
  ret->setTitle (title.to_qstring ());
  return ret;
}

utils::QObjectUniquePtr<ProjectSession>
ProjectManager::create_default (
  const std::filesystem::path &prj_dir,
  const utils::Utf8String     &name,
  bool                         with_engine)
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
          zapp->hw_audio_interface (), *zapp->deviceManager (),
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
      project_session->uiState ()->clipEditor ()->init ();
      project_session->uiState ()->chordPadBank ()->applyPresetFromScale (
        dsp::MusicalScale::ScaleType::Aeolian, dsp::MusicalNote::A);
    },
    Qt::BlockingQueuedConnection);
  project_session->moveToThread (this->thread ());

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
    .then ([this,
            project_dir_path] (utils::QObjectUniquePtr<ProjectSession> session) {
      auto future = controllers::ProjectSaver::save (
        *session->project (), *session->uiState (), *session->undoStack (),
        utils::get_app_version (), project_dir_path, false);
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
          QMetaObject::invokeMethod (this, [session_ptr = session.release ()] () {
            delete session_ptr;
          });
          throw;
        }
      const auto saved_path =
        utils::Utf8String::from_qstring (future.result ()).to_path ();
      session->setTitle (
        utils::Utf8String::from_path (utils::io::path_get_basename (saved_path))
          .to_qstring ());
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

gui::qquick::QFutureQmlWrapper *
ProjectManager::loadProject (const QString &filepath)
{
  if (load_future_.isRunning ())
    {
      z_warning ("A project load is already in progress, ignoring request");
      return nullptr;
    }

  const auto project_dir = utils::Utf8String::from_qstring (filepath).to_path ();

  z_debug ("Loading project from {}", project_dir);

  // Progress values (0-100) and the step each one is shown during:
  // 5-40: Verifying the directory, reading and parsing the project file
  // 40-50: Creating the project object
  // 50-70: Deserializing data
  // 70-85: Setting up the project
  // 85-100: Rebuilding the graph and starting the engine
  constexpr int kStageVerifyDir = 5;
  constexpr int kStageReadFile = 15;
  constexpr int kStageParseJson = 25;
  constexpr int kStageCreateProject = 40;
  constexpr int kStageDeserialize = 50;
  constexpr int kStageSetup = 70;
  constexpr int kStageRebuildGraph = 85;
  constexpr int kStageStartEngine = 95;
  constexpr int kStageDone = 100;

  auto future = QtConcurrent::run ([this, project_dir] (QPromise<QString> &promise) {
    promise.setProgressRange (0, kStageDone);

    nlohmann::json    json;
    utils::Utf8String title;

    try
      {
        // Run the file-loading steps inline instead of via a nested
        // QFuture: progress is reported directly, and no pool thread is
        // held while waiting on a second task
        promise.setProgressValueAndText (
          kStageVerifyDir, tr ("Verifying project directory..."));
        if (promise.isCanceled ())
          return;
        if (!std::filesystem::is_directory (project_dir))
          {
            throw ZrythmException (
              fmt::format ("Project directory does not exist: {}", project_dir));
          }

        promise.setProgressValueAndText (
          kStageReadFile, tr ("Reading project file..."));
        if (promise.isCanceled ())
          return;
        const auto json_str =
          controllers::ProjectLoader::get_uncompressed_project_text (project_dir);

        promise.setProgressValueAndText (
          kStageParseJson, tr ("Parsing project data..."));
        if (promise.isCanceled ())
          return;
        json = controllers::ProjectLoader::parse_and_validate (json_str);
        title = controllers::ProjectLoader::extract_title (json);
      }
    catch (const ZrythmException &e)
      {
        z_warning ("Failed to load project: {}", e.what ());
        Q_EMIT projectLoadingFailed (e.what_string ());
        promise.setException (std::current_exception ());
        return;
      }
    catch (const std::exception &e)
      {
        z_warning ("Failed to load project: {}", e.what ());
        Q_EMIT projectLoadingFailed (
          tr ("Failed to load project data. See the log for details."));
        promise.setException (std::current_exception ());
        return;
      }

    // Run main thread work via invokeMethod
    QMetaObject::invokeMethod (
      this,
      [this, &json, &title, &project_dir, &promise] () {
        try
          {
            const auto report_progress =
              [&promise] (int value, const QString &text) {
                promise.setProgressValueAndText (value, text);
              };
            // The main event loop is blocked while this lambda runs, so
            // progress updates would only reach the UI after everything
            // finishes. Pump events after the early updates so the dialog
            // repaints before the next step begins (user input is excluded,
            // so the dialog's Cancel button stays inert). Updates from
            // deserialization onwards must not pump: queued plugin editor
            // restores must only run once the new session is active
            const auto report_progress_and_repaint =
              [&report_progress] (int value, const QString &text) {
                report_progress (value, text);
                QCoreApplication::processEvents (
                  QEventLoop::ExcludeUserInputEvents);
              };

            report_progress_and_repaint (
              kStageCreateProject, tr ("Creating project..."));
            if (promise.isCanceled ())
              return;

            // Create project with all required dependencies
            auto * zapp = dynamic_cast<ZrythmApplication *> (qApp);
            auto prj = utils::make_qobject_unique<structure::project::Project> (
              app_settings_,
              [project_dir] (bool /*for_backup*/) { return project_dir; },
              zapp->hw_audio_interface (), *zapp->deviceManager (),
              zapp->pluginManager ()->get_format_manager (),
              [this] (plugins::Plugin &plugin) {
                return create_window_for_plugin (plugin);
              },
              *zapp->controlRoom ()->metronome (),
              *zapp->controlRoom ()->monitorFader ());

            report_progress_and_repaint (
              kStageDeserialize, tr ("Deserializing project data..."));
            if (promise.isCanceled ())
              return;

            auto project_session = utils::make_qobject_unique<ProjectSession> (
              app_settings_, std::move (prj));

            // Deserialize JSON into Project, ProjectUiState, and UndoStack
            controllers::ProjectLoader::deserialize (
              json, *project_session->project (), *project_session->uiState (),
              *project_session->undoStack ());

            report_progress (kStageSetup, tr ("Setting up project..."));
            if (promise.isCanceled ())
              return;

            // Set title from loaded project
            project_session->setTitle (title.to_qstring ());

            // Initialize clip editor
            project_session->uiState ()->clipEditor ()->init ();

            // Set up project
            auto * session = project_session.release ();
            session->setParent (this);
            session->setProjectDirectory (
              utils::Utf8String::from_path (project_dir).to_qstring ());

            // Re-attach generic UI tracking for all deserialized plugins in
            // the project's tracks (restores generic windows for plugins
            // saved with visible UI)
            for (
              const auto &tr_ref :
              session->project ()->tracklist ()->collection ()->tracks ())
              {
                std::vector<plugins::PluginUuidReference> plugins;
                tr_ref.get ()->collect_plugins (plugins);
                for (const auto &pl_ref : plugins)
                  {
                    session->genericPluginUiController ()
                      ->trackPluginUiVisibility (pl_ref.get ());
                  }
              }

            report_progress (
              kStageRebuildGraph, tr ("Rebuilding audio graph..."));

            // Rebuild graph and start engine
            session->project ()->engine ()->graph_dispatcher ().recalc_graph (
              false);

            report_progress (kStageStartEngine, tr ("Starting engine..."));

            session->project ()->engine ()->set_running (true);

            // Add to recent projects
            recent_projects_model_->addRecentProject (
              utils::Utf8String::from_path (project_dir).to_qstring ());

            // Expose the session as active only once it is fully set up,
            // so no event delivery can observe a half-initialized session
            setActiveSession (session);

            // Emit success signal
            Q_EMIT projectLoaded (session);

            report_progress (kStageDone, tr ("Project loaded"));
            promise.addResult (
              utils::Utf8String::from_path (project_dir).to_qstring ());
          }
        catch (const ZrythmException &e)
          {
            z_warning ("Failed to load project: {}", e.what ());
            Q_EMIT projectLoadingFailed (e.what_string ());
            promise.setException (std::current_exception ());
          }
        catch (const std::exception &e)
          {
            z_warning ("Failed to load project: {}", e.what ());
            Q_EMIT projectLoadingFailed (
              tr ("Failed to load project data. See the log for details."));
            promise.setException (std::current_exception ());
          }
      },
      Qt::BlockingQueuedConnection);
  });

  load_future_ = future;

  future.onCanceled (this, [] () {
    z_debug ("Project load cancelled");
    return QString{};
  });

  auto * wrapper = new gui::qquick::QFutureQmlWrapperT<QString> (future);
  QQmlEngine::setObjectOwnership (wrapper, QQmlEngine::JavaScriptOwnership);

  return wrapper;
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
      if (
        active_session_->project () != nullptr
        && active_session_->project ()->engine () != nullptr)
        {
          active_session_->project ()->engine ()->deactivate ();
        }
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
