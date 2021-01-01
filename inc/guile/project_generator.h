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
 */
void
guile_project_generator_generate_project_from_string (
  const char * script,
  const char * prj_path);

/**
 * Generates a Zrythm project from the @ref filepath
 * containing a generator script.
 *
 * @param filepath Path of the script file.
 * @param prj_path Path to save the project at.
 */
void
guile_project_generator_generate_project_from_file (
  const char * filepath,
  const char * prj_path);

/**
 * @}
 */

#endif
