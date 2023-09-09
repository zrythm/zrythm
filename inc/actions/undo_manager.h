// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Undo manager.
 */

#ifndef __UNDO_UNDO_MANAGER_H__
#define __UNDO_UNDO_MANAGER_H__

#include "actions/undo_stack.h"

#include "zix/sem.h"

typedef struct AudioClip AudioClip;

/**
 * @addtogroup actions
 *
 * @{
 */

#define UNDO_MANAGER_SCHEMA_VERSION 1

#define UNDO_MANAGER (PROJECT->undo_manager)

/**
 * Undo manager.
 */
typedef struct UndoManager
{
  int schema_version;

  UndoStack * undo_stack;
  UndoStack * redo_stack;

  /**
   * Whether the redo stack is currently locked.
   *
   * This is used as a hack when cancelling arranger
   * drags.
   */
  bool redo_stack_locked;

  /** Semaphore for performing actions. */
  ZixSem action_sem;
} UndoManager;

static const cyaml_schema_field_t undo_manager_fields_schema[] = {
  YAML_FIELD_INT (UndoManager, schema_version),
  YAML_FIELD_MAPPING_PTR (UndoManager, undo_stack, undo_stack_fields_schema),
  YAML_FIELD_MAPPING_PTR (UndoManager, redo_stack, undo_stack_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t undo_manager_schema = {
  YAML_VALUE_PTR (UndoManager, undo_manager_fields_schema),
};

/**
 * Inits the undo manager by populating the
 * undo/redo stacks.
 */
NONNULL void
undo_manager_init_loaded (UndoManager * self);

/**
 * Inits the undo manager by creating the undo/redo
 * stacks.
 */
WARN_UNUSED_RESULT UndoManager *
undo_manager_new (void);

/**
 * Undo last action.
 */
NONNULL_ARGS (1) int undo_manager_undo (UndoManager * self, GError ** error);

/**
 * Redo last undone action.
 */
NONNULL_ARGS (1) int undo_manager_redo (UndoManager * self, GError ** error);

/**
 * Performs the action and pushes it to the undo
 * stack.
 *
 * @return Non-zero if error.
 */
NONNULL_ARGS (1, 2)
int undo_manager_perform (
  UndoManager *    self,
  UndoableAction * action,
  GError **        error);

/**
 * Second and last argument given must be a
 * GError **.
 */
#define UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR(action, err, ...) \
  { \
    g_return_val_if_fail ( \
      router_is_processing_thread (ROUTER) == false, false); \
    UndoableAction * ua = action (__VA_ARGS__); \
    if (ua) \
      { \
        int ret = undo_manager_perform (UNDO_MANAGER, ua, err); \
        if (ret == 0) \
          return true; \
      } \
    return false; \
  }

/**
 * Returns whether the given clip is used by any
 * stack.
 */
NONNULL bool
undo_manager_contains_clip (UndoManager * self, AudioClip * clip);

/**
 * Returns all plugins in the undo stacks.
 *
 * Used when cleaning up state dirs.
 */
NONNULL void
undo_manager_get_plugins (UndoManager * self, GPtrArray * arr);

/**
 * Returns the last performed action, or NULL if
 * the stack is empty.
 */
NONNULL UndoableAction *
undo_manager_get_last_action (UndoManager * self);

/**
 * Clears the undo and redo stacks.
 */
NONNULL void
undo_manager_clear_stacks (UndoManager * self, bool free);

NONNULL UndoManager *
undo_manager_clone (const UndoManager * src);

NONNULL void
undo_manager_free (UndoManager * self);

/**
 * @}
 */

#endif
