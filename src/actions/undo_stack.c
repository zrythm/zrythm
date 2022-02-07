/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/stack.h"
#include "zrythm.h"
#include "zrythm_app.h"

NONNULL
size_t
undo_stack_get_total_cached_actions (
  UndoStack * self)
{
  size_t total =
    self->num_as_actions +
    self->num_mixer_selections_actions +
    self->num_tracklist_selections_actions +
    self->num_channel_send_actions +
    self->num_midi_mapping_actions +
    self->num_port_actions +
    self->num_port_connection_actions +
    self->num_range_actions +
    self->num_transport_actions +
    self->num_chord_actions;

  if ((int) total > self->stack->max_length)
    {
      g_critical (
        "total stack members (%zu) higher than "
        "max length (%d)",
        total, self->stack->max_length);
    }

  return total;
}

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

  size_t as_actions_idx = 0;
  size_t mixer_selections_actions_idx = 0;
  size_t tracklist_selections_actions_idx = 0;
  size_t channel_send_actions_idx = 0;
  size_t port_actions_idx = 0;
  size_t port_connection_actions_idx = 0;
  size_t midi_mapping_actions_idx = 0;
  size_t range_actions_idx = 0;
  size_t transport_actions_idx = 0;
  size_t chord_actions_idx = 0;

  size_t total_actions =
    undo_stack_get_total_cached_actions (self);

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

  for (size_t i = 0; i < total_actions; i++)
    {
      DO_SIMPLE (ArrangerSelections, as)
      DO_SIMPLE (TracklistSelections, tracklist_selections)
      DO_SIMPLE (ChannelSend, channel_send)
      DO_SIMPLE (MixerSelections, mixer_selections)
      DO_SIMPLE (PortConnection, port_connection)
      DO_SIMPLE (Port, port)
      DO_SIMPLE (MidiMapping, midi_mapping)
      DO_SIMPLE (Range, range)
      DO_SIMPLE (Transport, transport)
      DO_SIMPLE (Chord, chord)
    }

  g_return_if_fail (
    self->stack->top + 1 ==
      (int)
      undo_stack_get_total_cached_actions (self));
}

UndoStack *
undo_stack_new (void)
{
  g_return_val_if_fail (
    (ZRYTHM && ZRYTHM->testing) ||
      G_IS_SETTINGS (S_P_EDITING_UNDO), NULL);

  UndoStack * self = object_new (UndoStack);
  self->schema_version = UNDO_STACK_SCHEMA_VERSION;

  int undo_stack_length =
    ZRYTHM_TESTING ?
      ZRYTHM->undo_stack_len :
      g_settings_get_int (
        S_P_EDITING_UNDO, "undo-stack-length");
  self->stack =
    stack_new (undo_stack_length);
  self->stack->top = -1;

  return self;
}

UndoStack *
undo_stack_clone (
  const UndoStack * src)
{
  UndoStack * self = object_new (UndoStack);
  self->schema_version = UNDO_STACK_SCHEMA_VERSION;

  int top_before = src->stack->top;

  self->stack =
    stack_new (src->stack->max_length);
  self->stack->top = -1;

  /* clone all actions */
#define CLONE_ACTIONS(arr,fn,type) \
  self->arr = \
    object_new_n (src->num_##arr, type *); \
  for (size_t i = 0; i < src->num_##arr; i++) \
    { \
      self->arr[i] = fn##_clone (src->arr[i]); \
      g_return_val_if_fail (self->arr[i], NULL); \
    } \
  self->num_##arr = src->num_##arr

  CLONE_ACTIONS (
    as_actions, arranger_selections_action,
    ArrangerSelectionsAction);
  CLONE_ACTIONS (
    mixer_selections_actions,
    mixer_selections_action,
    MixerSelectionsAction);
  CLONE_ACTIONS (
    tracklist_selections_actions,
    tracklist_selections_action,
    TracklistSelectionsAction);
  CLONE_ACTIONS (
    channel_send_actions, channel_send_action,
    ChannelSendAction);
  CLONE_ACTIONS (
    port_connection_actions, port_connection_action,
    PortConnectionAction);
  CLONE_ACTIONS (
    port_actions, port_action,
    PortAction);
  CLONE_ACTIONS (
    midi_mapping_actions, midi_mapping_action,
    MidiMappingAction);
  CLONE_ACTIONS (
    range_actions, range_action, RangeAction);
  CLONE_ACTIONS (
    transport_actions, transport_action,
    TransportAction);
  CLONE_ACTIONS (
    chord_actions, chord_action, ChordAction);

#undef CLONE_ACTIONS

  /* init loaded to load the actions on the stack */
  undo_stack_init_loaded (self);

  g_return_val_if_fail (
    top_before == self->stack->top, NULL);

  return self;
}

/**
 * Gets the list of actions as a string.
 */
char *
undo_stack_get_as_string (
  UndoStack * self,
  int         limit)
{
  GString * g_str = g_string_new (NULL);

  Stack * stack = self->stack;
  for (int i = stack->top; i >= 0; i--)
    {
      UndoableAction * action =
        (UndoableAction *) stack->elements[i];

      char * action_str =
        undoable_action_to_string (action);
      g_string_append_printf (
        g_str, "[%d] %s\n",
        stack->top - i, action_str);
      g_free (action_str);

      if (stack->top - i == limit)
        break;
    }

  return g_string_free (g_str, false);
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
      TRACKLIST_SELECTIONS, TracklistSelections,
      tracklist_selections);
    APPEND_ELEMENT (
      CHANNEL_SEND, ChannelSend, channel_send);
    APPEND_ELEMENT (
      MIXER_SELECTIONS, MixerSelections,
      mixer_selections);
    APPEND_ELEMENT (
      PORT_CONNECTION, PortConnection,
      port_connection);
    APPEND_ELEMENT (PORT, Port, port);
    APPEND_ELEMENT (
      MIDI_MAPPING, MidiMapping,
      midi_mapping);
    APPEND_ELEMENT (
      RANGE, Range, range);
    APPEND_ELEMENT (
      TRANSPORT, Transport, transport);
    APPEND_ELEMENT (
      CHORD, Chord, chord);
    APPEND_ELEMENT (
      ARRANGER_SELECTIONS, ArrangerSelections, as);
    }
}

static bool
remove_action (
  UndoStack *      self,
  UndoableAction * action)
{
  /* CAPS, CamelCase, snake_case */
#define REMOVE_ELEMENT(caps,cc,sc) \
  case UA_##caps: \
    array_delete_confirm ( \
      self->sc##_actions, \
      self->num_##sc##_actions, \
      (cc##Action *) action, \
      removed); \
    g_return_val_if_fail (removed, removed); \
    if ((int) self->num_##sc##_actions > \
          g_atomic_int_get ( \
            &self->stack->top) + 1) \
      { \
        g_critical ( \
          "num " #sc " actions (%zu) is greater " \
          "than current stack top (%d)", \
          self->num_##sc##_actions, \
          g_atomic_int_get ( \
            &self->stack->top) + 1); \
        return removed; \
      } \
    break

  bool removed = false;
  switch (action->type)
    {
    REMOVE_ELEMENT (
      TRACKLIST_SELECTIONS, TracklistSelections,
      tracklist_selections);
    REMOVE_ELEMENT (
      CHANNEL_SEND, ChannelSend, channel_send);
    REMOVE_ELEMENT (
      MIXER_SELECTIONS, MixerSelections,
      mixer_selections);
    REMOVE_ELEMENT (
      PORT_CONNECTION, PortConnection,
      port_connection);
    REMOVE_ELEMENT (PORT, Port, port);
    REMOVE_ELEMENT (
      MIDI_MAPPING, MidiMapping,
      midi_mapping);
    REMOVE_ELEMENT (
      RANGE, Range, range);
    REMOVE_ELEMENT (
      TRANSPORT, Transport, transport);
    REMOVE_ELEMENT (CHORD, Chord, chord);
    case UA_ARRANGER_SELECTIONS:
      array_delete_confirm (
        self->as_actions,
        self->num_as_actions,
        (ArrangerSelectionsAction *) action,
        removed);
      g_return_val_if_fail (
        (int) self->num_as_actions <=
          g_atomic_int_get (&self->stack->top) + 1,
        removed);
      break;
    }

  /* re-set the indices */
  for (int i = 0;
       i <= g_atomic_int_get (&self->stack->top);
       i++)
    {
      UndoableAction * ua =
        (UndoableAction *)
        self->stack->elements[i];
      ua->stack_idx = i;
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
  g_return_val_if_fail (removed, action);

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
  g_message (
    "<undo stack> popping last (top = %d)",
    g_atomic_int_get (&self->stack->top));

  /* pop from stack */
  UndoableAction * action =
    (UndoableAction *) stack_pop_last (self->stack);

  /* remove the action */
  bool removed = remove_action (self, action);
  g_return_val_if_fail (removed, action);

  /* return it */
  return action;
}

bool
undo_stack_contains_clip (
  UndoStack * self,
  AudioClip * clip)
{
  for (int i = 0; i <= self->stack->top; i++)
    {
      UndoableAction * ua =
        (UndoableAction *)
        self->stack->elements[i];

      if (undoable_action_contains_clip (ua, clip))
        return true;
    }

  return false;
}

/**
 * Checks if the undo stack contains the given
 * action pointer.
 */
bool
undo_stack_contains_action (
  UndoStack *      self,
  UndoableAction * ua)
{
  for (int i = 0; i <= self->stack->top; i++)
    {
      UndoableAction * action =
        (UndoableAction *)
        self->stack->elements[i];
      if (ua == action)
        return true;
    }

  return false;
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

void
undo_stack_free (
  UndoStack * self)
{
  g_message ("%s: freeing...", __func__);

  while (!undo_stack_is_empty (self))
    {
      UndoableAction * ua = undo_stack_pop (self);
      char * type_str =
        undoable_action_to_string (ua);
      g_debug (
        "%s: freeing %s", __func__, type_str);
      g_free (type_str);
      undoable_action_free (ua);
    }

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
}
