/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __UNDO_MOVE_PLUGINS_ACTION_H__
#define __UNDO_MOVE_PLUGINS_ACTION_H__

#include "audio/channel.h"
#include "actions/undoable_action.h"
#include "gui/backend/mixer_selections.h"
#include "utils/yaml.h"

typedef struct Plugin Plugin;
typedef struct Track Track;
typedef struct MixerSelections MixerSelections;

/**
 * Restrict selections to a channel.
 */
typedef struct MovePluginsAction
{
  UndoableAction  parent_instance;

  /** Type of starting slot to move plugins to. */
  PluginSlotType  slot_type;

  /** To slot.
   *
   * The rest of the slots will start from this so
   * they can be calculated when doing/undoing. */
  int             to_slot;

  /** To track position. */
  int             to_track_pos;

  /** From track position. */
  int             from_track_pos;

  int             is_new_channel;

  /** MixerSelections clone containing all the
   * necessary information.
   */
  MixerSelections * ms;
} MovePluginsAction;

static const cyaml_schema_field_t
  move_plugins_action_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "parent_instance", CYAML_FLAG_DEFAULT,
    MovePluginsAction, parent_instance,
    undoable_action_fields_schema),
  YAML_FIELD_ENUM (
    MovePluginsAction, slot_type,
    plugin_slot_type_strings),
  YAML_FIELD_INT (
    MovePluginsAction, to_slot),
  YAML_FIELD_INT (
    MovePluginsAction, to_track_pos),
  YAML_FIELD_INT (
    MovePluginsAction, from_track_pos),
  YAML_FIELD_INT (
    MovePluginsAction, is_new_channel),
  YAML_FIELD_MAPPING_PTR (
    MovePluginsAction, ms,
    mixer_selections_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  move_plugins_action_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER, MovePluginsAction,
    move_plugins_action_fields_schema),
};

void
move_plugins_action_init_loaded (
  MovePluginsAction * self);

/**
 * Create a new action.
 *
 * @param slot_type Slot type to copy to.
 */
UndoableAction *
move_plugins_action_new (
  MixerSelections * ms,
  PluginSlotType   slot_type,
  Track *    to_tr,
  int        to_slot);

int
move_plugins_action_do (
  MovePluginsAction * self);

int
move_plugins_action_undo (
  MovePluginsAction * self);

char *
move_plugins_action_stringize (
  MovePluginsAction * self);

void
move_plugins_action_free (
  MovePluginsAction * self);

#endif
