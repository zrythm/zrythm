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

/**
 * \file
 *
 * Undo stack.
 */

#ifndef __UNDO_UNDO_STACK_H__
#define __UNDO_UNDO_STACK_H__

#include "actions/arranger_selections.h"
#include "actions/channel_send_action.h"
#include "actions/copy_plugins_action.h"
#include "actions/copy_tracks_action.h"
#include "actions/create_plugins_action.h"
#include "actions/create_tracks_action.h"
#include "actions/delete_plugins_action.h"
#include "actions/delete_tracks_action.h"
#include "actions/edit_plugins_action.h"
#include "actions/edit_tracks_action.h"
#include "actions/move_plugins_action.h"
#include "actions/move_tracks_action.h"
#include "actions/port_connection_action.h"
#include "actions/transport_action.h"
#include "utils/stack.h"
#include "utils/yaml.h"

typedef struct Stack Stack;

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * Serializable stack for undoable actions.
 *
 * This is used for both undo and redo.
 */
typedef struct UndoStack
{
  /** Actual stack used at runtime. */
  Stack *       stack;

  /* the following are for serialization
   * purposes only */

  ArrangerSelectionsAction ** as_actions;
  size_t        num_as_actions;
  size_t        as_actions_size;

  CopyPluginsAction ** copy_plugins_actions;
  size_t        num_copy_plugins_actions;
  size_t        copy_plugins_actions_size;

  CopyTracksAction ** copy_tracks_actions;
  size_t        num_copy_tracks_actions;
  size_t        copy_tracks_actions_size;

  CreatePluginsAction ** create_plugins_actions;
  size_t        num_create_plugins_actions;
  size_t        create_plugins_actions_size;

  CreateTracksAction ** create_tracks_actions;
  size_t        num_create_tracks_actions;
  size_t        create_tracks_actions_size;

  DeletePluginsAction ** delete_plugins_actions;
  size_t        num_delete_plugins_actions;
  size_t        delete_plugins_actions_size;

  DeleteTracksAction ** delete_tracks_actions;
  size_t        num_delete_tracks_actions;
  size_t        delete_tracks_actions_size;

  ChannelSendAction ** channel_send_actions;
  size_t        num_channel_send_actions;
  size_t        channel_send_actions_size;

  EditPluginsAction ** edit_plugins_actions;
  size_t        num_edit_plugins_actions;
  size_t        edit_plugins_actions_size;

  EditTracksAction ** edit_tracks_actions;
  size_t        num_edit_tracks_actions;
  size_t        edit_tracks_actions_size;

  MovePluginsAction ** move_plugins_actions;
  size_t        num_move_plugins_actions;
  size_t        move_plugins_actions_size;

  MoveTracksAction ** move_tracks_actions;
  size_t        num_move_tracks_actions;
  size_t        move_tracks_actions_size;

  PortConnectionAction ** port_connection_actions;
  size_t        num_port_connection_actions;
  size_t        port_connection_actions_size;

  TransportAction ** transport_actions;
  size_t        num_transport_actions;
  size_t        transport_actions_size;

} UndoStack;

static const cyaml_schema_field_t
  undo_stack_fields_schema[] =
{
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack, as_actions,
    arranger_selections_action_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack, copy_plugins_actions,
    copy_plugins_action_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack, copy_tracks_actions,
    copy_tracks_action_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack, create_plugins_actions,
    create_plugins_action_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack, create_tracks_actions,
    create_tracks_action_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack, delete_plugins_actions,
    delete_plugins_action_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack, delete_tracks_actions,
    delete_tracks_action_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack, channel_send_actions,
    channel_send_action_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack, edit_plugins_actions,
    edit_plugins_action_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack, edit_tracks_actions,
    edit_tracks_action_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack, move_plugins_actions,
    move_plugins_action_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack, move_tracks_actions,
    move_tracks_action_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack, port_connection_actions,
    port_connection_action_schema),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    UndoStack, transport_actions,
    transport_action_schema),
  YAML_FIELD_MAPPING_PTR (
    UndoStack, stack, stack_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  undo_stack_schema =
{
  YAML_VALUE_PTR (
    UndoStack, undo_stack_fields_schema),
};

void
undo_stack_init_loaded (
  UndoStack * self);

/**
 * Creates a new stack for undoable actions.
 */
UndoStack *
undo_stack_new (void);

void
undo_stack_free (
  UndoStack * self);

/* --- start wrappers --- */

#define undo_stack_size(x) \
  (stack_size ((x)->stack))

#define undo_stack_is_empty(x) \
  (stack_is_empty ((x)->stack))

#define undo_stack_is_full(x) \
  (stack_is_full ((x)->stack))

#define undo_stack_peek(x) \
  (stack_peek ((x)->stack))

#define undo_stack_peek_last(x) \
  (stack_peek_last ((x)->stack))

void
undo_stack_push (
  UndoStack *      self,
  UndoableAction * action);

UndoableAction *
undo_stack_pop (
  UndoStack * self);

/**
 * Pops the last element and moves everything back.
 */
UndoableAction *
undo_stack_pop_last (
  UndoStack * self);

/* --- end wrappers --- */

/**
 * Clears the stack, optionally freeing all the
 * elements.
 */
void
undo_stack_clear (
  UndoStack * self,
  bool        free);

/**
 * @}
 */

#endif
