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


#ifdef __linux__

#define _GNU_SOURCE // for execvpe()

#include <stdlib.h>
#include <unistd.h>

#include "utils/gdb.h"

#include <gtk/gtk.h>

extern char ** environ;

void
gdb_exec (
  char ** argv,
  bool    break_at_warnings)
{
  char * gdb_prg = g_find_program_in_path ("gdb");
  if (!gdb_prg)
    {
      g_error (
        "gdb is not found. "
        "Please install it first.");
    }

  /* array of args */
  char * gdb_args[] = {
    "gdb", "-ex", "run", "--args", argv[0],  NULL };

#define PRINT_ENV \
  g_message ( \
    "%s: added env %s at %d", __func__, \
    env_var, i - 2)

  /* array of current env variables
   * + G_DEBUG */
  int max_size = 100;
  char ** gdb_env =
    calloc (max_size, sizeof (char *));
  int i = 1;
  char * env_var;
  while ((env_var = *(environ + i++)))
    {
      if (i + 6 > max_size)
        {
          gdb_env =
            realloc (
              gdb_env, max_size * sizeof (char *));
        }
      gdb_env[i - 2] = env_var;
      PRINT_ENV;
    }
  if (break_at_warnings)
    {
      gdb_env[i - 2] =
        "G_DEBUG=fatal_warnings";
      PRINT_ENV;
      gdb_env[i - 1] = NULL;
    }
  else
    {
      gdb_env[i - 2] = NULL;
    }

  /* run gdb */
  execvpe ("gdb", gdb_args, gdb_env);
}

#endif // __linux__
