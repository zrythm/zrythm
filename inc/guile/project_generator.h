/*
 * SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Project/template generator.
 */

#ifndef __GUILE_PROJECT_GENERATOR_H__
#define __GUILE_PROJECT_GENERATOR_H__

typedef struct Project Project;

/**
 * @addtogroup guile
 *
 * @{
 */

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
  const char * prj_path);

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
  const char * prj_path);

/**
 * @}
 */

#endif
