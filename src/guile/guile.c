/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "guile/audio/position.h"
#include "guile/zrythm.h"

#include <gtk/gtk.h>

#include <libguile.h>
#include <pthread.h>

#if 0
/**
 * Function that runs in guile mode.
 */
static void*
guile_mode_func (void* data)
{
  /* load a file called script.scm with the following
   * content:
   *
   * (define zrythm-script
   *   (lambda ()
   *     (display "script called") (newline)))
   */
  scm_c_primitive_load ("script.scm");
  SCM func =
    scm_variable_ref (
      scm_c_lookup ("zrythm-script"));
  scm_call_0 (func);

  return NULL;
}

/**
 * Special thread for guile.
 */
static void *
thread_start_routine (void * arg)
{
  scm_with_guile (&guile_mode_func, NULL);

  return NULL;
}
#endif

/**
 * Inits the guile subsystem.
 */
int
guile_init (
  int     argc,
  char ** argv)
{
  g_message ("Initializing guile subsystem...");

#if 0
  /* spawn thread for guile mode */
  pthread_t thread_id;
  if (pthread_create (
        &thread_id, NULL,
        &thread_start_routine, NULL))
    {
      g_critical (
        "failed to create thread for guile");
      return -1;
    }

#endif
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
  guile_audio_position_define_module ();
  guile_zrythm_define_module ();
}
