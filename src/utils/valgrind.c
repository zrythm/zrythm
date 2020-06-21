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
#include "utils/io.h"
#include "utils/valgrind.h"
#include "zrythm.h"

#include <gtk/gtk.h>

void
valgrind_exec_callgrind (
  char ** argv)
{
  char * valgrind_prg =
    g_find_program_in_path ("valgrind");
  if (!valgrind_prg)
    {
      g_error (
        "valgrind is not found. "
        "Please install it first.");
    }

  char * datetime_str =
    datetime_get_for_filename ();
  char * filename =
    g_strdup_printf (
      "callgrind.out_%s.%s", datetime_str, "%p");
  char * dir =
    zrythm_get_dir (ZRYTHM_DIR_USER_PROFILING);
  io_mkdir (dir);
  char * out_file =
    g_build_filename (
      dir, filename, NULL);
  char * callgrind_out_file_arg =
    g_strdup_printf (
      "--callgrind-out-file=%s", out_file);
  g_free (dir);
  g_free (out_file);
  g_free (datetime_str);
  g_free (filename);

  /* array of args */
  const char * valgrind_args[] = {
    "valgrind", "--tool=callgrind",
    callgrind_out_file_arg,
    argv[0], NULL };

#define PRINT_ENV \
  g_message ( \
    "%s: added env %s at %zu", __func__, \
    env_var, i - 2)

  /* array of current env variables
   * + G_DEBUG */
  size_t max_size = 100;
  char ** valgrind_env =
    calloc (max_size, sizeof (char *));
  size_t i = 1;
  char * env_var;
  while ((env_var = *(environ + i++)))
    {
      if (i + 6 > max_size)
        {
          valgrind_env =
            realloc (
              valgrind_env,
              max_size * sizeof (char *));
        }
      valgrind_env[i - 2] = env_var;
      PRINT_ENV;
    }
  valgrind_env[i - 2] = NULL;

  /* run */
  execvpe (
    "valgrind", (char * const *) valgrind_args,
    (char * const *) valgrind_env);
}

#endif // __linux__
