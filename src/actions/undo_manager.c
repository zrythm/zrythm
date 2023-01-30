// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/undo_manager.h"
#include "actions/undo_stack.h"
#include "actions/undoable_action.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/header.h"
#include "gui/widgets/home_toolbar.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/error.h"
#include "utils/objects.h"
#include "utils/stack.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

typedef enum
{
  Z_ACTIONS_UNDO_MANAGER_ERROR_FAILED,
  Z_ACTIONS_UNDO_MANAGER_ERROR_ACTION_FAILED,
} ZActionsUndoManagerError;

#define Z_ACTIONS_UNDO_MANAGER_ERROR \
  z_actions_undo_manager_error_quark ()
GQuark
z_actions_undo_manager_error_quark (void);
G_DEFINE_QUARK (
  z - actions - undo - manager - error - quark,
  z_actions_undo_manager_error)

/**
 * Inits the undo manager by populating the
 * undo/redo stacks.
 */
void
undo_manager_init_loaded (UndoManager * self)
{
  g_message ("%s: loading...", __func__);
  undo_stack_init_loaded (self->undo_stack);
  undo_stack_init_loaded (self->redo_stack);
  zix_sem_init (&self->action_sem, 1);
  g_message ("%s: done", __func__);
}

/**
 * Inits the undo manager by creating the undo/redo
 * stacks.
 */
UndoManager *
undo_manager_new (void)
{
  g_message ("%s: creating...", __func__);
  UndoManager * self = object_new (UndoManager);
  self->schema_version = UNDO_MANAGER_SCHEMA_VERSION;

  self->undo_stack = undo_stack_new ();
  self->redo_stack = undo_stack_new ();

  zix_sem_init (&self->action_sem, 1);

  g_message ("%s: done", __func__);

  return self;
}

/**
 * Does or undoes the given action.
 *
 * @param action An action, when performing,
 *   otherwise NULL if undoing/redoing.
 * @param main_stack Undo stack if undoing, redo
 *   stack if doing.
 */
static int
do_or_undo_action (
  UndoManager *    self,
  UndoableAction * action,
  UndoStack *      main_stack,
  UndoStack *      opposite_stack,
  GError **        error)
{
  bool need_pop = false;
  if (!action)
    {
      /* pop the action from the undo stack and
       * undo it */
      action = (UndoableAction *) undo_stack_peek (main_stack);
      need_pop = true;
    }

  if (ZRYTHM_HAVE_UI)
    {
      /* process UI events now */
      event_manager_process_now (EVENT_MANAGER);
    }

  int ret = 0;
  if (main_stack == self->undo_stack)
    {
      ret = undoable_action_undo (action, error);
    }
  else if (main_stack == self->redo_stack)
    {
      ret = undoable_action_do (action, error);
    }
  else
    {
      /* invalid stack */
      g_critical ("%s: invalid stack (err %d)", __func__, ret);
      return -1;
    }

  /* if error return */
  if (ret != 0)
    {
      zix_sem_post (&self->action_sem);
      g_warning (
        "%s: action not performed (err %d)", __func__, ret);
      return -1;
    }
  /* else if no error pop from the stack */
  else if (need_pop)
    {
      undo_stack_pop (main_stack);
    }

  /* if redo stack is locked don't alter it */
  if (self->redo_stack_locked && opposite_stack == self->redo_stack)
    return 0;

  /* if the redo stack is full, delete the last
   * element */
  if (undo_stack_is_full (opposite_stack))
    {
      UndoableAction * action_to_delete = (UndoableAction *)
        undo_stack_pop_last (opposite_stack);

      /* TODO create functions to delete
       * unnecessary files held by the action
       * (eg, something that calls
       * plugin_delete_state_files()) */
      undoable_action_free (action_to_delete);
    }

  /* push action to the redo stack */
  undo_stack_push (opposite_stack, action);
  undo_stack_get_total_cached_actions (opposite_stack);

  return 0;
}

/**
 * Undo last action.
 */
int
undo_manager_undo (UndoManager * self, GError ** error)
{
  g_warn_if_fail (!undo_stack_is_empty (self->undo_stack));

  zix_sem_wait (&self->action_sem);

  UndoableAction * action =
    (UndoableAction *) undo_stack_peek (self->undo_stack);
  g_return_val_if_fail (action, -1);
  const int num_actions = action->num_actions;
  g_return_val_if_fail (num_actions > 0, -1);

  int ret = 0;
  for (int i = 0; i < num_actions; i++)
    {
      g_message ("[ACTION %d/%d]", i + 1, num_actions);
      action =
        (UndoableAction *) undo_stack_peek (self->undo_stack);
      if (i == 0)
        action->num_actions = 1;
      else if (i == num_actions - 1)
        action->num_actions = num_actions;

      GError * err = NULL;
      ret = do_or_undo_action (
        self, NULL, self->undo_stack, self->redo_stack, &err);
      if (ret != 0)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, "%s", _ ("Failed to undo action"));
          break;
        }
    }

  if (ZRYTHM_HAVE_UI)
    {
      EVENTS_PUSH (ET_UNDO_REDO_ACTION_DONE, NULL);

      /* process UI events now */
      event_manager_process_now (EVENT_MANAGER);
    }

  zix_sem_post (&self->action_sem);

  return ret;
}

/**
 * Redo last undone action.
 */
int
undo_manager_redo (UndoManager * self, GError ** error)
{
  g_warn_if_fail (!undo_stack_is_empty (self->redo_stack));

  zix_sem_wait (&self->action_sem);

  UndoableAction * action =
    (UndoableAction *) undo_stack_peek (self->redo_stack);
  g_return_val_if_fail (action, -1);
  const int num_actions = action->num_actions;
  g_return_val_if_fail (num_actions > 0, -1);

  int ret = 0;
  for (int i = 0; i < num_actions; i++)
    {
      g_message ("[ACTION %d/%d]", i + 1, num_actions);
      action =
        (UndoableAction *) undo_stack_peek (self->redo_stack);
      if (i == 0)
        action->num_actions = 1;
      else if (i == num_actions - 1)
        action->num_actions = num_actions;

      GError * err = NULL;
      ret = do_or_undo_action (
        self, NULL, self->redo_stack, self->undo_stack, &err);
      if (ret != 0)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, "%s", _ ("Failed to redo action"));
          break;
        }
    }

  if (ZRYTHM_HAVE_UI)
    {
      EVENTS_PUSH (ET_UNDO_REDO_ACTION_DONE, NULL);

      /* process UI events now */
      event_manager_process_now (EVENT_MANAGER);
    }

  zix_sem_post (&self->action_sem);

  return ret;
}

/**
 * Does performs the action and pushes it to the
 * undo stack, clearing the redo stack.
 *
 * @return Non-zero if error.
 */
int
undo_manager_perform (
  UndoManager *    self,
  UndoableAction * action,
  GError **        error)
{
  /* check that action is not already in the
   * stacks */
  g_return_val_if_fail (
    !undo_stack_contains_action (self->undo_stack, action)
      && !undo_stack_contains_action (self->redo_stack, action),
    -1);

  zix_sem_wait (&self->action_sem);

  /* if error return */
  GError * err = NULL;
  int      ret = do_or_undo_action (
    self, action, self->redo_stack, self->undo_stack, &err);
  if (ret != 0)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s", _ ("Failed to perform action"));
      zix_sem_post (&self->action_sem);
      return ret;
    }

  if (!self->redo_stack_locked)
    {
      undo_stack_clear (self->redo_stack, true);
    }

  if (ZRYTHM_HAVE_UI)
    {
      EVENTS_PUSH (ET_UNDO_REDO_ACTION_DONE, NULL);

      /* process UI events now */
      event_manager_process_now (EVENT_MANAGER);
    }

  zix_sem_post (&self->action_sem);

  return 0;
}

/**
 * Returns whether the given clip is used by any
 * stack.
 */
bool
undo_manager_contains_clip (UndoManager * self, AudioClip * clip)
{
  bool ret =
    undo_stack_contains_clip (self->undo_stack, clip)
    || undo_stack_contains_clip (self->redo_stack, clip);

  g_debug ("%s: %d", __func__, ret);

  return ret;
}

/**
 * Returns all plugins in the undo stacks.
 *
 * Used when cleaning up state dirs.
 */
NONNULL void
undo_manager_get_plugins (UndoManager * self, GPtrArray * arr)
{
  undo_stack_get_plugins (self->undo_stack, arr);
  undo_stack_get_plugins (self->redo_stack, arr);
}

/**
 * Returns the last performed action, or NULL if
 * the stack is empty.
 */
UndoableAction *
undo_manager_get_last_action (UndoManager * self)
{
  if (undo_stack_is_empty (self->undo_stack))
    return NULL;

  UndoableAction * action =
    (UndoableAction *) undo_stack_peek (self->undo_stack);
  return action;
}

/**
 * Clears the undo and redo stacks.
 */
void
undo_manager_clear_stacks (UndoManager * self, bool free)
{
  g_return_if_fail (self->undo_stack && self->redo_stack);
  undo_stack_clear (self->undo_stack, free);
  undo_stack_clear (self->redo_stack, free);
}

UndoManager *
undo_manager_clone (const UndoManager * src)
{
  UndoManager * self = object_new (UndoManager);
  self->schema_version = UNDO_MANAGER_SCHEMA_VERSION;

  self->undo_stack = undo_stack_clone (src->undo_stack);
  self->redo_stack = undo_stack_clone (src->redo_stack);

  zix_sem_init (&self->action_sem, 1);

  return self;
}

void
undo_manager_free (UndoManager * self)
{
  g_message ("%s: freeing...", __func__);

  object_free_w_func_and_null (
    undo_stack_free, self->undo_stack);
  object_free_w_func_and_null (
    undo_stack_free, self->redo_stack);

  zix_sem_destroy (&self->action_sem);

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
}
