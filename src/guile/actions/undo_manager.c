/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "guile/modules.h"

#ifndef SNARF_MODE
#  include "actions/undo_manager.h"
#  include "project.h"
#  include "utils/error.h"

#  include <glib/gi18n.h>
#endif

#if 0
SCM_DEFINE (
  s_undo_manager_perform,
  "undo-manager-perform", 2, 0, 0,
  (SCM undo_manager, SCM action),
  "Performs the given action and adds it to the undo stack.")
#  define FUNC_NAME s_
{
  UndoManager * undo_mgr =
    (UndoManager *) scm_to_pointer (undo_manager);

  undo_manager_perform (
    undo_mgr,
    (UndoableAction *) scm_to_pointer (action));

  return SCM_BOOL_T;
}
#  undef FUNC_NAME
#endif

SCM_DEFINE (
  s_undo_manager_undo,
  "undo-manager-undo",
  1,
  0,
  0,
  (SCM undo_manager),
  "Undoes the last action.")
#define FUNC_NAME s_
{
  UndoManager * undo_mgr = (UndoManager *) scm_to_pointer (undo_manager);

  GError * err = NULL;
  int      ret = undo_manager_undo (undo_mgr, &err);
  if (ret != 0)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to undo"));
    }

  return SCM_BOOL_T;
}
#undef FUNC_NAME

SCM_DEFINE (
  s_undo_manager_redo,
  "undo-manager-redo",
  1,
  0,
  0,
  (SCM undo_manager),
  "Redoes the last undone action.")
#define FUNC_NAME s_
{
  UndoManager * undo_mgr = (UndoManager *) scm_to_pointer (undo_manager);

  GError * err = NULL;
  int      ret = undo_manager_redo (undo_mgr, &err);
  if (ret != 0)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to redo"));
    }

  return SCM_BOOL_T;
}
#undef FUNC_NAME

static void
init_module (void * data)
{
#ifndef SNARF_MODE
#  include "actions_undo_manager.x"
#endif

  scm_c_export (
#if 0
    "undo-manager-perform",
#endif
    "undo-manager-undo", "undo-manager-redo", NULL);
}

void
guile_actions_undo_manager_define_module (void)
{
  scm_c_define_module ("actions undo-manager", init_module, NULL);
}
