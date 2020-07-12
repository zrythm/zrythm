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
#include "utils/objects.h"
#include "utils/stack.h"
#include "zrythm.h"
#include "zrythm_app.h"

void
undo_stack_init_loaded (
  UndoStack * self)
{
  /* use the remembered max length instead of the
   * one in settings so they match */
  int undo_stack_length = self->stack->max_length;
  stack_free (self->stack);
  self->stack =
    stack_new (undo_stack_length);
  self->stack->top = -1;

#define DO_SIMPLE(cc,sc) \
  /* if there are still actions of this type */ \
  if (sc##_actions_idx < self->num_##sc##_actions) \
    { \
      cc##Action * a = \
        self->sc##_actions[sc##_actions_idx]; \
      UndoableAction * ua = \
        (UndoableAction *) a; \
      undoable_action_init_loaded (ua); \
      if (self->stack->top + 1 == ua->stack_idx) \
        { \
          STACK_PUSH (self->stack, ua); \
          sc##_actions_idx++; \
        } \
    }

  size_t as_actions_idx = 0;
  size_t copy_plugins_actions_idx = 0;
  size_t copy_tracks_actions_idx = 0;
  size_t create_plugins_actions_idx = 0;
  size_t create_tracks_actions_idx = 0;
  size_t delete_plugins_actions_idx = 0;
  size_t delete_tracks_actions_idx = 0;
  size_t channel_send_actions_idx = 0;
  size_t edit_plugins_actions_idx = 0;
  size_t edit_tracks_actions_idx = 0;
  size_t move_plugins_actions_idx = 0;
  size_t move_tracks_actions_idx = 0;
  size_t port_connection_actions_idx = 0;
  size_t transport_actions_idx = 0;

  size_t total_actions =
    self->num_as_actions +
    self->num_copy_plugins_actions +
    self->num_copy_tracks_actions +
    self->num_create_plugins_actions +
    self->num_create_tracks_actions +
    self->num_delete_plugins_actions +
    self->num_delete_tracks_actions +
    self->num_channel_send_actions +
    self->num_edit_plugins_actions +
    self->num_edit_tracks_actions +
    self->num_move_plugins_actions +
    self->num_move_tracks_actions +
    self->num_port_connection_actions +
    self->num_transport_actions;

  size_t i = 0;
  while (i < total_actions)
    {
      DO_SIMPLE (ArrangerSelections, as)
      DO_SIMPLE (CopyPlugins, copy_plugins)
      DO_SIMPLE (CopyTracks, copy_tracks)
      DO_SIMPLE (CreatePlugins, create_plugins)
      DO_SIMPLE (CreateTracks, create_tracks)
      DO_SIMPLE (DeletePlugins, delete_plugins)
      DO_SIMPLE (DeleteTracks, delete_tracks)
      DO_SIMPLE (ChannelSend, channel_send)
      DO_SIMPLE (EditPlugins, edit_plugins)
      DO_SIMPLE (EditTracks, edit_tracks)
      DO_SIMPLE (MovePlugins, move_plugins)
      DO_SIMPLE (MoveTracks, move_tracks)
      DO_SIMPLE (PortConnection, port_connection)
      DO_SIMPLE (Transport, transport)

      i++;
    }
}

UndoStack *
undo_stack_new (void)
{
  g_return_val_if_fail (
    ZRYTHM_TESTING ||
      G_IS_SETTINGS (S_P_EDITING_UNDO), NULL);

  UndoStack * self = calloc (1, sizeof (UndoStack));

  int undo_stack_length =
    ZRYTHM_TESTING ? 64 :
    g_settings_get_int (
      S_P_EDITING_UNDO, "undo-stack-length");
  self->stack =
    stack_new (undo_stack_length);
  self->stack->top = -1;

  return self;
}

void
undo_stack_free (
  UndoStack * self)
{
  g_message ("%s: freeing...", __func__);

  while (!undo_stack_is_empty (self))
    {
      UndoableAction * ua = undo_stack_pop (self);
      undoable_action_free (ua);
    }

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
}

void
undo_stack_push (
  UndoStack *      self,
  UndoableAction * action)
{
  g_message ("pushed to undo/redo stack");

  /* push to stack */
  STACK_PUSH (self->stack, action);

  action->stack_idx = self->stack->top;

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
      CHANNEL_SEND, ChannelSend, channel_send);
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
    APPEND_ELEMENT (
      PORT_CONNECTION, PortConnection,
      port_connection);
    APPEND_ELEMENT (
      TRANSPORT, Transport, transport);
    case UA_CREATE_ARRANGER_SELECTIONS:
    case UA_MOVE_ARRANGER_SELECTIONS:
    case UA_LINK_ARRANGER_SELECTIONS:
    case UA_RECORD_ARRANGER_SELECTIONS:
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

static int
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
    removed = 1; \
    break

  int removed = 0;
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
      CHANNEL_SEND, ChannelSend, channel_send);
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
    REMOVE_ELEMENT (
      PORT_CONNECTION, PortConnection,
      port_connection);
    REMOVE_ELEMENT (
      TRANSPORT, Transport, transport);
    case UA_CREATE_ARRANGER_SELECTIONS:
    case UA_MOVE_ARRANGER_SELECTIONS:
    case UA_LINK_ARRANGER_SELECTIONS:
    case UA_RECORD_ARRANGER_SELECTIONS:
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
      removed = 1;
      g_warn_if_fail (
        (int) self->num_as_actions <=
          self->stack->top + 1);
      break;
    }

  return removed;
}

UndoableAction *
undo_stack_pop (
  UndoStack * self)
{
  /* pop from stack */
  UndoableAction * action =
    (UndoableAction *) stack_pop (self->stack);

  /* remove the action */
  int removed = remove_action (self, action);
  g_warn_if_fail (removed);

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
  int removed = remove_action (self, action);
  g_warn_if_fail (removed);

  /* return it */
  return action;
}

/**
 * Clears the stack, optionally freeing all the
 * elements.
 */
void
undo_stack_clear (
  UndoStack * self,
  bool        free)
{
  while (!undo_stack_is_empty (self))
    {
      UndoableAction * ua = undo_stack_pop (self);
      if (free)
        {
          undoable_action_free (ua);
        }
    }
}
