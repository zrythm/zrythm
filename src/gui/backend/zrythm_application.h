// SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
/* SPDX-License-Identifier: LicenseRef-ZrythmLicense */

#pragma once

#include "engine/session/control_room.h"
#include "gui/backend/alert_manager.h"
#include "gui/backend/backend/theme_manager.h"
#include "gui/backend/device_manager.h"
#include "gui/backend/file_system_model.h"
#include "gui/backend/plugin_manager.h"
#include "gui/backend/project_manager.h"
#include "gui/backend/translation_manager.h"
#include "utils/app_settings.h"
#include "utils/qt.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QQmlApplicationEngine>

class DirectoryManager;

namespace zrythm::dsp
{
class IHardwareAudioInterface;
}

namespace zrythm::gui
{

class ZrythmApplication final : public QApplication
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::gui::ThemeManager * themeManager READ themeManager CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::utils::AppSettings * appSettings READ appSettings CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::gui::ProjectManager * projectManager READ projectManager CONSTANT
      FINAL)
  Q_PROPERTY (
    zrythm::gui::old_dsp::plugins::PluginManager * pluginManager READ
      pluginManager CONSTANT FINAL)
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
  Q_DISABLE_COPY_MOVE (ZrythmApplication)

  void setup_ui ();
  void setup_ipc ();
  void launch_engine_process ();

  zrythm::gui::ThemeManager *           themeManager () const;
  zrythm::utils::AppSettings *          appSettings () const;
  zrythm::gui::ProjectManager *         projectManager () const;
  old_dsp::plugins::PluginManager *     pluginManager () const;
  zrythm::gui::AlertManager *           alertManager () const;
  zrythm::gui::TranslationManager *     translationManager () const;
  zrythm::gui::backend::DeviceManager * deviceManager () const;
  zrythm::gui::FileSystemModel *        fileSystemModel () const;

  engine::session::ControlRoom * controlRoom () const;

  DirectoryManager                            &get_directory_manager () const;
  QQmlApplicationEngine *                      get_qml_engine () const;
  std::shared_ptr<gui::backend::DeviceManager> get_device_manager () const;
  dsp::IHardwareAudioInterface                &hw_audio_interface () const;

private:
  void setup_command_line_options ();
  void post_exec_initialization ();
  void setup_device_manager ();
  void setup_control_room ();

private Q_SLOTS:
  void onEngineOutput ();
  void onAboutToQuit ();

public:
  QCommandLineParser cmd_line_parser_;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace zrythm::gui
