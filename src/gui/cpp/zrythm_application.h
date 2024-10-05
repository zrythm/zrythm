// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
/* SPDX-License-Identifier: LicenseRef-ZrythmLicense */

#ifndef ZRYTHM_APPLICATION_H_INCLUDED
#define ZRYTHM_APPLICATION_H_INCLUDED

#include "zrythm-config.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QLocalSocket>
#include <QProcess>
#include <QQmlApplicationEngine>

#include "gui/cpp/glue/settings_manager.h"
#include "gui/cpp/glue/theme_manager.h"
#include "utils/rt_thread_id.h"

class ZrythmApplication final : public QApplication
{
  Q_OBJECT

public:
  ZrythmApplication (int &argc, char ** argv);

  ~ZrythmApplication () override;

  void setup_ui ();
  void setup_ipc ();
  void launch_engine_process ();

  SettingsManager * get_settings_manager () const;
  ThemeManager *    get_theme_manager () const;

private:
  void setup_command_line_options ();

  void post_exec_initialization ();

private Q_SLOTS:
  void onEngineOutput ();
  void onAboutToQuit ();

public:
  RTThreadId::IdType qt_thread_id_;
  QCommandLineParser cmd_line_parser_;

private:
  /**
   * @brief Socket for communicating with the engine process.
   */
  QLocalSocket * socket_ = nullptr;

  SettingsManager * settings_manager_ = nullptr;
  ThemeManager *    theme_manager_ = nullptr;

  /**
   * @brief Engine process handle.
   */
  QProcess * engine_process_ = nullptr;

  QQmlApplicationEngine * qml_engine_ = nullptr;
};

/**
 * @}
 */

#endif /* ZRYTHM_APPLICATION_H_INCLUDED */
