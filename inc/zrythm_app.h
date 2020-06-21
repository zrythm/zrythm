/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * The Zrythm GTK application.
 */

#ifndef __ZRYTHM_APP_H__
#define __ZRYTHM_APP_H__

#include <gtk/gtk.h>

#define ZRYTHM_APP_TYPE (zrythm_app_get_type ())
G_DECLARE_FINAL_TYPE (
  ZrythmApp, zrythm_app, ZRYTHM, APP,
  GtkApplication)

typedef struct _MainWindowWidget MainWindowWidget;
typedef struct _SplashWindowWidget
  SplashWindowWidget;
typedef struct _FirstRunAssistantWidget
  FirstRunAssistantWidget;
typedef struct _ProjectAssistantWidget
  ProjectAssistantWidget;
typedef struct Zrythm Zrythm;
typedef struct UiCaches UiCaches;

/**
 * @addtogroup general
 *
 * @{
 */

/**
 * The global struct.
 *
 * Contains data that is only relevant to the GUI
 * and not to Zrythm.
 */
struct _ZrythmApp
{
  GtkApplication     parent;

  /**
   * Default settings (got from
   * gtk_settings_get_default()).
   */
  GtkSettings *      default_settings;

  /** Main window. */
  MainWindowWidget * main_window;

  /**
   * The GTK thread where the main GUI loop runs.
   *
   * This is stored for identification purposes
   * in other threads.
   */
  GThread *          gtk_thread;

  UiCaches *         ui_caches;

  /** Initialization thread. */
  GThread *          init_thread;

  /** Semaphore for setting the progress in the
   * splash screen from a non-gtk thread. */
  ZixSem             progress_status_lock;

  /** Flag to set when initialization has
   * finished. */
  bool               init_finished;

  /** Status text to be used in the splash
   * screen. */
  char               status[800];

  /** Splash screen. */
  SplashWindowWidget * splash;

  /** First run wizard. */
  FirstRunAssistantWidget * first_run_assistant;

  /** Project selector. */
  ProjectAssistantWidget * assistant;

  bool               have_svg_loader;

  /** Audio backend passed with --audio-backend=,
   * if any. */
  char *             audio_backend;

  /** MIDI backend passed with --audio-backend=,
   * if any. */
  char *             midi_backend;

  /** Buffer size passed with --buf-size=, if any. */
  char *             buf_size;
};

/**
 * Global variable, should be available to all files.
 */
extern ZrythmApp * zrythm_app;

/**
 * Creates the Zrythm GApplication.
 *
 * This also initializes the Zrythm struct.
 */
ZrythmApp *
zrythm_app_new (
  char * audio_backend,
  char * midi_backend,
  char * buf_size);

/**
 * Sets the current status and progress percentage
 * during loading.
 *
 * The splash screen then reads these values from
 * the Zrythm struct.
 */
void
zrythm_app_set_progress_status (
  ZrythmApp *  self,
  const char * text,
  const double perc);

/**
 * @}
 */

#endif /* __ZRYTHM_APP_H__ */
