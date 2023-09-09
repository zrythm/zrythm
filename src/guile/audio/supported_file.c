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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "guile/modules.h"

#ifndef SNARF_MODE
#  include "dsp/supported_file.h"
#  include "project.h"
#endif

SCM_DEFINE (
  s_supported_file_new_from_path,
  "supported-file-new-from-path",
  1,
  0,
  0,
  (SCM path),
  "Returns an instance of SupportedFile.")
#define FUNC_NAME s_
{
  SupportedFile * file = supported_file_new_from_path (scm_to_pointer (path));

  return scm_from_pointer (file, NULL);
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "audio_supported_file.x"
#endif

  scm_c_export ("supported-file-new-from-path", NULL);
}

void
guile_audio_supported_file_define_module (void)
{
  scm_c_define_module ("audio supported-file", init_module, NULL);
}
