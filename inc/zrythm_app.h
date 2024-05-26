// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
/* SPDX-License-Identifier: LicenseRef-ZrythmLicense */

/**
 * \file
 *
 * The Zrythm GTK application.
 */

#ifndef __ZRYTHM_APP_H__
#define __ZRYTHM_APP_H__

#include "zrythm-config.h"

#include <memory>

#include "utils/types.h"

#include <adwaita.h>
#include <gtk/gtk.h>

#include <zix/sem.h>

#define ZRYTHM_APP_TYPE (zrythm_app_get_type ())
G_DECLARE_FINAL_TYPE (ZrythmApp, zrythm_app, ZRYTHM, APP, GtkApplication)

#define ZRYTHM_APP_IS_GTK_THREAD \
  (zrythm_app && zrythm_app->gtk_thread == g_thread_self ())

TYPEDEF_STRUCT_UNDERSCORED (MainWindowWidget);
TYPEDEF_STRUCT_UNDERSCORED (BugReportDialogWidget);
TYPEDEF_STRUCT_UNDERSCORED (GreeterWidget);
TYPEDEF_STRUCT (Zrythm);
TYPEDEF_STRUCT (UiCaches);
class ZrythmDirectoryManager;

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
zrythm_app_ui_message_new (GtkMessageType type, const char * msg);

void
zrythm_app_ui_message_free (ZrythmAppUiMessage * self);

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

  /** Flag to set when initialization has
   * finished. */
  bool init_finished;

  /** Greeter screen. */
  GreeterWidget * greeter;

  /**
   * True if this is the first time Zrythm is runh
   *
   * This remains true even after setting the corresponding GSettings value.
   */
  bool is_first_run;

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

  /** Currently opened bug report dialog. */
  BugReportDialogWidget * bug_report_dialog;

  guint project_autosave_source_id;
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

void
zrythm_app_set_font_scale (ZrythmApp * self, double font_scale);

/**
 * Handles the logic for checking for updates on
 * startup.
 */
void
zrythm_app_check_for_updates (ZrythmApp * self);

/**
 * Unlike the init thread, this will run in the main GTK thread. Do not put
 * expensive logic here.
 *
 * This should be ran after the expensive initialization has finished.
 */
int
zrythm_app_prompt_for_project_func (ZrythmApp * self);

/**
 * Returns a pointer to the global zrythm_app.
 */
ZrythmApp **
zrythm_app_get (void);

/**
 * Shows the trial limitation error message.
 *
 * @return Whether the limit was reached.
 */
bool
zrythm_app_check_and_show_trial_limit_error (ZrythmApp * self);

void *
zrythm_app_init_thread (ZrythmApp * self);

/**
 * Install accelerator for an action.
 *
 * @memberof ZrythmApp
 */
void
zrythm_app_install_action_accel (
  ZrythmApp *  self,
  const char * primary,
  const char * secondary,
  const char * action_name);

/**
 * @memberof ZrythmApp
 */
char *
zrythm_app_get_primary_accel_for_action (
  ZrythmApp *  self,
  const char * action_name);

/**
 * To be used to exit Zrythm using the "response" signal on a message dialog.
 */
void
zrythm_exit_response_callback (AdwDialog * dialog, gpointer user_data);

/**
 * @}
 */

#endif /* __ZRYTHM_APP_H__ */
