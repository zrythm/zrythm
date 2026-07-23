// SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
/* SPDX-License-Identifier: LicenseRef-ZrythmLicense */

#pragma once

#include "engine/session/control_room.h"
#include "gui/backend/alert_manager.h"
#include "gui/backend/chord_preset_manager.h"
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
  Q_PROPERTY (
    ChordPresetManager * chordPresetManager READ chordPresetManager CONSTANT
      FINAL)
  Q_PROPERTY (
    QString pendingProjectFile READ pendingProjectFile NOTIFY
      pendingProjectFileChanged FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ZrythmApplication (int &argc, char ** argv);
  ~ZrythmApplication () override;
  Q_DISABLE_COPY_MOVE (ZrythmApplication)

  void setup_ui ();
  void setup_ipc ();
  void launch_engine_process ();

  zrythm::utils::AppSettings *          appSettings () const;
  zrythm::gui::ProjectManager *         projectManager () const;
  old_dsp::plugins::PluginManager *     pluginManager () const;
  zrythm::gui::AlertManager *           alertManager () const;
  zrythm::gui::TranslationManager *     translationManager () const;
  zrythm::gui::backend::DeviceManager * deviceManager () const;
  zrythm::gui::FileSystemModel *        fileSystemModel () const;

  engine::session::ControlRoom * controlRoom () const;

  ChordPresetManager * chordPresetManager () const;

  DirectoryManager                            &get_directory_manager () const;
  QQmlApplicationEngine *                      get_qml_engine () const;
  std::shared_ptr<gui::backend::DeviceManager> get_device_manager () const;
  dsp::IHardwareAudioInterface                &hw_audio_interface () const;

  QString pendingProjectFile () const;

private:
  void setup_command_line_options ();
  void process_command_line ();
  void set_pending_project_file (const QString &path);
  void post_exec_initialization ();
  void setup_device_manager ();
  void setup_control_room ();

protected:
  bool notify (QObject * receiver, QEvent * event) override;
  bool event (QEvent * event) override;

private Q_SLOTS:
  void onEngineOutput ();
  void onAboutToQuit ();

Q_SIGNALS:
  void pendingProjectFileChanged (const QString &path);

public:
  QCommandLineParser cmd_line_parser_;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;

  /**
   * Project directory pending to be opened, as passed on the command line or
   * via a file-open request from the OS (eg, double-clicking a .zpj file).
   *
   * Empty if no project was requested to be opened at startup.
   */
  QString pending_project_file_;
};

} // namespace zrythm::gui
