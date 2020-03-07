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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/undo_stack.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/stack.h"
#include "zrythm.h"

UndoStack *
undo_stack_new (void)
{
  g_return_val_if_fail (
    ZRYTHM_TESTING ||
      G_IS_SETTINGS (S_PREFERENCES), NULL);

  UndoStack * self = calloc (1, sizeof (UndoStack));

  int undo_stack_length =
    ZRYTHM_TESTING ? 64 :
    g_settings_get_int (
      S_PREFERENCES, "undo-stack-length");
  self->stack =
    stack_new (undo_stack_length);
  self->stack->top = -1;

  return self;
}

void
undo_stack_free (
  UndoStack * self)
{
}

void
undo_stack_push (
  UndoStack *      self,
  UndoableAction * action)
{
  /* push to stack */
  STACK_PUSH (self->stack, action);

  /* remember the action */

  /* CAPS, CamelCase, snake_case */
#define APPEND_ELEMENT(caps,cc,sc) \
  case UA_##caps: \
    array_double_size_if_full ( \
      self->sc##_actions, \
      self->num_##sc##_actions, \
      self->sc##_actions_size, \
      cc##Action *); \
    array_append ( \
      self->sc##_actions, \
      self->num_##sc##_actions, \
      (cc##Action *) action); \
    break


  switch (action->type)
    {
    APPEND_ELEMENT (
      CREATE_TRACKS, CreateTracks, create_tracks);
    APPEND_ELEMENT (
      MOVE_TRACKS, MoveTracks, move_tracks);
    APPEND_ELEMENT (
      EDIT_TRACKS, EditTracks, edit_tracks);
    APPEND_ELEMENT (
      COPY_TRACKS, CopyTracks, copy_tracks);
    APPEND_ELEMENT (
      DELETE_TRACKS, DeleteTracks, delete_tracks);
    APPEND_ELEMENT (
      CREATE_PLUGINS, CreatePlugins, create_plugins);
    APPEND_ELEMENT (
      MOVE_PLUGINS, MovePlugins, move_plugins);
    APPEND_ELEMENT (
      EDIT_PLUGINS, EditPlugins, edit_plugins);
    APPEND_ELEMENT (
      COPY_PLUGINS, CopyPlugins, copy_plugins);
    APPEND_ELEMENT (
      DELETE_PLUGINS, DeletePlugins, delete_plugins);
    case UA_CREATE_ARRANGER_SELECTIONS:
    case UA_MOVE_ARRANGER_SELECTIONS:
    case UA_RESIZE_ARRANGER_SELECTIONS:
    case UA_SPLIT_ARRANGER_SELECTIONS:
    case UA_EDIT_ARRANGER_SELECTIONS:
    case UA_DUPLICATE_ARRANGER_SELECTIONS:
    case UA_DELETE_ARRANGER_SELECTIONS:
    case UA_QUANTIZE_ARRANGER_SELECTIONS:
      array_double_size_if_full (
        self->as_actions,
        self->num_as_actions,
        self->as_actions_size,
        ArrangerSelectionsAction *);
      array_append (
        self->as_actions,
        self->num_as_actions,
        (ArrangerSelectionsAction *) action);
      break;
    }
}

static void
remove_action (
  UndoStack *      self,
  UndoableAction * action)
{
  /* CAPS, CamelCase, snake_case */
#define REMOVE_ELEMENT(caps,cc,sc) \
  case UA_##caps: \
    array_delete ( \
      self->sc##_actions, \
      self->num_##sc##_actions, \
      (cc##Action *) action); \
    break


  switch (action->type)
    {
    REMOVE_ELEMENT (
      CREATE_TRACKS, CreateTracks, create_tracks);
    REMOVE_ELEMENT (
      MOVE_TRACKS, MoveTracks, move_tracks);
    REMOVE_ELEMENT (
      EDIT_TRACKS, EditTracks, edit_tracks);
    REMOVE_ELEMENT (
      COPY_TRACKS, CopyTracks, copy_tracks);
    REMOVE_ELEMENT (
      DELETE_TRACKS, DeleteTracks, delete_tracks);
    REMOVE_ELEMENT (
      CREATE_PLUGINS, CreatePlugins, create_plugins);
    REMOVE_ELEMENT (
      MOVE_PLUGINS, MovePlugins, move_plugins);
    REMOVE_ELEMENT (
      EDIT_PLUGINS, EditPlugins, edit_plugins);
    REMOVE_ELEMENT (
      COPY_PLUGINS, CopyPlugins, copy_plugins);
    REMOVE_ELEMENT (
      DELETE_PLUGINS, DeletePlugins, delete_plugins);
    case UA_CREATE_ARRANGER_SELECTIONS:
    case UA_MOVE_ARRANGER_SELECTIONS:
    case UA_RESIZE_ARRANGER_SELECTIONS:
    case UA_SPLIT_ARRANGER_SELECTIONS:
    case UA_EDIT_ARRANGER_SELECTIONS:
    case UA_DUPLICATE_ARRANGER_SELECTIONS:
    case UA_DELETE_ARRANGER_SELECTIONS:
    case UA_QUANTIZE_ARRANGER_SELECTIONS:
      array_delete (
        self->as_actions,
        self->num_as_actions,
        (ArrangerSelectionsAction *) action);
      break;
    }
}

UndoableAction *
undo_stack_pop (
  UndoStack * self)
{
  /* pop from stack */
  UndoableAction * action =
    (UndoableAction *) stack_pop (self->stack);

  /* remove the action */
  remove_action (self, action);

  /* return it */
  return action;
}

/**
 * Pops the last element and moves everything back.
 */
UndoableAction *
undo_stack_pop_last (
  UndoStack * self)
{
  /* pop from stack */
  UndoableAction * action =
    (UndoableAction *) stack_pop_last (self->stack);

  /* remove the action */
  remove_action (self, action);

  /* return it */
  return action;
}
