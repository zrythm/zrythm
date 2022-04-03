/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>

#include "audio/engine.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/string.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "guile/modules.h"
#include <libguile.h>
#include <pthread.h>

SCM position_type;
SCM track_type;
SCM tracklist_type;

#define SUCCESS_TEXT_TRANSLATED \
  _ ("Script execution successful")

#define FAILURE_TEXT_TRANSLATED \
  _ ("Script execution failed")

/**
 * Inits the guile subsystem.
 */
int
guile_init (int argc, char ** argv)
{
  g_message ("Initializing guile subsystem...");

  return 0;
}

/**
 * Defines all available modules to be used
 * by scripts.
 *
 * This must be called in guile mode.
 */
void
guile_define_modules (void)
{
  guile_actions_channel_send_action_define_module ();
  guile_actions_tracklist_selections_action_define_module ();
  guile_actions_port_connection_action_define_module ();
  guile_actions_undo_manager_define_module ();
  guile_audio_channel_define_module ();
  guile_audio_midi_note_define_module ();
  guile_audio_midi_region_define_module ();
  guile_audio_port_define_module ();
  guile_audio_position_define_module ();
  guile_audio_supported_file_define_module ();
  guile_audio_track_define_module ();
  guile_audio_track_processor_define_module ();
  guile_audio_tracklist_define_module ();
  guile_plugins_plugin_define_module ();
  guile_plugins_plugin_manager_define_module ();
  guile_project_define_module ();
  guile_zrythm_define_module ();
}

static SCM out_port;
static SCM error_out_port;

static SCM
call_proc (void * data)
{
  char * full_path = (char *) data;

  /* load a file called script.scm with the following
   * content:
   *
   * (define zrythm-script
   *   (lambda ()
   *     (display "script called") (newline)))
   */
  scm_c_primitive_load (full_path);
  SCM func = scm_variable_ref (
    scm_c_lookup ("zrythm-script"));
  scm_call_0 (func);

  return SCM_BOOL_T;
}

static SCM
eval_handler (void * handler_data, SCM key, SCM args)
{
  SCM stack = *(SCM *) handler_data;

  /* Put the code which you want to handle an error
   * after the stack has been unwound here. */

  scm_print_exception (
    error_out_port, SCM_BOOL_F, key, args);
  scm_display_backtrace (
    stack, error_out_port, SCM_BOOL_F, SCM_BOOL_F);

  return SCM_BOOL_F;
}

static SCM
preunwind_proc (
  void * handler_data,
  SCM    key,
  SCM    parameters)
{
  /* Capture the stack here: */
  *(SCM *) handler_data =
    scm_make_stack (SCM_BOOL_T, SCM_EOL);

  return SCM_BOOL_T;
}

/**
 * Function that runs in guile mode.
 */
static void *
guile_mode_func (void * data)
{
  char * script_content = (char *) data;

  char * tmp_dir =
    g_dir_make_tmp ("zrythm_script_XXXXXX", NULL);
  char * full_path =
    g_build_filename (tmp_dir, "script.scm", NULL);
  GError * err = NULL;
  g_file_set_contents (
    full_path, script_content, -1, &err);
  if (err != NULL)
    {
      g_warning (
        "Unable to write file: %s", err->message);
      g_error_free (err);
      return NULL;
    }

  guile_define_modules ();

  /* receive output */
  out_port = scm_open_output_string ();
  error_out_port = scm_open_output_string ();
  scm_set_current_output_port (out_port);
  scm_set_current_error_port (error_out_port);

  SCM captured_stack = SCM_BOOL_F;

  SCM ret = scm_c_catch (
    SCM_BOOL_T, call_proc, full_path, eval_handler,
    &captured_stack, preunwind_proc,
    &captured_stack);

  SCM    str_scm = scm_get_output_string (out_port);
  char * str = scm_to_locale_string (str_scm);
  str_scm = scm_get_output_string (error_out_port);
  char * err_str = scm_to_locale_string (str_scm);
  char * markup;
  if (ret == SCM_BOOL_T)
    {
      markup = g_markup_printf_escaped (
        "<span foreground=\"green\" size=\"large\">"
        "<b>%s</b></span>\n%s",
        SUCCESS_TEXT_TRANSLATED, str);
    }
  else
    {
      markup = g_markup_printf_escaped (
        "<span foreground=\"red\" size=\"large\">"
        "<b>%s</b></span>\n%s",
        FAILURE_TEXT_TRANSLATED, err_str);
    }

  io_rmdir (tmp_dir, true);

  return markup;
}

/**
 * Runs the script and returns the output message
 * in Pango markup.
 */
char *
guile_run_script (const char * script)
{
  /* pause engine */
  EngineState state;
  engine_wait_for_pause (
    AUDIO_ENGINE, &state, Z_F_NO_FORCE);

  char * ret = scm_with_guile (
    &guile_mode_func, (void *) script);

  /* restart engine */
  engine_resume (AUDIO_ENGINE, &state);

  return ret;
}

/**
 * Returns whether the script succeeded based on
 * the markup.
 */
bool
guile_script_succeeded (const char * pango_markup)
{
  return string_contains_substr (
    pango_markup, SUCCESS_TEXT_TRANSLATED);
}
