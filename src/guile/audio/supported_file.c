/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
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
