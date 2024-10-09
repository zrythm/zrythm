// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/directory_manager.h"
#include "common/utils/pcg_rand.h"
#include "gui/backend/backend/settings_manager.h"

#include <QFontDatabase>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>

#include "engine/ipc_message.h"
#include "zrythm_application.h"

using namespace zrythm::gui;

JUCE_CREATE_APPLICATION_DEFINE (ZrythmJuceApplicationWrapper)

ZrythmApplication::ZrythmApplication (int &argc, char ** argv)
    : QApplication (argc, argv), qt_thread_id_ (current_thread_id.get ())
{
  Backtrace::init_signal_handlers ();

  /* app info */
  setApplicationName ("Zrythm");
  setApplicationVersion (PACKAGE_VERSION);
  setOrganizationName ("Zrythm.org");
  setOrganizationDomain ("zrythm.org");
  setApplicationDisplayName ("Zrythm");
  // setWindowIcon (QIcon (":/org.zrythm.Zrythm/resources/icons/zrythm.svg"));

  /* setup command line parser */
  setup_command_line_options ();
  cmd_line_parser_.process (*this);

  // Initialize JUCE
  juce ::JUCEApplicationBase ::createInstance = &juce_CreateApplication;
  juce::MessageManager::getInstance ()->setCurrentThreadAsMessageThread ();

  settings_manager_ = new SettingsManager (this);
  theme_manager_ = new ThemeManager (this);

  launch_engine_process ();

  Zrythm::getInstance ()->pre_init (
    applicationFilePath ().toStdString ().c_str (), true, true);

  // TODO: init localization

  AudioEngine::set_default_backends (false);

  setup_ui ();

  gZrythm->init ();

  constexpr const char * copyright_line =
    "Copyright (C) " COPYRIGHT_YEARS " " COPYRIGHT_NAME;
  std::string ver = Zrythm::get_version (false);
  std::cout
    << "\n==============================================================\n\n"
    << format_str (
         tr ("{}-{}\n{}\n\n"
             "{} comes with ABSOLUTELY NO WARRANTY!\n\n"
             "This is free software, and you are welcome to redistribute it\n"
             "under certain conditions. See the file 'COPYING' for details.\n\n"
             "Write comments and bugs to {}\n"
             "Support this project at {}\n\n")
           .toStdString (),
         PROGRAM_NAME, ver, copyright_line, PROGRAM_NAME, ISSUE_TRACKER_URL,
         "https://liberapay.com/Zrythm")
    << "==============================================================\n\n";

  z_info ("Running Zrythm in '{}'", QDir::currentPath ());

  // TODO: install signal handlers

  // Schedule the post-exec initialization
  QTimer::singleShot (0, this, &ZrythmApplication::post_exec_initialization);
}

void
ZrythmApplication::post_exec_initialization ()
{
  setup_ipc ();

  /* init directories in user path */
  try
    {
      gZrythm->init_user_dirs_and_files ();
    }
  catch (const ZrythmException &e)
    {
      z_critical ("Failed to create user dirs and files: {}", e.what ());
      return;
    }
  gZrythm->init_templates ();
}

void
ZrythmApplication::setup_command_line_options ()
{
  cmd_line_parser_.setApplicationDescription (
    "Zrythm Digital Audio Workstation");
  cmd_line_parser_.addHelpOption ();
  cmd_line_parser_.addVersionOption ();

  cmd_line_parser_.addOptions ({
    { "project", "Open project", "project", "project" },
    { "new-project", "Create new project", "new-project", "new-project" },
    { "new-project-with-template", "Create new project with template",
     "new-project-with-template", "new-project-with-template" },
    {
     "dummy", "Use dummy audio/midi engine",
     },
  });
}

SettingsManager *
ZrythmApplication::get_settings_manager () const
{
  return settings_manager_;
}

ThemeManager *
ZrythmApplication::get_theme_manager () const
{
  return theme_manager_;
}

void
ZrythmApplication::setup_ui ()
{
  // QQuickStyle::setStyle ("Imagine");
  QIcon::setThemeName ("dark");

  // note: this is the system palette - different from qtquick palette
  // it's just used to set the window frame color
  setPalette (theme_manager_->palette_);

  // Set font scaling TODO (if needed)
  // QSettings settings;
  // double fontScale = settings.value ("UI/General/font-scale", 1.0).toDouble
  // ();

  /* prepend freedesktop system icons to search path, just in case */
  auto * dir_mgr = DirectoryManager::getInstance ();
  auto   parent_datadir =
    dir_mgr->get_dir (DirectoryManager::DirectoryType::SYSTEM_PARENT_DATADIR);
  auto freedesktop_icon_theme_dir = parent_datadir / "icons";

  auto prepend_icon_theme_search_path = [] (const fs::path &path) {
    QIcon::themeSearchPaths ().prepend (QString::fromStdString (path.string ()));
    z_debug ("added icon theme search path: {}", path);
  };

  prepend_icon_theme_search_path (freedesktop_icon_theme_dir);

  /* prepend zrythm system icons to search path */
  {
    const auto system_icon_theme_dir = dir_mgr->get_dir (
      DirectoryManager::DirectoryType::SYSTEM_THEMES_ICONS_DIR);
    prepend_icon_theme_search_path (system_icon_theme_dir);
  }

  /* prepend user custom icons to search path */
  {
    auto user_icon_theme_dir =
      dir_mgr->get_dir (DirectoryManager::DirectoryType::USER_THEMES_ICONS);
    prepend_icon_theme_search_path (user_icon_theme_dir);
  }

  // icon theme
  const auto icon_theme_name = settings_manager_->get_icon_theme ();
  z_info ("Setting icon theme to '{}'", icon_theme_name);
  QIcon::setThemeName (icon_theme_name);

  // KDDockWidgets::initFrontend (KDDockWidgets::FrontendType::QtQuick);

  // Create and show the main window
  qml_engine_ = new QQmlApplicationEngine (this);
  // KDDockWidgets::QtQuick::Platform::instance ()->setQmlEngine (&engine);

  // RESOURCE_PREFIX from CMakeLists.txt
  // allows loading QML modules from qrc
  // skips plugin loading for modules found in qrc
  qml_engine_->addImportPath (":/org.zrythm/imports/");

  // hints:
  // always load from qrc
  // files in qrc are pre-compiled
  // files in host file system are compiled at runtime
  const QUrl url ("qrc:/org.zrythm/imports/Zrythm/greeter.qml");
  qml_engine_->load (url);
  // qml_engine_->loadFromModule ("Zrythm", "greeter");

  if (!qml_engine_->rootObjects ().isEmpty ())
    {
      z_debug ("QML file loaded successfully");
    }
  else
    {
      z_critical ("Failed to load QML file");
    }
}

void
ZrythmApplication::onEngineOutput ()
{
  QByteArray output = engine_process_->readAllStandardOutput ();
  QString    outputString (output);
  z_info (
    "\n[ === Engine output === ]\n{}\n[ === End engine output === ]",
    outputString.toStdString ());
}

void
ZrythmApplication::setup_ipc ()
{
  socket_ = new QLocalSocket (this);
  socket_->connectToServer (IPC_SOCKET_NAME);
  if (socket_->waitForConnected (1000))
    {
      z_info ("Connected to IPC server");
    }
  else
    {
      z_error ("Failed to connect to IPC server: {}", socket_->errorString ());
    }
}

void
ZrythmApplication::launch_engine_process ()
{
  engine_process_ = new QProcess (this);
  engine_process_->setProcessChannelMode (QProcess::MergedChannels);
  fs::path   path;
  const auto path_from_env =
    QProcessEnvironment::systemEnvironment ().value ("ZRYTHM_ENGINE_PATH");
  if (!path_from_env.isEmpty ())
    {
      path = fs::path (path_from_env.toStdString ());
    }
  else
    {
      path = fs::path (applicationDirPath ().toStdString ()) / "zrythm-engine";
    }
  z_info (
    "Starting engine process at {} (application file path: {})", path,
    applicationFilePath ());

  // Connect the readyReadStandardOutput signal to the new slot
  connect (
    engine_process_, &QProcess::readyReadStandardOutput, this,
    &ZrythmApplication::onEngineOutput);

  engine_process_->start (QString (path.string ().c_str ()));
  if (engine_process_->waitForStarted ())
    {
      z_info ("Started engine process");
    }
  else
    {
      z_warning (
        "Failed to start engine process: {}", engine_process_->errorString ());
    }
}

bool
ZrythmApplication::notify (QObject * receiver, QEvent * event)
{
  // Run JUCE's dispatch loop before processing Qt events
  if (juce_app_wrapper_)
    {
      juce::MessageManager::getInstance ()->runDispatchLoop ();
    }

  // Process Qt event
  return QApplication::notify (receiver, event);
}

void
ZrythmApplication::onAboutToQuit ()
{
  z_info ("Shutting down...");

  if (gZrythm)
    {
      if (gZrythm->project_ && gZrythm->project_->audio_engine_)
        {
          gZrythm->project_->audio_engine_->activate (false);
        }
      gZrythm->project_.reset ();
      gZrythm->deleteInstance ();
    }

  DirectoryManager::deleteInstance ();
  PCGRand::deleteInstance ();
}

ZrythmApplication::~ZrythmApplication ()
{
  if (socket_)
    {
      if (socket_->state () == QLocalSocket::ConnectedState)
        {
          socket_->disconnectFromServer ();
        }
      socket_->close ();
    }
  if (engine_process_)
    {
      engine_process_->terminate ();
      engine_process_->waitForFinished ();
      if (engine_process_->state () != QProcess::NotRunning)
        {
          engine_process_->kill ();
        }
    }

  z_info ("ZrythmApplication destroyed - deleting logger...");
  Logger::deleteInstance ();

  juce::MessageManager::deleteInstance ();
}