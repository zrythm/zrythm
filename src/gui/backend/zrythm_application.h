// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
/* SPDX-License-Identifier: LicenseRef-ZrythmLicense */

#pragma once

#include "engine/session/control_room.h"
#include "engine/session/midi_mapping.h"
#include "gui/backend/alert_manager.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/theme_manager.h"
#include "gui/backend/device_manager.h"
#include "gui/backend/file_system_model.h"
#include "gui/backend/project_manager.h"
#include "gui/backend/translation_manager.h"
#include "utils/directory_manager.h"
#include "utils/qt.h"
#include "utils/rt_thread_id.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QLocalSocket>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QTranslator>

namespace backward
{
class SignalHandling;
}

namespace zrythm::gui
{

class ZrythmApplication final : public QApplication
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::gui::ThemeManager * themeManager READ themeManager CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::gui::SettingsManager * settingsManager READ settingsManager CONSTANT
      FINAL)
  Q_PROPERTY (
    zrythm::gui::ProjectManager * projectManager READ projectManager CONSTANT
      FINAL)
  Q_PROPERTY (
    zrythm::gui::AlertManager * alertManager READ alertManager CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::gui::TranslationManager * translationManager READ translationManager
      CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::gui::backend::DeviceManager * deviceManager READ deviceManager
      CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::gui::FileSystemModel * fileSystemModel READ fileSystemModel CONSTANT
      FINAL)
  Q_PROPERTY (
    zrythm::engine::session::ControlRoom * controlRoom READ controlRoom CONSTANT
      FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ZrythmApplication (int &argc, char ** argv);
  ~ZrythmApplication () override;
  Z_DISABLE_COPY_MOVE (ZrythmApplication)

  void setup_ui ();
  void setup_ipc ();
  void launch_engine_process ();

  zrythm::gui::ThemeManager * themeManager () const
  {
    return theme_manager_.get ();
  }
  zrythm::gui::SettingsManager * settingsManager () const
  {
    return settings_manager_.get ();
  }
  zrythm::gui::ProjectManager * projectManager () const
  {
    return project_manager_.get ();
  }
  zrythm::gui::AlertManager * alertManager () const
  {
    return alert_manager_.get ();
  }
  zrythm::gui::TranslationManager * translationManager () const
  {
    return translation_manager_.get ();
  }
  zrythm::gui::backend::DeviceManager * deviceManager () const
  {
    return device_manager_.get ();
  }
  zrythm::gui::FileSystemModel * fileSystemModel () const
  {
    return file_system_model_.get ();
  }

  engine::session::ControlRoom * controlRoom () const
  {
    return control_room_.get ();
  }

  DirectoryManager &get_directory_manager () const { return *dir_manager_; }

  QQmlApplicationEngine * get_qml_engine () const { return qml_engine_; }

  bool notify (QObject * receiver, QEvent * event) override;

  std::shared_ptr<gui::backend::DeviceManager> get_device_manager () const
  {
    return device_manager_;
  }

private:
  void setup_command_line_options ();

  void post_exec_initialization ();

  void setup_device_manager ();

  void setup_control_room ();

private Q_SLOTS:
  void onEngineOutput ();
  void onAboutToQuit ();

public:
  RTThreadId::IdType qt_thread_id_;
  QCommandLineParser cmd_line_parser_;

private:
  std::unique_ptr<backward::SignalHandling> signal_handling_;
  std::unique_ptr<juce::ScopedJuceInitialiser_GUI>
    juce_message_handler_initializer_;

  /**
   * @brief Socket for communicating with the engine process.
   */
  QLocalSocket * socket_ = nullptr;

  std::unique_ptr<DirectoryManager>                     dir_manager_;
  utils::QObjectUniquePtr<AlertManager>                 alert_manager_;
  utils::QObjectUniquePtr<SettingsManager>              settings_manager_;
  utils::QObjectUniquePtr<ThemeManager>                 theme_manager_;
  utils::QObjectUniquePtr<TranslationManager>           translation_manager_;
  utils::QObjectUniquePtr<ProjectManager>               project_manager_;
  utils::QObjectUniquePtr<FileSystemModel>              file_system_model_;
  utils::QObjectUniquePtr<engine::session::ControlRoom> control_room_;

  /**
   * @brief Engine process handle.
   */
  QProcess * engine_process_ = nullptr;

  QQmlApplicationEngine * qml_engine_ = nullptr;

  QTranslator * translator_ = nullptr;

  std::shared_ptr<gui::backend::DeviceManager> device_manager_;

  /** MIDI bindings (TODO). */
  std::unique_ptr<engine::session::MidiMappings> midi_mappings_;
};

} // namespace zrythm::gui
