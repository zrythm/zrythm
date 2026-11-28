// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cassert>
#include <exception>
#include <filesystem>
#include <functional>
#include <tuple>
#include <vector>

#include "utils/format_qt.h"
#include <fmt/std.h>

#include "controllers/project_json_serializer.h"
#include "controllers/project_loader.h"
#include "controllers/project_saver.h"
#include "frontends/gui/backend/app_scope.h"
#include "frontends/gui/backend/project_manager.h"
#include "frontends/gui/backend/project_session.h"
#include "frontends/gui/backend/qt_plugin_host_window.h"
#include "frontends/gui/backend/x11_plugin_host_window.h"
#include "frontends/gui/backend/zrythm_application.h"
#include "plugins/plugin.h"
#include "structure/project/project_directory.h"
#include "structure/tracks/track.h"
#include "structure/tracks/tracklist.h"
#include "utils/directory_manager.h"
#include "utils/io_utils.h"
#include "utils/version.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QThread>
#include <QtConcurrentRun>

using namespace std::chrono_literals;
namespace zrythm::frontends::gui
{

ProjectManager::ProjectManager (
  utils::AppSettings &app_settings,
  QObject *           parent)
    : QObject (parent), app_settings_ (app_settings),
      recent_projects_model_ (new RecentProjectsModel (app_settings, this)),
      clipboard_ (utils::make_qobject_unique<controllers::Clipboard> (this))
{
  z_debug ("Initializing project manager...");
  init_templates ();
}

namespace
{

/**
 * @brief Returns the directories directly inside @p dir that hold a
 * project file, in path order.
 *
 * A read error reports a warning; the entries found before it are
 * returned.
 */
std::vector<std::filesystem::path>
project_directories_in (const std::filesystem::path &dir)
{
  std::vector<std::filesystem::path> ret;
  try
    {
      for (const auto &entry : std::filesystem::directory_iterator{ dir })
        {
          if (
            structure::project::classify_directory (entry.path ())
            == structure::project::DirectoryContent::ContainsProjectFile)
            {
              z_info ("Template found: {}", entry.path ());
              ret.push_back (entry.path ());
            }
        }
    }
  catch (const std::filesystem::filesystem_error &e)
    {
      z_warning ("Failed to read the directory {}: {}", dir, e.what ());
    }
  std::ranges::sort (ret);
  return ret;
}

} // namespace

void
ProjectManager::init_templates ()
{
  z_info ("Initializing templates...");

  auto &dir_mgr =
    dynamic_cast<ZrythmApplication *> (qApp)->get_directory_manager ();
  for (
    const auto dir_type :
    { IDirectoryManager::DirectoryType::USER_TEMPLATES,
      IDirectoryManager::DirectoryType::SYSTEM_TEMPLATES })
    {
      const auto templates_dir = dir_mgr.get_dir (dir_type);
      if (!std::filesystem::is_directory (templates_dir))
        {
          continue;
        }
      const auto project_dirs = project_directories_in (templates_dir);
      templates_.insert (
        templates_.end (), project_dirs.begin (), project_dirs.end ());
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

bool
ProjectManager::isValidProjectName (const QString &name)
{
  return utils::io::is_valid_file_name (
    utils::Utf8String::from_qstring (name).view ());
}

ProjectManager::SaveTargetState
ProjectManager::classifySaveTarget (
  const QUrl    &parent_directory,
  const QString &name)
{
  if (!isValidProjectName (name))
    return InvalidName;

  const auto parent_path =
    utils::Utf8String::from_qurl (parent_directory).to_path ();
  if (parent_path.empty () || parent_path.is_relative ())
    return Unknown;

  using structure::project::DirectoryContent;
  switch (structure::project::classify_save_target (
    parent_path, utils::Utf8String::from_qstring (name)))
    {
    case DirectoryContent::NonExistent:
      return NonExistent;
    case DirectoryContent::Empty:
      return Empty;
    case DirectoryContent::ContainsProjectFile:
      return ContainsProjectFile;
    case DirectoryContent::OtherContent:
      return OtherContent;
    case DirectoryContent::Unknown:
      break;
    }
  return Unknown;
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
  // While a project is being loaded, no session is active, but a plugin
  // editor restore queued during deserialization can already arrive here;
  // the track lookup then misses and the window title falls back to
  // "<no track>"
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

namespace
{

/**
 * @brief Throws a ZrythmException if no audio device is available.
 *
 * Opening a project requires audio hardware (the project's tempo map
 * and processing graph are built from the device's sample rate and
 * buffer size). The dummy engine counts as available hardware.
 */
void
throw_if_no_audio_device ()
{
  auto * zapp = dynamic_cast<ZrythmApplication *> (qApp);
  if (zapp != nullptr)
    {
      try
        {
          // Throws when no device is open
          std::ignore = zapp->hw_audio_interface ().get_device_info ();
          return;
        }
      catch (const ZrythmException &)
        {
          // Reported below with a message for the user
        }
    }
  throw ZrythmException (
    QObject::tr (
      "No audio device is open. Select an audio device in the device "
      "settings and try again."));
}

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

/**
 * @brief Runs @p func on the main thread and waits for it to finish.
 *
 * Must be called from another thread than the main thread.
 *
 * @throw ZrythmException when @p func could not be run on the main
 * thread.
 * @throw Any exception thrown by @p func, rethrown on the calling
 * thread.
 */
void
run_on_main_thread_blocking (const std::function<void ()> &func)
{
  assert (QThread::currentThread () != qApp->thread ());
  std::exception_ptr main_thread_error;
  const auto         invoked = QMetaObject::invokeMethod (
    qApp,
    [&func, &main_thread_error] {
      // Exceptions cannot propagate through a queued invocation: capture
      // them here and rethrow on the calling thread below
      try
        {
          func ();
        }
      catch (...)
        {
          main_thread_error = std::current_exception ();
        }
    },
    Qt::BlockingQueuedConnection);
  if (!invoked)
    {
      throw ZrythmException (
        "A step of the project creation could not be run on the main "
        "thread");
    }
  if (main_thread_error)
    {
      std::rethrow_exception (main_thread_error);
    }
}

/**
 * @brief Deserializes @p json into @p session and gives it the title
 * @p title.
 *
 * @param source_project_dir Directory the JSON was read from, when it
 * is not the directory @p session lives in (see
 * ProjectLoader::deserialize()).
 *
 * Main thread only.
 */
void
deserialize_session (
  ProjectSession              &session,
  const nlohmann::json        &json,
  const utils::Utf8String     &title,
  const std::filesystem::path &source_project_dir = {})
{
  controllers::ProjectLoader::deserialize (
    json, *session.project (), *session.uiState (), *session.undoStack (),
    source_project_dir);
  session.setTitle (title.to_qstring ());
  session.uiState ()->clipEditor ()->init ();
}

/**
 * @brief Attaches the generic UI tracking to the plugins in the tracks
 * and the modulators of @p session.
 *
 * Restores generic windows for plugins saved with a visible UI.
 *
 * Main thread only.
 */
void
register_plugin_uis (ProjectSession &session)
{
  for (
    const auto &tr_ref :
    session.project ()->tracklist ()->collection ()->tracks ())
    {
      std::vector<plugins::PluginUuidReference> plugins;
      tr_ref.get ()->collect_plugins (plugins);
      for (const auto &pl_ref : plugins)
        {
          session.genericPluginUiController ()->trackPluginUiVisibility (
            pl_ref.get ());
        }
    }
  for (const auto &pl_ref : session.project ()->modulators ()->get_plugins ())
    {
      session.genericPluginUiController ()->trackPluginUiVisibility (
        pl_ref.get ());
    }
}

} // namespace

utils::QObjectUniquePtr<ProjectSession>
ProjectManager::make_session (const std::filesystem::path &project_dir)
{
  throw_if_no_audio_device ();
  auto * zapp = dynamic_cast<ZrythmApplication *> (qApp);
  auto   prj = utils::make_qobject_unique<structure::project::Project> (
    app_settings_, [project_dir] () { return project_dir; },
    zapp->hw_audio_interface (), *zapp->deviceManager (),
    zapp->pluginManager ()->get_format_manager (),
    [this] (plugins::Plugin &plugin) {
      return create_window_for_plugin (plugin);
    },
    *zapp->controlRoom ()->metronome (), *zapp->controlRoom ()->monitorFader (),
    zapp->pluginManager ()->get_lv2_world ());
  return utils::make_qobject_unique<ProjectSession> (
    app_settings_, *clipboard_, zapp->appScope ().registry (), std::move (prj));
}

bool
ProjectManager::adopt_session (
  utils::QObjectUniquePtr<ProjectSession> session,
  const std::filesystem::path            &project_dir,
  const StageReporter                    &report_stage)
{
  const auto report = [&report_stage] (int value, const QString &text) {
    if (report_stage)
      {
        report_stage (value, text);
      }
  };

  if (active_session_ != nullptr)
    {
      z_warning (
        "A project became active while another one was being prepared; "
        "discarding the prepared project");
      Q_EMIT projectLoadingFailed (tr (
        "A project is already open. Close it before opening or creating "
        "another one."));
      return false;
    }

  auto * session_ptr = session.get ();
  session_ptr->setProjectDirectory (
    utils::Utf8String::from_path (project_dir).to_qstring ());

  register_plugin_uis (*session_ptr);

  report (kStageRebuildGraph, tr ("Rebuilding audio graph..."));
  session_ptr->project ()->engine ()->graph_dispatcher ().recalc_graph (false);

  report (kStageStartEngine, tr ("Starting engine..."));
  session_ptr->project ()->engine ()->set_running (true);

  // Add to recent projects
  recent_projects_model_->addRecentProject (
    utils::Utf8String::from_path (project_dir).to_qstring ());

  // Expose the session as active only once it is fully set up,
  // so no event delivery can observe a half-initialized session. The
  // session stays owned by this function until then and is destroyed
  // when a step throws.
  setActiveSession (session.release ());

  Q_EMIT projectLoaded (session_ptr);

  report_plugins_failed_to_load (*session_ptr->project ());

  return true;
}

utils::QObjectUniquePtr<ProjectSession>
ProjectManager::create_default (
  const std::filesystem::path &project_dir,
  const utils::Utf8String     &name)
{
  z_info ("Creating default project '{}' in {}", name, project_dir);

  utils::QObjectUniquePtr<ProjectSession> project_session;
  run_on_main_thread_blocking ([&] {
    // registries and registry objects must be created on the main
    // thread
    auto session = make_session (project_dir);
    session->setTitle (name.to_qstring ());
    session->project ()->add_default_tracks ();
    session->uiState ()->clipEditor ()->init ();
    session->uiState ()->chordPadBank ()->applyPresetFromScale (
      dsp::MusicalScale::ScaleType::Aeolian, dsp::MusicalNote::A);
    project_session = std::move (session);
  });

  z_debug ("done creating default project");

  return project_session;
}

bool
ProjectManager::createNewProject (
  const QUrl    &directory,
  const QString &name,
  const QString &templateDirectory)
{
  if (active_session_ != nullptr)
    {
      z_warning ("A project is open; refusing to create a new project");
      Q_EMIT projectLoadingFailed (
        tr ("A project is already open. Close it before creating a new one."));
      return false;
    }
  if (is_project_operation_in_progress ())
    {
      z_warning (
        "A project load or creation is in progress; refusing to create a "
        "new project");
      Q_EMIT projectLoadingFailed (tr (
        "Another project is already being opened or created. Wait for it "
        "to finish."));
      return false;
    }

  const auto directory_file = directory.toLocalFile ();
  switch (classifySaveTarget (directory, name))
    {
    case NonExistent:
    case Empty:
      break;
    case InvalidName:
      z_warning ("project name '{}' cannot be used as a folder name", name);
      Q_EMIT projectLoadingFailed (
        tr (
          "'%1' cannot be used as a project name. Use a name that works as "
          "a folder name on all systems.")
          .arg (name));
      return false;
    case ContainsProjectFile:
      z_warning ("a project already exists at '{}/{}'", directory_file, name);
      Q_EMIT projectLoadingFailed (
        tr (
          "A project with the name '%1' already exists in this location. "
          "Choose another name or location.")
          .arg (name));
      return false;
    case OtherContent:
      z_warning (
        "folder '{}/{}' already exists and holds other files", directory_file,
        name);
      Q_EMIT projectLoadingFailed (
        tr (
          "The folder for '%1' already exists and holds other files. "
          "Choose another name or location.")
          .arg (name));
      return false;
    case Unknown:
      z_warning (
        "could not check the target folder '{}/{}'", directory_file, name);
      Q_EMIT projectLoadingFailed (
        tr ("Zrythm could not check the folder for '%1'.").arg (name));
      return false;
    }

  const auto template_dir =
    utils::Utf8String::from_qstring (templateDirectory).to_path ();
  if (!template_dir.empty () && template_dir.is_relative ())
    {
      z_warning ("template directory '{}' is a relative path", template_dir);
      Q_EMIT projectLoadingFailed (
        tr ("The template at '%1' cannot be used: give an absolute path.")
          .arg (templateDirectory));
      return false;
    }
  if (!template_dir.empty ())
    {
      using structure::project::DirectoryContent;
      switch (structure::project::classify_directory (template_dir))
        {
        case DirectoryContent::ContainsProjectFile:
          break;
        case DirectoryContent::NonExistent:
          z_warning ("template directory '{}' does not exist", template_dir);
          Q_EMIT projectLoadingFailed (
            tr (
              "The template at '%1' cannot be used: the directory does not "
              "exist.")
              .arg (templateDirectory));
          return false;
        case DirectoryContent::Empty:
          z_warning ("template directory '{}' is empty", template_dir);
          Q_EMIT projectLoadingFailed (
            tr ("The template at '%1' cannot be used: the directory is empty.")
              .arg (templateDirectory));
          return false;
        case DirectoryContent::OtherContent:
          z_warning ("'{}' is not a project directory", template_dir);
          Q_EMIT projectLoadingFailed (
            tr (
              "The template at '%1' cannot be used: the path is not a project "
              "directory.")
              .arg (templateDirectory));
          return false;
        case DirectoryContent::Unknown:
          z_warning ("template directory '{}' cannot be read", template_dir);
          Q_EMIT projectLoadingFailed (
            tr (
              "The template at '%1' cannot be used: the directory cannot be "
              "read.")
              .arg (templateDirectory));
          return false;
        }
    }

  z_debug (
    "Creating new project in {} (template {})", directory_file, template_dir);

  const auto parent_dir_path =
    utils::Utf8String::from_qstring (directory_file).to_path ();
  const auto project_dir_path =
    parent_dir_path / utils::Utf8String::from_qstring (name).to_path ();

  creation_future_ =
    QtConcurrent::run (
      [name, template_dir, project_dir_path,
       this] () -> utils::QObjectUniquePtr<ProjectSession> {
        if (template_dir.empty ())
          {
            return create_default (
              project_dir_path, utils::Utf8String::from_qstring (name));
          }

        // The new project is built from the template's project file
        const auto json = controllers::ProjectLoader::parse_and_validate (
          controllers::ProjectLoader::get_uncompressed_project_text (
            template_dir));

        utils::QObjectUniquePtr<ProjectSession> project_session;
        run_on_main_thread_blocking ([&] {
          auto session = make_session (project_dir_path);
          deserialize_session (
            *session, json, utils::Utf8String::from_qstring (name),
            template_dir);
          // The new project gets an identity of its own
          session->project ()->regenerate_project_id ();
          project_session = std::move (session);
        });
        return project_session;
      })
      .then ([this, project_dir_path] (
               utils::QObjectUniquePtr<ProjectSession> session) {
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
            // ... so catch the exception here, delete the ProjectSession
            // object in the main thread (which owns it), and re-throw to
            // trigger the onFailed() block.
            QMetaObject::invokeMethod (
              this,
              [session_ptr = session.release ()] () { delete session_ptr; });
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
          adopt_session (std::move (session_unique_ptr), project_dir_path, {});
        })
      .onFailed (
        this,
        [this] (const ZrythmException &e) {
          z_warning ("Failed to create project: {}", e.what ());
          Q_EMIT projectLoadingFailed (e.what_string ());
        })
      .onFailed (this, [this] (const std::exception &e) {
        z_warning ("Failed to create project: {}", e.what ());
        Q_EMIT projectLoadingFailed (
          tr ("Failed to create the project. See the log for details."));
      });

  return true;
}

frontends::gui::qquick::QFutureQmlWrapper *
ProjectManager::loadProject (const QString &filepath)
{
  if (active_session_ != nullptr)
    {
      z_warning ("A project is open; ignoring request to load another one");
      return nullptr;
    }
  if (is_project_operation_in_progress ())
    {
      z_warning (
        "A project load or creation is already in progress, ignoring request");
      return nullptr;
    }

  const auto project_dir = utils::Utf8String::from_qstring (filepath).to_path ();

  z_debug ("Loading project from {}", project_dir);

  auto future =
    QtConcurrent::run ([this, project_dir] (QPromise<QString> &promise) {
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
                fmt::format (
                  "Project directory does not exist: {}", project_dir));
            }

          promise.setProgressValueAndText (
            kStageReadFile, tr ("Reading project file..."));
          if (promise.isCanceled ())
            return;
          const auto json_str = controllers::ProjectLoader::
            get_uncompressed_project_text (project_dir);

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

              auto project_session = make_session (project_dir);

              report_progress_and_repaint (
                kStageDeserialize, tr ("Deserializing project data..."));
              if (promise.isCanceled ())
                return;

              deserialize_session (*project_session, json, title);

              report_progress (kStageSetup, tr ("Setting up project..."));
              if (promise.isCanceled ())
                return;

              if (!adopt_session (
                    std::move (project_session), project_dir, report_progress))
                {
                  promise.addResult (QString{});
                  return;
                }

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

  auto * wrapper =
    new frontends::gui::qquick::QFutureQmlWrapperT<QString> (future);
  QQmlEngine::setObjectOwnership (wrapper, QQmlEngine::JavaScriptOwnership);

  return wrapper;
}

void
ProjectManager::report_plugins_failed_to_load (
  structure::project::Project &project)
{
  QStringList failures;
  project.get_registry ().for_each_matching<plugins::Plugin> (
    [&failures] (const plugins::Plugin &plugin) {
      if (
        plugin.instantiationStatus ()
        != plugins::Plugin::InstantiationStatus::Failed)
        return;
      const auto error =
        plugin.instantiationError ().isEmpty ()
          ? tr ("unknown error")
          : plugin.instantiationError ();
      failures.append (
        u"%1: %2"_s.arg (plugin.get_name ().to_qstring ()).arg (error));
    });
  if (!failures.isEmpty ())
    {
      Q_EMIT pluginsFailedToLoad (
        tr ("%n plugin(s) failed to load", nullptr, failures.size ()),
        failures.join (u'\n'));
    }
}

ProjectSession *
ProjectManager::activeSession () const
{
  return active_session_.get ();
}

controllers::Clipboard *
ProjectManager::clipboard () const
{
  return clipboard_.get ();
}

void
ProjectManager::setActiveSession (ProjectSession * project)
{
  if (active_session_.get () == project)
    return;

  assert (project == nullptr || active_session_ == nullptr);

  active_session_.reset ();
  active_session_ = project;
  if (active_session_)
    {
      active_session_->setParent (this);
    }

  Q_EMIT activeSessionChanged (active_session_.get ());
}
}
