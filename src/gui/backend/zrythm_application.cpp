// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/juce_hardware_audio_interface.h"
#include "engine/session/midi_mapping.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/plugin_protocol_paths.h"
#include "utils/backtrace.h"
#include "utils/directory_manager.h"
#include "utils/format_juce.h"
#include "utils/qsettings_backend.h"
#include "utils/thread_safe_fftw.h"

#include <QFontDatabase>
#include <QIcon>
#include <QLocalSocket>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlProperty>
#include <QQuickStyle>
#include <QTimer>
#include <QTranslator>

// FIXME: temporarily disabled - engine-process not currently used
// #include "engine-process/ipc_message.h"
#include "zrythm_application.h"
#include <backward.hpp>

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

using namespace zrythm::gui;
using namespace Qt::StringLiterals;

class ZrythmApplication::Impl
{
public:
  std::unique_ptr<backward::SignalHandling> signal_handling_;
  std::unique_ptr<juce::ScopedJuceInitialiser_GUI>
    juce_message_handler_initializer_;

  utils::QObjectUniquePtr<utils::AppSettings> app_settings_;

  QLocalSocket * socket_ = nullptr;

  std::unique_ptr<DirectoryManager>                     dir_manager_;
  utils::QObjectUniquePtr<AlertManager>                 alert_manager_;
  utils::QObjectUniquePtr<ThemeManager>                 theme_manager_;
  utils::QObjectUniquePtr<TranslationManager>           translation_manager_;
  utils::QObjectUniquePtr<ProjectManager>               project_manager_;
  utils::QObjectUniquePtr<FileSystemModel>              file_system_model_;
  utils::QObjectUniquePtr<engine::session::ControlRoom> control_room_;

  utils::QObjectUniquePtr<zrythm::gui::old_dsp::plugins::PluginManager>
    plugin_manager_;

  QProcess * engine_process_ = nullptr;

  utils::QObjectUniquePtr<QTimer> juce_dispatch_timer_;

  QQmlApplicationEngine * qml_engine_ = nullptr;

  QTranslator * translator_ = nullptr;

  std::shared_ptr<gui::backend::DeviceManager> device_manager_;

  std::unique_ptr<dsp::IHardwareAudioInterface> hw_audio_interface_;

  std::unique_ptr<engine::session::MidiMappings> midi_mappings_;
};

ZrythmApplication::ZrythmApplication (int &argc, char ** argv)
    : QApplication (argc, argv), impl_ (std::make_unique<Impl> ())
{
  // install signal handlers
  impl_->signal_handling_ = utils::Backtrace::init_signal_handlers ();

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
  impl_->app_settings_ = utils::make_qobject_unique<utils::AppSettings> (
    std::unique_ptr<utils::ISettingsBackend> (new utils::QSettingsBackend),
    this);
  auto * app_settings = impl_->app_settings_.get ();
  impl_->dir_manager_ = std::make_unique<DirectoryManager> (
    [app_settings] () {
      return utils::Utf8String::from_qstring (app_settings->zrythm_user_path ())
        .to_path ();
    },
    [] () {
      return utils::Utf8String::
        from_qstring (utils::AppSettings::get_default_zrythm_user_path ())
          .to_path ();
    },
    [] () {
      return utils::Utf8String::from_qstring (applicationDirPath ()).to_path ();
    });
  utils::init_logging (utils::LoggerType::GUI);

  /* setup command line parser */
  setup_command_line_options ();
  cmd_line_parser_.process (*this);

  // Create control room after command-line parsing to avoid leaking
  // AudioBuffers when --help/--version calls std::exit()
  impl_
    ->control_room_ = utils::make_qobject_unique<engine::session::ControlRoom> (
    [this] () -> const engine::session::ControlRoom::RealtimeTracks & {
      static boost::unordered_flat_map<
        structure::tracks::TrackUuid, structure::tracks::TrackPtrVariant>
        dummy_tracks;
      if (!impl_->project_manager_)
        {
          return dummy_tracks;
        }
      auto * project_session = impl_->project_manager_->activeSession ();
      if (project_session == nullptr)
        {
          return dummy_tracks;
        }
      return project_session->project ()->tracks_rt_;
    },
    this);

  // Initialize JUCE
  impl_->juce_message_handler_initializer_ =
    std::make_unique<juce::ScopedJuceInitialiser_GUI> ();

#ifdef __linux__
  // Dispatch JUCE messages on the main thread via a timer instead of from
  // notify(). This avoids dispatching JUCE messages multiple times on the same
  // frame which
  // can cause crashes.
  impl_->juce_dispatch_timer_ = utils::make_qobject_unique<QTimer> (this);
  QObject::connect (
    impl_->juce_dispatch_timer_.get (), &QTimer::timeout, this, [] () {
      // We need to do this on GNU/Linux otherwise plugin UIs appear blank.
      juce::detail::dispatchNextMessageOnSystemQueue (true);
    });
  impl_->juce_dispatch_timer_->start (0);
#endif

  impl_->alert_manager_ = utils::make_qobject_unique<AlertManager> (this);
  impl_->theme_manager_ = utils::make_qobject_unique<ThemeManager> (this);
  impl_->project_manager_ =
    utils::make_qobject_unique<ProjectManager> (*impl_->app_settings_, this);
  impl_->translation_manager_ = utils::make_qobject_unique<TranslationManager> (
    *impl_->app_settings_, this);
  impl_->file_system_model_ = utils::make_qobject_unique<FileSystemModel> (this);
  impl_->plugin_manager_ = utils::make_qobject_unique<
    zrythm::gui::old_dsp::plugins::PluginManager> (
    [this] (const auto protocol) {
      return plugins::PluginProtocolPaths (*impl_->app_settings_)
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
    auto * control_room = impl_->control_room_.get ();
    auto * app_settings = impl_->app_settings_.get ();
    control_room->setDimOutput (app_settings->monitorDimEnabled ());
    QObject::connect (
      control_room, &engine::session::ControlRoom::dimOutputChanged,
      app_settings, &utils::AppSettings::set_monitorDimEnabled);

    control_room->muteVolume ()->setBaseValue (
      control_room->muteVolume ()->range ().convertTo0To1 (
        app_settings->monitorMuteVolume ()));
    QObject::connect (
      control_room->muteVolume (), &dsp::ProcessorParameter::baseValueChanged,
      app_settings, [app_settings, control_room] (float value) {
        app_settings->set_monitorMuteVolume (
          control_room->muteVolume ()->range ().convertFrom0To1 (value));
      });

    control_room->listenVolume ()->setBaseValue (
      control_room->listenVolume ()->range ().convertTo0To1 (
        app_settings->monitorListenVolume ()));
    QObject::connect (
      control_room->listenVolume (), &dsp::ProcessorParameter::baseValueChanged,
      app_settings, [app_settings, control_room] (float value) {
        app_settings->set_monitorListenVolume (
          control_room->listenVolume ()->range ().convertFrom0To1 (value));
      });

    control_room->dimVolume ()->setBaseValue (
      control_room->dimVolume ()->range ().convertTo0To1 (
        app_settings->monitorDimVolume ()));
    QObject::connect (
      control_room->dimVolume (), &dsp::ProcessorParameter::baseValueChanged,
      app_settings, [app_settings, control_room] (float value) {
        app_settings->set_monitorDimVolume (
          control_room->dimVolume ()->range ().convertFrom0To1 (value));
      });

    {
      auto * monitor_fader = control_room->monitorFader ();
      monitor_fader->gain ()->setBaseValue (
        monitor_fader->gain ()->range ().convertTo0To1 (
          app_settings->monitorVolume ()));
      QObject::connect (
        monitor_fader->gain (), &dsp::ProcessorParameter::baseValueChanged,
        app_settings, [app_settings, monitor_fader] (float value) {
          app_settings->set_monitorVolume (
            monitor_fader->gain ()->range ().convertFrom0To1 (value));
        });

      monitor_fader->monoToggle ()->setBaseValue (
        app_settings->monitorMonoEnabled () ? 1.f : 0.f);
      QObject::connect (
        monitor_fader->monoToggle (), &dsp::ProcessorParameter::baseValueChanged,
        app_settings, [app_settings, monitor_fader] (float value) {
          app_settings->set_monitorMonoEnabled (
            monitor_fader->monoToggle ()->range ().is_toggled (value));
        });

      monitor_fader->mute ()->setBaseValue (
        app_settings->monitorMuteEnabled () ? 1.f : 0.f);
      QObject::connect (
        monitor_fader->mute (), &dsp::ProcessorParameter::baseValueChanged,
        app_settings, [app_settings, monitor_fader] (float value) {
          app_settings->set_monitorMuteEnabled (
            monitor_fader->mute ()->range ().is_toggled (value));
        });
    }

    {
      auto * metronome = control_room->metronome ();
      metronome->setVolume (
        static_cast<float> (app_settings->metronomeVolume ()));
      QObject::connect (
        metronome, &dsp::Metronome::volumeChanged, app_settings,
        [app_settings] (float value) {
          app_settings->set_metronomeVolume (static_cast<double> (value));
        });
      QObject::connect (
        app_settings, &utils::AppSettings::metronomeVolumeChanged, metronome,
        [metronome] (double value) {
          metronome->setVolume (static_cast<float> (value));
        });

      metronome->setEnabled (app_settings->metronomeEnabled ());
      QObject::connect (
        metronome, &dsp::Metronome::enabledChanged, app_settings,
        &utils::AppSettings::set_metronomeEnabled);
      QObject::connect (
        app_settings, &utils::AppSettings::metronomeEnabledChanged, metronome,
        &dsp::Metronome::setEnabled);
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
  impl_->device_manager_ = std::make_shared<gui::backend::DeviceManager> (
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
  impl_->device_manager_->initialize (2, 2, true);

  // Create hardware audio interface wrapper
  impl_->hw_audio_interface_ =
    dsp::JuceHardwareAudioInterface::create (impl_->device_manager_);
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
  setPalette (impl_->theme_manager_->palette_);

  // Set font scaling TODO (if needed)
  // QSettings settings;
  // double fontScale = settings.value ("UI/General/font-scale", 1.0).toDouble
  // ();

  /* prepend freedesktop system icons to search path, just in case */
  auto &dir_mgr = get_directory_manager ();
  auto  parent_datadir =
    dir_mgr.get_dir (DirectoryManager::DirectoryType::SYSTEM_PARENT_DATADIR);
  auto freedesktop_icon_theme_dir = parent_datadir / "icons";

  auto prepend_icon_theme_search_path = [] (const std::filesystem::path &path) {
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
  const auto icon_theme_name = impl_->app_settings_->icon_theme ();
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
  impl_->qml_engine_ = new QQmlApplicationEngine (this);
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
  impl_->qml_engine_->load (url);
  // qml_engine_->loadFromModule ("Zrythm", "greeter");

  if (!impl_->qml_engine_->rootObjects ().isEmpty ())
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
        impl_->qml_engine_, QUrl ("qrc:/qt/qml/ZrythmStyle/Style.qml"),
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
  QByteArray output = impl_->engine_process_->readAllStandardOutput ();
  z_info (
    "\n[ === Engine output === ]\n{}\n[ === End engine output === ]",
    output.toStdString ());
}

void
ZrythmApplication::setup_ipc ()
{
  return; // skip for now
#if 0
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
#endif
}

void
ZrythmApplication::launch_engine_process ()
{
  return; // skip for now

  impl_->engine_process_ = new QProcess (this);
  impl_->engine_process_->setProcessChannelMode (QProcess::MergedChannels);
  std::filesystem::path path;
  const auto            path_from_env =
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
    impl_->engine_process_, &QProcess::readyReadStandardOutput, this,
    &ZrythmApplication::onEngineOutput);

  impl_->engine_process_->start (
    utils::Utf8String::from_path (path).to_qstring ());
  if (impl_->engine_process_->waitForStarted ())
    {
      z_info (
        "Started engine process with PID: {}",
        impl_->engine_process_->processId ());
      z_info ("To debug, run: gdb -p {}", impl_->engine_process_->processId ());
    }
  else
    {
      z_warning (
        "Failed to start engine process: {}",
        impl_->engine_process_->errorString ());
    }
}

void
ZrythmApplication::onAboutToQuit ()
{
  z_info ("Shutting down...");
}

ZrythmApplication::~ZrythmApplication ()
{
  impl_->device_manager_->closeAudioDevice ();

  // make sure the project manager & its project are deleted first to avoid
  // holding shared resources there while other members get deallocated
  impl_->project_manager_.reset ();

  if (impl_->socket_ != nullptr)
    {
      if (impl_->socket_->state () == QLocalSocket::ConnectedState)
        {
          impl_->socket_->disconnectFromServer ();
        }
      impl_->socket_->close ();
    }
  if (impl_->engine_process_ != nullptr)
    {
      impl_->engine_process_->terminate ();
      impl_->engine_process_->waitForFinished ();
      if (impl_->engine_process_->state () != QProcess::NotRunning)
        {
          impl_->engine_process_->kill ();
        }
    }

  Zrythm::deleteInstance ();
}

zrythm::gui::ThemeManager *
ZrythmApplication::themeManager () const
{
  return impl_->theme_manager_.get ();
}

zrythm::utils::AppSettings *
ZrythmApplication::appSettings () const
{
  return impl_->app_settings_.get ();
}

zrythm::gui::ProjectManager *
ZrythmApplication::projectManager () const
{
  return impl_->project_manager_.get ();
}

zrythm::gui::old_dsp::plugins::PluginManager *
ZrythmApplication::pluginManager () const
{
  return impl_->plugin_manager_.get ();
}

zrythm::gui::AlertManager *
ZrythmApplication::alertManager () const
{
  return impl_->alert_manager_.get ();
}

zrythm::gui::TranslationManager *
ZrythmApplication::translationManager () const
{
  return impl_->translation_manager_.get ();
}

zrythm::gui::backend::DeviceManager *
ZrythmApplication::deviceManager () const
{
  return impl_->device_manager_.get ();
}

zrythm::gui::FileSystemModel *
ZrythmApplication::fileSystemModel () const
{
  return impl_->file_system_model_.get ();
}

engine::session::ControlRoom *
ZrythmApplication::controlRoom () const
{
  return impl_->control_room_.get ();
}

DirectoryManager &
ZrythmApplication::get_directory_manager () const
{
  return *impl_->dir_manager_;
}

QQmlApplicationEngine *
ZrythmApplication::get_qml_engine () const
{
  return impl_->qml_engine_;
}

std::shared_ptr<gui::backend::DeviceManager>
ZrythmApplication::get_device_manager () const
{
  return impl_->device_manager_;
}

dsp::IHardwareAudioInterface &
ZrythmApplication::hw_audio_interface () const
{
  return *impl_->hw_audio_interface_;
}
