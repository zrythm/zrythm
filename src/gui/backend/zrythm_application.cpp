// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/directory_manager.h"
#include "utils/pcg_rand.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/realtime_updater.h"

#include <QFontDatabase>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickStyle>
#include <QTimer>

#include "engine/ipc_message.h"
#include "zrythm_application.h"

using namespace zrythm::gui;
using namespace Qt::StringLiterals;

JUCE_CREATE_APPLICATION_DEFINE (ZrythmJuceApplicationWrapper)

ZrythmApplication::ZrythmApplication (int &argc, char ** argv)
    : QApplication (argc, argv), qt_thread_id_ (current_thread_id.get ())
{
  utils::Backtrace::init_signal_handlers ();

  /* app info */
  setApplicationName (u"Zrythm"_s);
  setApplicationVersion (QString::fromUtf8 (PACKAGE_VERSION));
  setOrganizationName (u"Zrythm.org"_s);
  setOrganizationDomain (u"zrythm.org"_s);
  setApplicationDisplayName (u"Zrythm"_s);
  // setWindowIcon (QIcon (":/org.zrythm.Zrythm/resources/icons/zrythm.svg"));

  settings_manager_ = new SettingsManager (this);
  dir_manager_ = std::make_unique<DirectoryManager> ([&] () {
    return
      fs::path(settings_manager_->get_zrythm_user_path ()
      .toStdString ());
  },
  [&] () {
      return fs::path(SettingsManager::get_default_zrythm_user_path ()
        .toStdString ());
    },
    [&]() {
      return fs::path(applicationDirPath().toStdString());
    });
  utils::LoggerProvider::set_logger(std::make_shared<utils::Logger>(utils::Logger::LoggerType::GUI));

  /* setup command line parser */
  setup_command_line_options ();
  cmd_line_parser_.process (*this);

  // Initialize JUCE
  juce ::JUCEApplicationBase ::createInstance = &juce_CreateApplication;
  juce::MessageManager::getInstance ()->setCurrentThreadAsMessageThread ();

  alert_manager_ = new AlertManager (this);
  theme_manager_ = new ThemeManager (this);
  project_manager_ = new ProjectManager (this);
  translation_manager_ = new TranslationManager (this);
  RealtimeUpdater::instance ();

  launch_engine_process ();

  Zrythm::getInstance ()->pre_init (
    applicationFilePath ().toStdString ().c_str (), true, true);

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

  // do this on demand, not on startup
#if 0
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
#endif
}

void
ZrythmApplication::setup_command_line_options ()
{
  cmd_line_parser_.setApplicationDescription (
    u"Zrythm Digital Audio Workstation"_s);
  cmd_line_parser_.addHelpOption ();
  cmd_line_parser_.addVersionOption ();

  cmd_line_parser_.addOptions ({
    { u"project"_s, tr ("Open project"), u"project"_s, u"project"_s },
    { u"new-project"_s, tr ("Create new project"), u"new-project"_s,
     u"new-project"_s },
    { u"new-project-with-template"_s, tr ("Create new project with template"),
     u"new-project-with-template"_s, u"new-project-with-template"_s },
    {
     u"dummy"_s, tr ("Use dummy audio/midi engine"),
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

ProjectManager *
ZrythmApplication::get_project_manager () const
{
  return project_manager_;
}

AlertManager *
ZrythmApplication::get_alert_manager () const
{
  return alert_manager_;
}

TranslationManager *
ZrythmApplication::get_translation_manager () const
{
  return translation_manager_;
}

void
ZrythmApplication::setup_ui ()
{
  // QQuickStyle::setStyle ("Imagine");
  QIcon::setThemeName (u"dark"_s);

  // note: this is the system palette - different from qtquick palette
  // it's just used to set the window frame color
  setPalette (theme_manager_->palette_);

  // Set font scaling TODO (if needed)
  // QSettings settings;
  // double fontScale = settings.value ("UI/General/font-scale", 1.0).toDouble
  // ();

  /* prepend freedesktop system icons to search path, just in case */
  auto & dir_mgr = get_directory_manager();
  auto   parent_datadir =
    dir_mgr.get_dir (DirectoryManager::DirectoryType::SYSTEM_PARENT_DATADIR);
  auto freedesktop_icon_theme_dir = parent_datadir / "icons";

  auto prepend_icon_theme_search_path = [] (const fs::path &path) {
    QIcon::themeSearchPaths ().prepend (QString::fromStdString (path.string ()));
    z_debug ("added icon theme search path: {}", path);
  };

  prepend_icon_theme_search_path (freedesktop_icon_theme_dir);

  /* prepend zrythm system icons to search path */
  {
    const auto system_icon_theme_dir = dir_mgr.get_dir (
      DirectoryManager::DirectoryType::SYSTEM_THEMES_ICONS_DIR);
    prepend_icon_theme_search_path (system_icon_theme_dir);
  }

  /* prepend user custom icons to search path */
  {
    auto user_icon_theme_dir =
      dir_mgr.get_dir (DirectoryManager::DirectoryType::USER_THEMES_ICONS);
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

  // ${RESOURCE_PREFIX} from CMakeLists.txt
  // allows loading QML modules from qrc
  // skips plugin loading for modules found in qrc
  // qml_engine_->addImportPath (":/org.zrythm/imports/");

  // hints:
  // always load from qrc
  // files in qrc are pre-compiled
  // files in host file system are compiled at runtime
  // ${RESOURCE_PREFIX}/${URI}
  // const QUrl url ("qrc:/qt/qml/Zrythm/DemoView.qml");
  const QUrl url (u"qrc:/qt/qml/Zrythm/Greeter.qml"_s);
  qml_engine_->load (url);
  // qml_engine_->loadFromModule ("Zrythm", "greeter");

  if (!qml_engine_->rootObjects ().isEmpty ())
    {
      z_debug ("QML file loaded successfully");

#if 0
      qmlRegisterSingletonType (
        QUrl (QStringLiteral ("qrc:/qt/qml/ZrythmStyle/Style.qml")),
        "ZrythmStyle", 1, 0, "Style");
      QQmlComponent component (
        qml_engine_, QUrl ("qrc:/qt/qml/ZrythmStyle/Style.qml"),
        QQmlComponent::PreferSynchronous);
      auto * obj = component.create ();
      auto palette = QQmlProperty::read (obj, "colorPalette").value<QPalette> ();
      // doesn't work as expected, accent color becomes green
      // setPalette (palette);
#endif
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
  z_info (
    "\n[ === Engine output === ]\n{}\n[ === End engine output === ]",
    output.toStdString ());
}

void
ZrythmApplication::setup_ipc ()
{
  socket_ = new QLocalSocket (this);
  socket_->connectToServer (QString::fromUtf8 (IPC_SOCKET_NAME));
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
    QProcessEnvironment::systemEnvironment ().value (u"ZRYTHM_ENGINE_PATH"_s);
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

  engine_process_->start (QString::fromStdString (path.string ()));
  if (engine_process_->waitForStarted ())
    {
      z_info (
        "Started engine process with PID: {}", engine_process_->processId ());
      z_info ("To debug, run: gdb -p {}", engine_process_->processId ());
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

  juce::MessageManager::deleteInstance ();
}