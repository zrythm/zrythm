// SPDX-FileCopyrightText: © 2021-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "project.h"
#include "utils/error.h"
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
  char *    tmp_path = g_dir_make_tmp ("zrythm_guile_prj_gen_XXXXXX", NULL);
  GError *  err = NULL;
  Project * prj = project_create_default (
    use_tmp_project ? PROJECT : NULL, tmp_path, true, true, &err);
  if (!prj)
    {
      HANDLE_ERROR_LITERAL (err, "Failed to create default project");
      return -1;
    }

  /* save the project at the given path */
  bool success = project_save (prj, prj_path, false, false, F_NO_ASYNC, &err);
  if (!success)
    {
      HANDLE_ERROR_LITERAL (err, "Failed to save project");
      return -1;
    }

  /* remember prev project (if any) */
  Project * prev_prj = NULL;
  if (!use_tmp_project)
    {
      prev_prj = PROJECT;
    }
  PROJECT = prj;

  /* run the script to fill in the project */
  char * markup =
    (char *) guile_run_script (script, GUILE_SCRIPT_LANGUAGE_SCHEME);
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
  success = project_save (prj, prj_path, false, false, F_NO_ASYNC, &err);
  if (!success)
    {
      HANDLE_ERROR_LITERAL (err, "Failed to save project");
      return -1;
    }

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
  g_file_get_contents (filepath, &contents, &contents_len, &err);
  if (err)
    {
      g_warning ("Failed to open file: %s", err->message);
      return -1;
    }

  return guile_project_generator_generate_project_from_string (
    contents, prj_path);
}
