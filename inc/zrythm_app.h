/* SPDX-License-Identifier: AGPL-3.0-or-later */
/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 */

/**
 * \file
 *
 * The Zrythm GTK application.
 */

#ifndef __ZRYTHM_APP_H__
#define __ZRYTHM_APP_H__

#include <gtk/gtk.h>

#include "zix/sem.h"

#define ZRYTHM_APP_TYPE (zrythm_app_get_type ())
G_DECLARE_FINAL_TYPE (
  ZrythmApp,
  zrythm_app,
  ZRYTHM,
  APP,
  GtkApplication)

#define ZRYTHM_APP_IS_GTK_THREAD \
  (zrythm_app \
   && zrythm_app->gtk_thread == g_thread_self ())

typedef struct _MainWindowWidget   MainWindowWidget;
typedef struct _SplashWindowWidget SplashWindowWidget;
typedef struct _FirstRunAssistantWidget
  FirstRunAssistantWidget;
typedef struct _ProjectAssistantWidget
                        ProjectAssistantWidget;
typedef struct Zrythm   Zrythm;
typedef struct UiCaches UiCaches;

/**
 * @addtogroup general
 *
 * @{
 */

/**
 * UI message for the message queue.
 */
typedef struct ZrythmAppUiMessage
{
  GtkMessageType type;
  char *         msg;
} ZrythmAppUiMessage;

ZrythmAppUiMessage *
zrythm_app_ui_message_new (
  GtkMessageType type,
  const char *   msg);

void
zrythm_app_ui_message_free (
  ZrythmAppUiMessage * self);

/**
 * The global struct.
 *
 * Contains data that is only relevant to the GUI
 * or command line.
 */
struct _ZrythmApp
{
  GtkApplication parent;

  /**
   * Default settings (got from
   * gtk_settings_get_default()).
   */
  GtkSettings * default_settings;

  /** Main window. */
  MainWindowWidget * main_window;

  /**
   * The GTK thread where the main GUI loop runs.
   *
   * This is stored for identification purposes
   * in other threads.
   */
  GThread * gtk_thread;

  UiCaches * ui_caches;

  /** Initialization thread. */
  GThread * init_thread;

  /** Semaphore for setting the progress in the
   * splash screen from a non-gtk thread. */
  ZixSem progress_status_lock;

  /** Flag to set when initialization has
   * finished. */
  bool init_finished;

  /** Status text to be used in the splash
   * screen. */
  char status[800];

  /** Splash screen. */
  SplashWindowWidget * splash;

  /** First run wizard. */
  //FirstRunAssistantWidget * first_run_assistant;

  /** Project selector. */
  //ProjectAssistantWidget * assistant;

  bool have_svg_loader;

  /** Audio backend passed with --audio-backend=,
   * if any. */
  char * audio_backend;

  /** MIDI backend passed with --audio-backend=,
   * if any. */
  char * midi_backend;

  /** Buffer size passed with --buf-size=, if any. */
  int buf_size;

  /** Samplerate passed with --samplerate=, if
   * any. */
  int samplerate;

  /** Messages to show when the main window is
   * shown. */
  /* FIXME delete this. it does the same thing as
   * the queue below */
  char * startup_errors[24];
  int    num_startup_errors;

  /** Output file passed with --output. */
  char * output_file;

  /** Whether to pretty-print. */
  bool pretty_print;

  /** CLI args. */
  int     argc;
  char ** argv;

  /** AppImage runtime path, if AppImage build. */
  char * appimage_runtime_path;

  /** Flag used to only show the RT priority
   * message once. */
  bool rt_priority_message_shown;

  /**
   * Queue for messages to be shown when the project
   * loads.
   */
  GAsyncQueue * project_load_message_queue;
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
zrythm_app_new (int argc, const char ** argv);

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

void
zrythm_app_set_font_scale (
  ZrythmApp * self,
  double      font_scale);

/**
 * Handles the logic for checking for updates on
 * startup.
 */
void
zrythm_app_check_for_updates (ZrythmApp * self);

/**
 * Unlike the init thread, this will run in the
 * main GTK thread. Do not put expensive logic here.
 *
 * This should be ran after the expensive
 * initialization has finished.
 */
int
zrythm_app_prompt_for_project_func (
  ZrythmApp * self);

/**
 * Returns a pointer to the global zrythm_app.
 */
ZrythmApp **
zrythm_app_get (void);

/**
 * @}
 */

#endif /* __ZRYTHM_APP_H__ */
