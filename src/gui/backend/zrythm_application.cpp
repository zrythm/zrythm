// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "gui/backend/backend/zrythm.h"
#include "gui/backend/plugin_protocol_paths.h"
#include "utils/directory_manager.h"
#include "utils/qsettings_backend.h"
#include "utils/thread_safe_fftw.h"

#include <QFontDatabase>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickStyle>
#include <QTimer>

#include "engine-process/ipc_message.h"
#include "zrythm_application.h"
#include <backward.hpp>

using namespace zrythm::gui;
using namespace Qt::StringLiterals;

ZrythmApplication::ZrythmApplication (int &argc, char ** argv)
    : QApplication (argc, argv), qt_thread_id_ (current_thread_id.get ()),
      control_room_ (
        utils::make_qobject_unique<engine::session::ControlRoom> (
          [this] () -> const engine::session::ControlRoom::RealtimeTracks & {
            static boost::unordered_flat_map<
              structure::tracks::TrackUuid, structure::tracks::TrackPtrVariant>
              dummy_tracks;
            if (!project_manager_)
              {
                return dummy_tracks;
              }
            auto * project_session = project_manager_->activeSession ();
            if (project_session == nullptr)
              {
                return dummy_tracks;
              }
            return project_session->project ()->tracks_rt_;
          },
          this))
{
  // install signal handlers
  signal_handling_ = utils::Backtrace::init_signal_handlers ();

  /* app info */
  setApplicationName (u"Zrythm"_s);
  setApplicationVersion (QString::fromUtf8 (PACKAGE_VERSION));
  setOrganizationName (u"Zrythm.org"_s);
  setOrganizationDomain (u"zrythm.org"_s);
  setApplicationDisplayName (u"Zrythm"_s);

  // doesn't work for some reason but the path is correct
  setWindowIcon (QIcon (":/qt/qml/Zrythm/icons/zrythm-dark/zrythm.svg"));

  // # https://github.com/FFTW/fftw3/issues/16
  ThreadSafeFFTW ();

  // Register meta-types
  qRegisterMetaType<utils::ExpandableTickRange> ();
  app_settings_ = utils::make_qobject_unique<utils::AppSettings> (
    std::unique_ptr<utils::ISettingsBackend> (new utils::QSettingsBackend),
    this);
  dir_manager_ = std::make_unique<DirectoryManager> (
    [&] () {
      return utils::Utf8String::from_qstring (app_settings_->zrythm_user_path ())
        .to_path ();
    },
    [&] () {
      return utils::Utf8String::
        from_qstring (utils::AppSettings::get_default_zrythm_user_path ())
          .to_path ();
    },
    [&] () {
      return utils::Utf8String::from_qstring (applicationDirPath ()).to_path ();
    });
  utils::LoggerProvider::set_logger (
    std::make_shared<utils::Logger> (utils::Logger::LoggerType::GUI));

  /* setup command line parser */
  setup_command_line_options ();
  cmd_line_parser_.process (*this);

  // Initialize JUCE
  juce_message_handler_initializer_ =
    std::make_unique<juce::ScopedJuceInitialiser_GUI> ();

  alert_manager_ = utils::make_qobject_unique<AlertManager> (this);
  theme_manager_ = utils::make_qobject_unique<ThemeManager> (this);
  project_manager_ =
    utils::make_qobject_unique<ProjectManager> (*app_settings_, this);
  translation_manager_ =
    utils::make_qobject_unique<TranslationManager> (*app_settings_, this);
  file_system_model_ = utils::make_qobject_unique<FileSystemModel> (this);
  plugin_manager_ = utils::make_qobject_unique<
    zrythm::gui::old_dsp::plugins::PluginManager> (
    [this] (const auto protocol) {
      return plugins::PluginProtocolPaths (*app_settings_)
        .get_for_protocol (protocol);
    },
    this);

  launch_engine_process ();

  Zrythm::getInstance ()->pre_init (
    utils::Utf8String::from_qstring (applicationFilePath ()).to_path (), true,
    true);

  setup_device_manager ();

  setup_control_room ();

  setup_ui ();

  gZrythm->init ();

  constexpr const char * copyright_line =
    "Copyright (C) " COPYRIGHT_YEARS " " COPYRIGHT_NAME;
  const auto ver = Zrythm::get_version (false);
  std::cout
    << "\n==============================================================\n\n"
    << utils::Utf8String::from_qstring (format_qstr (
         QObject::tr (
           "{}-{}\n{}\n\n"
           "{} comes with ABSOLUTELY NO WARRANTY!\n\n"
           "This is free software, and you are welcome to redistribute it\n"
           "under certain conditions. See the file 'COPYING' for details.\n\n"
           "Write comments and bugs to {}\n"
           "Support this project at {}\n\n"),
         PROGRAM_NAME, ver, copyright_line, PROGRAM_NAME, ISSUE_TRACKER_URL,
         "https://liberapay.com/Zrythm"))
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
ZrythmApplication::setup_control_room ()
{
  {
    auto * control_room = control_room_.get ();
    control_room->setDimOutput (app_settings_->monitorDimEnabled ());
    QObject::connect (
      control_room, &engine::session::ControlRoom::dimOutputChanged,
      app_settings_.get (), &utils::AppSettings::set_monitorDimEnabled);

    control_room->muteVolume ()->setBaseValue (
      control_room->muteVolume ()->range ().convertTo0To1 (
        app_settings_->monitorMuteVolume ()));
    QObject::connect (
      control_room->muteVolume (), &dsp::ProcessorParameter::baseValueChanged,
      app_settings_.get (), [&] (float value) {
        app_settings_->set_monitorMuteVolume (
          control_room->muteVolume ()->range ().convertFrom0To1 (value));
      });

    control_room->listenVolume ()->setBaseValue (
      control_room->listenVolume ()->range ().convertTo0To1 (
        app_settings_->monitorListenVolume ()));
    QObject::connect (
      control_room->listenVolume (), &dsp::ProcessorParameter::baseValueChanged,
      app_settings_.get (), [&] (float value) {
        app_settings_->set_monitorListenVolume (
          control_room->listenVolume ()->range ().convertFrom0To1 (value));
      });

    control_room->dimVolume ()->setBaseValue (
      control_room->dimVolume ()->range ().convertTo0To1 (
        app_settings_->monitorDimVolume ()));
    QObject::connect (
      control_room->dimVolume (), &dsp::ProcessorParameter::baseValueChanged,
      app_settings_.get (), [&] (float value) {
        app_settings_->set_monitorDimVolume (
          control_room->dimVolume ()->range ().convertFrom0To1 (value));
      });

    {
      auto * monitor_fader = control_room->monitorFader ();
      monitor_fader->gain ()->setBaseValue (
        monitor_fader->gain ()->range ().convertTo0To1 (
          app_settings_->monitorVolume ()));
      QObject::connect (
        monitor_fader->gain (), &dsp::ProcessorParameter::baseValueChanged,
        app_settings_.get (), [&] (float value) {
          app_settings_->set_monitorVolume (
            monitor_fader->gain ()->range ().convertFrom0To1 (value));
        });

      monitor_fader->monoToggle ()->setBaseValue (
        app_settings_->monitorMonoEnabled () ? 1.f : 0.f);
      QObject::connect (
        monitor_fader->monoToggle (), &dsp::ProcessorParameter::baseValueChanged,
        app_settings_.get (), [&] (float value) {
          app_settings_->set_monitorMonoEnabled (
            monitor_fader->monoToggle ()->range ().is_toggled (value));
        });

      monitor_fader->mute ()->setBaseValue (
        app_settings_->monitorMuteEnabled () ? 1.f : 0.f);
      QObject::connect (
        monitor_fader->mute (), &dsp::ProcessorParameter::baseValueChanged,
        app_settings_.get (), [&] (float value) {
          app_settings_->set_monitorMuteEnabled (
            monitor_fader->mute ()->range ().is_toggled (value));
        });
    }

    {
      auto * metronome = control_room->metronome ();
      metronome->setVolume (
        static_cast<float> (app_settings_->metronomeVolume ()));
      QObject::connect (
        metronome, &dsp::Metronome::volumeChanged, app_settings_.get (),
        [&] (float value) {
          app_settings_->set_metronomeVolume (static_cast<double> (value));
        });
      QObject::connect (
        app_settings_.get (), &utils::AppSettings::metronomeVolumeChanged,
        metronome, [&] (double value) {
          metronome->setVolume (static_cast<float> (value));
        });

      metronome->setEnabled (app_settings_->metronomeEnabled ());
      QObject::connect (
        metronome, &dsp::Metronome::enabledChanged, app_settings_.get (),
        &utils::AppSettings::set_metronomeEnabled);
      QObject::connect (
        app_settings_.get (), &utils::AppSettings::metronomeEnabledChanged,
        metronome, &dsp::Metronome::setEnabled);
    }
  }
}

void
ZrythmApplication::setup_device_manager ()
{
  const auto data_dir =
    QStandardPaths::writableLocation (QStandardPaths::AppDataLocation);
  const auto filepath =
    utils::Utf8String::from_path (
      utils::Utf8String::from_qstring (data_dir).to_path ()
      / u8"device_setup.xml")
      .to_juce_file ();
  device_manager_ = std::make_shared<gui::backend::DeviceManager> (
    [filepath] () -> std::unique_ptr<juce::XmlElement> {
      auto xml = juce::parseXML (filepath);
      if (xml)
        {
          z_info (
            "Successfully parsed device setup XML contents from {}", filepath);
          z_debug ("Parsed contents:\n{}", xml->toString ());
          return xml;
        }
      z_warning (
        "Failed to parse device setup XML conents from file: {}", filepath);
      return nullptr;
    },
    [filepath] (const juce::XmlElement &el) {
      const auto str = utils::Utf8String::from_juce_string (el.toString ());
      z_debug ("Audio/MIDI device setup:\n{}", str);
      if (el.writeTo (filepath))
        {
          z_info ("Saved audio/MIDI device setup to: {}", filepath);
        }
      else
        {
          z_warning ("Failed to write device setup file: {}", filepath);
        }
    });
  device_manager_->initialize (2, 2, true);

  // Create hardware audio interface wrapper
  hw_audio_interface_ =
    dsp::JuceHardwareAudioInterface::create (device_manager_);
}

void
ZrythmApplication::setup_command_line_options ()
{
  cmd_line_parser_.setApplicationDescription (
    u"Zrythm Digital Audio Workstation"_s);
  cmd_line_parser_.addHelpOption ();
  cmd_line_parser_.addVersionOption ();

  cmd_line_parser_.addOptions (
    {
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
  auto &dir_mgr = get_directory_manager ();
  auto  parent_datadir =
    dir_mgr.get_dir (DirectoryManager::DirectoryType::SYSTEM_PARENT_DATADIR);
  auto freedesktop_icon_theme_dir = parent_datadir / "icons";

  auto prepend_icon_theme_search_path = [] (const fs::path &path) {
    QIcon::themeSearchPaths ().prepend (
      utils::Utf8String::from_path (path).to_qstring ());
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
  const auto icon_theme_name = app_settings_->icon_theme ();
  z_info ("Setting icon theme to '{}'", icon_theme_name);
  QIcon::setThemeName (icon_theme_name);

  // KDDockWidgets::initFrontend (KDDockWidgets::FrontendType::QtQuick);

  // not needed - done in Style.qml - but kept for reference
#if 0
  // fonts
  int fontId = QFontDatabase::addApplicationFont (
    u":/qt/qml/Zrythm/fonts/NotoSansJP-VariableFont_wght.ttf"_s);
  if (fontId == -1)
    {
      z_warning ("Failed to load custom font!");
    }
#endif

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
  // const QUrl url (u"qrc:/qt/qml/Zrythm/DemoView.qml"_s);
  const QUrl url (u"qrc:/qt/qml/Zrythm/Greeter.qml"_s);
  qml_engine_->load (url);
  // qml_engine_->loadFromModule ("Zrythm", "greeter");

  if (!qml_engine_->rootObjects ().isEmpty ())
    {
      z_debug ("QML file loaded successfully");

      // QML_ELEMENT doesn't work as expected in these Q_GADGET's so register
      // manually
      qmlRegisterUncreatableType<structure::arrangement::ArrangerObject> (
        "Zrythm", 1, 0, "ArrangerObject", "Cannot create ArrangerObject");

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
  return; // skip for now
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
  return; // skip for now

  engine_process_ = new QProcess (this);
  engine_process_->setProcessChannelMode (QProcess::MergedChannels);
  fs::path   path;
  const auto path_from_env =
    QProcessEnvironment::systemEnvironment ().value (u"ZRYTHM_ENGINE_PATH"_s);
  if (!path_from_env.isEmpty ())
    {
      path = utils::Utf8String::from_qstring (path_from_env).to_path ();
    }
  else
    {
      path =
        utils::Utf8String::from_qstring (applicationDirPath ()).to_path ()
        / "zrythm-engine";
    }
  z_info (
    "Starting engine process at {} (application file path: {})", path,
    applicationFilePath ());

  // Connect the readyReadStandardOutput signal to the new slot
  connect (
    engine_process_, &QProcess::readyReadStandardOutput, this,
    &ZrythmApplication::onEngineOutput);

  engine_process_->start (utils::Utf8String::from_path (path).to_qstring ());
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

#ifdef __linux__
// see the following:
// - juce_audio_plugin_client/detail/juce_LinuxMessageThread.h
// - juce_events/native/juce_Messaging_linux.cpp
namespace juce::detail
{
bool
dispatchNextMessageOnSystemQueue (bool returnIfNoPendingMessages);
}
#endif

bool
ZrythmApplication::notify (QObject * receiver, QEvent * event)
{
#ifdef __linux__
  if (juce_message_handler_initializer_)
    {
      // Note: This notify() method may be called from the QML Debug server
      // thread as well as the main thread (thread 0), which causes issues with
      // the JUCE dispatcher, which expects to only be called in the main
      // thread. Thus, we only dispatch JUCE messages if this is the main thread
      if (current_thread_id.get () == qt_thread_id_)
        {
          // We need to do this on GNU/Linux otherwise plugin UIs appear blank
          juce::detail::dispatchNextMessageOnSystemQueue (true);
        }
    }
#endif

  // Process Qt event
  return QApplication::notify (receiver, event);
}

void
ZrythmApplication::onAboutToQuit ()
{
  z_info ("Shutting down...");
}

ZrythmApplication::~ZrythmApplication ()
{
  device_manager_->closeAudioDevice ();

  // make sure the project manager & its project are deleted first to avoid
  // holding shared resources there while other members get deallocated
  project_manager_.reset ();

  if (socket_ != nullptr)
    {
      if (socket_->state () == QLocalSocket::ConnectedState)
        {
          socket_->disconnectFromServer ();
        }
      socket_->close ();
    }
  if (engine_process_ != nullptr)
    {
      engine_process_->terminate ();
      engine_process_->waitForFinished ();
      if (engine_process_->state () != QProcess::NotRunning)
        {
          engine_process_->kill ();
        }
    }

  Zrythm::deleteInstance ();
}
