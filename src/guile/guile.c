// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
  char * code_str = (char *) data;

  SCM eval_string_proc =
    scm_variable_ref (
      scm_c_public_lookup (
        "ice-9 eval-string", "eval-string"));
  SCM code =
    scm_from_utf8_string (code_str);
  SCM s_from_lang =
    /* can switch this to any language supported
     * by the guile compiler, for example
     * "ecmascript" */
    /* TODO add a dropdown in the UI */
    scm_from_utf8_symbol ("scheme");
  SCM kwd_lang = scm_from_utf8_keyword ("lang");
  scm_call_3 (
    eval_string_proc, code, kwd_lang, s_from_lang);

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

  guile_define_modules ();

  /* receive output */
  out_port = scm_open_output_string ();
  error_out_port = scm_open_output_string ();
  scm_set_current_output_port (out_port);
  scm_set_current_error_port (error_out_port);

  SCM captured_stack = SCM_BOOL_F;

  SCM ret = scm_c_catch (
    SCM_BOOL_T, call_proc, script_content,
    eval_handler,
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
