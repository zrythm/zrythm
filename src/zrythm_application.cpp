// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <QFontDatabase>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTimer>

#include "settings/settings_manager.h"

#include "engine/ipc_message.h"
#include "zrythm_application.h"
// #include <kddockwidgets/Config.h>
// #include <kddockwidgets/qtquick/Platform.h>
// #include <kddockwidgets/qtquick/ViewFactory.h>

ZrythmApplication::ZrythmApplication (int &argc, char ** argv)
    : QApplication (argc, argv), qt_thread_id_ (current_thread_id.get ())
{
  Backtrace::init_signal_handlers ();

  /* app info */
  setApplicationName ("Zrythm");
  setApplicationVersion (PACKAGE_VERSION);
  setOrganizationName ("Zrythm DAW");
  setOrganizationDomain ("zrythm.org");
  setApplicationDisplayName ("Zrythm");
  // setWindowIcon (QIcon (":/org.zrythm.Zrythm/resources/icons/zrythm.svg"));

  /* setup command line parser */
  setup_command_line_options ();

  setup_ui ();
  launch_engine_process ();
}

void
ZrythmApplication::post_exec_initialization ()
{
  Zrythm::getInstance ()->pre_init (
    applicationFilePath ().toStdString ().c_str (), true, true);

  // TODO: init localization

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

  setup_ipc ();
}

void
ZrythmApplication::setup_command_line_options ()
{
  cmd_line_parser_.setApplicationDescription (
    "Zrythm Digital Audio Workstation");
  cmd_line_parser_.addHelpOption ();
  cmd_line_parser_.addVersionOption ();

  cmd_line_parser_.addOptions ({
    { "project",                   "Open project",                     "project",     "project"                   },
    { "new-project",               "Create new project",               "new-project", "new-project"               },
    { "new-project-with-template", "Create new project with template",
     "new-project-with-template",                                                     "new-project-with-template" }
  });
}

void
ZrythmApplication::setup_ui ()
{
  QQuickStyle::setStyle ("Imagine");
  QIcon::setThemeName ("dark");

  // KDDockWidgets::initFrontend (KDDockWidgets::FrontendType::QtQuick);

  // Create and show the main window
  qml_engine_ = new QQmlApplicationEngine (this);
  // KDDockWidgets::QtQuick::Platform::instance ()->setQmlEngine (&engine);

  qml_engine_->addImportPath (":/org.zrythm.Zrythm/imports");

  qml_engine_->rootContext ()->setContextProperty (
    "settingsManager", SettingsManager::getInstance ());

  const QUrl url (QStringLiteral (
    "qrc:/org.zrythm.Zrythm/imports/Zrythm/resources/ui/greeter.qml"));
  // u"qrc:/org.zrythm.Zrythm/imports/Zrythm/resources/ui/greeter.qml"_qs);
  qml_engine_->load (url);

  if (!qml_engine_->rootObjects ().isEmpty ())
    {
      qDebug () << "QML file loaded successfully";
    }
  else
    {
      qDebug () << "Failed to load QML file";
    }

  // Load custom font
  int fontId = QFontDatabase::addApplicationFont (
    "/usr/share/fonts/cantarell/Cantarell-VF.otf");
  if (fontId == -1)
    {
      qWarning () << "Failed to load custom font";
    }

  // Schedule the post-exec initialization
  QTimer::singleShot (0, this, &ZrythmApplication::post_exec_initialization);
}

void
ZrythmApplication::handle_engine_output ()
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
  auto path = fs::path (applicationDirPath ().toStdString ()) / "zrythm-engine";
  z_info (
    "Starting engine process at {} (application file path: {})", path,
    applicationFilePath ());

  // Connect the readyReadStandardOutput signal to the new slot
  connect (
    engine_process_, &QProcess::readyReadStandardOutput, this,
    &ZrythmApplication::handle_engine_output);

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
}