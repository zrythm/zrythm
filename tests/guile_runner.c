// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <gtk/gtk.h>

#include "guile/guile.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <libguile.h>
#pragma GCC diagnostic pop

static SCM out_port;
static SCM error_out_port;

static SCM
call_proc (void * data)
{
  char * full_path = (char *) data;

  /* load a file called script.scm with the following
   * content:
   *
   * (define zrythm-test
   *   (lambda ()
   *     (display "script called") (newline)))
   */
  scm_c_primitive_load (full_path);
  SCM func = scm_variable_ref (scm_c_lookup ("zrythm-test"));
  scm_call_0 (func);

  return SCM_BOOL_T;
}

static SCM
eval_handler (void * handler_data, SCM key, SCM args)
{
  SCM stack = *(SCM *) handler_data;

  /* Put the code which you want to handle an error
   * after the stack has been unwound here. */

  scm_print_exception (error_out_port, SCM_BOOL_F, key, args);
  scm_display_backtrace (stack, error_out_port, SCM_BOOL_F, SCM_BOOL_F);

  return SCM_BOOL_F;
}

static SCM
preunwind_proc (void * handler_data, SCM key, SCM parameters)
{
  /* Capture the stack here: */
  *(SCM *) handler_data = scm_make_stack (SCM_BOOL_T, SCM_EOL);

  return SCM_BOOL_T;
}

/**
 * Function that runs in guile mode.
 */
static void *
guile_mode_func (void * data)
{
  char * script_path =
    g_build_filename (TESTS_SRCDIR, (const char *) data, NULL);

  guile_define_modules ();

  /* receive output */
  out_port = scm_open_output_string ();
  error_out_port = scm_open_output_string ();
  scm_set_current_output_port (out_port);
  scm_set_current_error_port (error_out_port);

  SCM captured_stack = SCM_BOOL_F;

  SCM ret = scm_c_catch (
    SCM_BOOL_T, call_proc, script_path, eval_handler, &captured_stack,
    preunwind_proc, &captured_stack);
  g_free (script_path);

  SCM str_scm = scm_get_output_string (out_port);
  str_scm = scm_get_output_string (error_out_port);
  char * err_str = scm_to_locale_string (str_scm);
  if (ret == SCM_BOOL_T)
    {
      g_message ("Test successful");
    }
  else
    {
      g_error ("Test failed: %s", err_str);
    }

  return ret;
}

int
main (int argc, char ** argv)
{
  if (argc != 2)
    {
      g_error ("Script path is required");
    }

  if (scm_with_guile (&guile_mode_func, argv[1]) == SCM_BOOL_T)
    {
      return 0;
    }
  else
    {
      return 1;
    }
}
