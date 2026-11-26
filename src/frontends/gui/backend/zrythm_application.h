// SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
/* SPDX-License-Identifier: LicenseRef-ZrythmLicense */

#pragma once

#include <expected>

#include "engine/session/control_room.h"
#include "frontends/gui/backend/chord_preset_manager.h"
#include "frontends/gui/backend/device_manager.h"
#include "frontends/gui/backend/file_system_model.h"
#include "frontends/gui/backend/notification_center.h"
#include "frontends/gui/backend/plugin_manager.h"
#include "frontends/gui/backend/project_manager.h"
#include "frontends/gui/backend/translation_manager.h"
#include "utils/app_settings.h"
#include "utils/qt.h"
#include "utils/utf8_string.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QQmlApplicationEngine>

class DirectoryManager;

namespace zrythm::dsp
{
class IHardwareAudioInterface;
}

namespace zrythm::frontends::gui
{

class AppScope;

class ZrythmApplication final : public QApplication
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::utils::AppSettings * appSettings READ appSettings CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::frontends::gui::ProjectManager * projectManager READ projectManager
      CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::frontends::gui::old_dsp::plugins::PluginManager * pluginManager READ
      pluginManager CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::frontends::gui::NotificationCenter * notificationCenter READ
      notificationCenter CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::frontends::gui::TranslationManager * translationManager READ
      translationManager CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::frontends::gui::backend::DeviceManager * deviceManager READ
      deviceManager CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::frontends::gui::FileSystemModel * fileSystemModel READ
      fileSystemModel CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::engine::session::ControlRoom * controlRoom READ controlRoom CONSTANT
      FINAL)
  Q_PROPERTY (
    ChordPresetManager * chordPresetManager READ chordPresetManager CONSTANT
      FINAL)
  Q_PROPERTY (
    QString pendingProjectFile READ pendingProjectFile NOTIFY
      pendingProjectFileChanged FINAL)
  /** Whether the engine runs on the dummy audio and MIDI hardware. */
  Q_PROPERTY (bool dummyEngine READ dummyEngine CONSTANT FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ZrythmApplication (int &argc, char ** argv);
  ~ZrythmApplication () override;
  Q_DISABLE_COPY_MOVE (ZrythmApplication)

  /**
   * @brief Checks that the data required at startup exists in the
   * installation being executed, resolving each location the same way
   * startup does.
   *
   * Callable without an instance: the constructor itself requires this
   * data.
   *
   * @return An error describing the first failed check, or success.
   */
  static std::expected<void, zrythm::utils::Utf8String> verify_installation ();

  void setup_ui ();
  void setup_ipc ();
  void launch_engine_process ();

  zrythm::utils::AppSettings *                     appSettings () const;
  zrythm::frontends::gui::ProjectManager *         projectManager () const;
  old_dsp::plugins::PluginManager *                pluginManager () const;
  zrythm::frontends::gui::NotificationCenter *     notificationCenter () const;
  zrythm::frontends::gui::TranslationManager *     translationManager () const;
  zrythm::frontends::gui::backend::DeviceManager * deviceManager () const;
  zrythm::frontends::gui::FileSystemModel *        fileSystemModel () const;

  engine::session::ControlRoom * controlRoom () const;

  AppScope &appScope () const;

  ChordPresetManager * chordPresetManager () const;

  DirectoryManager       &get_directory_manager () const;
  QQmlApplicationEngine * get_qml_engine () const;
  std::shared_ptr<frontends::gui::backend::DeviceManager>
                                get_device_manager () const;
  dsp::IHardwareAudioInterface &hw_audio_interface () const;

  /**
   * @brief Whether audio is processed through a dummy device and no
   * MIDI device is opened.
   */
  bool dummyEngine () const;

  QString pendingProjectFile () const;

  /**
   * @brief Requests that the project at @p path be opened.
   *
   * A project file path resolves to its directory. An empty path is
   * ignored, and so is a request for the directory that is already
   * pending.
   */
  Q_INVOKABLE void setPendingProjectFile (const QString &path);

  /**
   * @brief Returns the project that a request asked to open, and forgets
   * the request.
   *
   * A request comes from the command line, from the operating system,
   * or from the user in the application. pendingProjectFile() gives an
   * empty string afterwards, until another request arrives.
   *
   * @return The directory of the project, or an empty string when no
   * request is pending.
   */
  Q_INVOKABLE QString takePendingProjectFile ();

private:
  /**
   * @brief Runs the application initialization performed by the
   * constructor.
   *
   * Called by the constructor inside a try block so startup failures are
   * reported (log and dialog) before the exception propagates.
   */
  void initialize ();

  void setup_command_line_options ();

  /**
   * @brief Uses the private directory requested with --user-data-dir, then
   * requests every location once.
   *
   * @throw ZrythmException When the directory cannot be used, or when a
   * location cannot be given.
   */
  void setup_user_directories ();

  void process_command_line ();
  void post_exec_initialization ();
  void setup_device_manager ();
  void setup_control_room ();

  /**
   * @brief Makes the application quit a fixed delay after startup.
   *
   * Startup ends when the project requested on the command line is
   * loaded or refused, or with the first event loop iteration when no
   * project was requested. The application quits with EXIT_FAILURE
   * when startup has not ended within the startup time limit.
   */
  void setup_quit_after_startup ();

  /**
   * @brief Ends startup for the quit after startup.
   *
   * Stops the startup time limit and starts the countdown to the quit.
   * Later calls do nothing.
   */
  void startupFinished ();

  /**
   * @brief Quits with EXIT_FAILURE because startup did not end within
   * the startup time limit.
   */
  void startupTimeLimitReached ();

  /**
   * @brief Leaves the event loop.
   *
   * The exit status is EXIT_FAILURE when loading a project failed since
   * startup or startup did not end within its time limit, EXIT_SUCCESS
   * otherwise.
   */
  void request_quit ();

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
   * Project directory pending to be opened, as requested on the command
   * line, by the operating system (eg, double-clicking a .zpj file), or
   * by the user in the application.
   *
   * Empty while no request is pending.
   */
  QString pending_project_file_;
};

} // namespace zrythm::frontends::gui
