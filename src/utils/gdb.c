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

#include "utils/datetime.h"
#include "utils/gdb.h"
#include "utils/io.h"
#include "zrythm.h"

#include <gtk/gtk.h>

extern char ** environ;

void
gdb_exec (
  char ** argv,
  bool    break_at_warnings,
  bool    interactive)
{
  char * gdb_prg = g_find_program_in_path ("gdb");
  if (!gdb_prg)
    {
      g_error (
        "gdb is not found. "
        "Please install it first.");
    }

  char * datetime_str =
    datetime_get_for_filename ();
  char * filename =
    g_strdup_printf (
      "gdb_%s.bt", datetime_str);
  char * dir =
    zrythm_get_dir (ZRYTHM_DIR_USER_GDB);
  io_mkdir (dir);
  char * out_file =
    g_build_filename (
      dir, filename, NULL);
  char * gdb_out_file_arg =
    g_strdup_printf (
      "set logging file %s", out_file);
  g_free (dir);
  g_free (out_file);
  g_free (datetime_str);
  g_free (filename);

  /* see https://blog.cryptomilk.org/2010/12/23/gdb-backtrace-to-file/ and
   * https://wiki.gentoo.org/wiki/Project:Quality_Assurance/Backtraces for more details */

  /* array of args */
  const char ** gdb_args;
  if (interactive)
    {
      const char * _gdb_args[] = {
        "gdb", "-q",
        "-ex", "run",
        "-ex", "set logging overwrite on",
        "-ex", gdb_out_file_arg,
        "-ex", "set logging on",
        "-ex", "set pagination off",
        "-ex", "handle SIG32 pass nostop noprint",
        "-ex", "echo backtrace:\\n",
        "-ex", "backtrace full",
        "--args", argv[0],  NULL };
      gdb_args = _gdb_args;
    }
  else
    {
      const char * _gdb_args[] = {
        "gdb", "-q", "--batch",
        "-ex", "run",
        "-ex", "set logging overwrite on",
        "-ex", gdb_out_file_arg,
        "-ex", "set logging on",
        "-ex", "set pagination off",
        "-ex", "handle SIG32 pass nostop noprint",
        "-ex", "echo backtrace:\\n",
        "-ex", "backtrace full",
        "-ex", "echo \\n\\nregisters:\\n",
        "-ex", "info registers",
        "-ex",
        "echo \\n\\ncurrent instructions:\\n",
        "-ex", "x/16i $pc",
        "-ex", "echo \\n\\nthreads backtrace:\\n",
        "-ex", "thread apply all backtrace",
        "-ex", "set logging off",
        "-ex", "quit",
        "--args", argv[0],  NULL };
      gdb_args = _gdb_args;
    }

#define PRINT_ENV \
  g_message ( \
    "%s: added env %s at %zu", __func__, \
    gdb_env[i - 2], i - 2)

  /* array of current env variables
   * + G_DEBUG */
  size_t max_size = 100;
  const char ** gdb_env =
    calloc (max_size, sizeof (char *));
  size_t i = 1;
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
  execvpe (
    "gdb", (char * const *) gdb_args,
    (char * const *) gdb_env);
}

#endif // __linux__
