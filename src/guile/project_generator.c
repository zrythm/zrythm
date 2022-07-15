/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "project.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "zrythm.h"

#include "guile/guile.h"
#include "guile/project_generator.h"

/**
 * Generates a Zrythm project from the script
 * contained in @ref script.
 *
 * @param script Script content.
 * @param prj_path Path to save the project at.
 *
 * @return Non-zero if fail.
 */
int
guile_project_generator_generate_project_from_string (
  const char * script,
  const char * prj_path)
{
  g_return_val_if_fail (ZRYTHM && prj_path && script, -1);

  bool use_tmp_project = false;
  if (!PROJECT)
    {
      /* create a temporary project struct */
      PROJECT = project_new (ZRYTHM);
      use_tmp_project = true;
    }

  /* create the project at a temporary path */
  char * tmp_path =
    g_dir_make_tmp ("zrythm_guile_prj_gen_XXXXXX", NULL);
  Project * prj = project_create_default (
    use_tmp_project ? PROJECT : NULL, tmp_path, true, true);

  /* save the project at the given path */
  project_save (prj, prj_path, false, false, F_NO_ASYNC);

  /* remember prev project (if any) */
  Project * prev_prj = NULL;
  if (!use_tmp_project)
    {
      prev_prj = PROJECT;
    }
  PROJECT = prj;

  /* run the script to fill in the project */
  char * markup = (char *) guile_run_script (
    script, GUILE_SCRIPT_LANGUAGE_SCHEME);
  g_message ("\nResult:\n%s", markup);

  if (!guile_script_succeeded (markup))
    {
      return -1;
    }

  /* set back the previous project (if any) */
  if (prev_prj)
    {
      PROJECT = prev_prj;
    }

  /* save the project at the given path */
  project_save (prj, prj_path, false, false, F_NO_ASYNC);

  /* free the instance */
  project_free (prj);

  /* remove temporary path */
  io_rmdir (tmp_path, true);
  g_free (tmp_path);

  return 0;
}

/**
 * Generates a Zrythm project from the @ref filepath
 * containing a generator script.
 *
 * @param filepath Path of the script file.
 * @param prj_path Path to save the project at.
 *
 * @return Non-zero if fail.
 */
int
guile_project_generator_generate_project_from_file (
  const char * filepath,
  const char * prj_path)
{
  GError * err = NULL;
  char *   contents;
  gsize    contents_len;
  g_file_get_contents (
    filepath, &contents, &contents_len, &err);
  if (err)
    {
      g_warning ("Failed to open file: %s", err->message);
      return -1;
    }

  return guile_project_generator_generate_project_from_string (
    contents, prj_path);
}
