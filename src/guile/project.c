// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <string.h>

#include "guile/modules.h"

#ifndef SNARF_MODE
#  include "zrythm-config.h"

#  include "project.h"
#endif

SCM_DEFINE (
  s_project_get_title,
  "project-get-title",
  1,
  0,
  0,
  (SCM project),
  "Return the project's title.")
#define FUNC_NAME s_
{
  Project * prj = (Project *) scm_to_pointer (project);

  return scm_from_stringn (
    prj->title, strlen (prj->title), "UTF8",
    SCM_FAILED_CONVERSION_QUESTION_MARK);
}
#undef FUNC_NAME

SCM_DEFINE (
  s_project_get_tracklist,
  "project-get-tracklist",
  1,
  0,
  0,
  (SCM project),
  "Returns the tracklist for the project.")
#define FUNC_NAME s_
{
  Project * prj = (Project *) scm_to_pointer (project);

  return scm_from_pointer (prj->tracklist, NULL);
}
#undef FUNC_NAME

SCM_DEFINE (
  s_project_get_undo_manager,
  "project-get-undo-manager",
  1,
  0,
  0,
  (SCM project),
  "Returns the undo manager for the project.")
#define FUNC_NAME s_
{
  Project * prj = (Project *) scm_to_pointer (project);

  return scm_from_pointer (prj->undo_manager, NULL);
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "project.x"
#endif
  scm_c_export (
    "project-get-title", "project-get-tracklist", "project-get-undo-manager",
    NULL);
}

void
guile_project_define_module (void)
{
  scm_c_define_module ("project", init_module, NULL);
}
