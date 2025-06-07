// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
/* SPDX-License-Identifier: LicenseRef-ZrythmLicense */

#pragma once

#include "zrythm-config.h"

#include "gui/backend/alert_manager.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/theme_manager.h"
#include "gui/backend/device_manager.h"
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

namespace zrythm::gui
{

class ZrythmApplication final : public QApplication
{
  Q_OBJECT

public:
  ZrythmApplication (int &argc, char ** argv);
  ~ZrythmApplication () override;
  Z_DISABLE_COPY_MOVE (ZrythmApplication)

  void setup_ui ();
  void setup_ipc ();
  void launch_engine_process ();

  DirectoryManager    &get_directory_manager () const { return *dir_manager_; }
  AlertManager *       get_alert_manager () const;
  SettingsManager *    get_settings_manager () const;
  ThemeManager *       get_theme_manager () const;
  ProjectManager *     get_project_manager () const;
  TranslationManager * get_translation_manager () const;

  QQmlApplicationEngine * get_qml_engine () const { return qml_engine_; }

  bool notify (QObject * receiver, QEvent * event) override;

  std::shared_ptr<engine::device_io::DeviceManager> get_device_manager () const
  {
    return device_manager_;
  }

private:
  void setup_command_line_options ();

  void post_exec_initialization ();

  void setup_device_manager ();

private Q_SLOTS:
  void onEngineOutput ();
  void onAboutToQuit ();

public:
  RTThreadId::IdType qt_thread_id_;
  QCommandLineParser cmd_line_parser_;

private:
  std::unique_ptr<juce::ScopedJuceInitialiser_GUI>
    juce_message_handler_initializer_;

  /**
   * @brief Socket for communicating with the engine process.
   */
  QLocalSocket * socket_ = nullptr;

  std::unique_ptr<DirectoryManager>           dir_manager_;
  utils::QObjectUniquePtr<AlertManager>       alert_manager_;
  utils::QObjectUniquePtr<SettingsManager>    settings_manager_;
  utils::QObjectUniquePtr<ThemeManager>       theme_manager_;
  utils::QObjectUniquePtr<ProjectManager>     project_manager_;
  utils::QObjectUniquePtr<TranslationManager> translation_manager_;

  /**
   * @brief Engine process handle.
   */
  QProcess * engine_process_ = nullptr;

  QQmlApplicationEngine * qml_engine_ = nullptr;

  QTranslator * translator_ = nullptr;

  std::shared_ptr<engine::device_io::DeviceManager> device_manager_;
};

} // namespace zrythm::gui
