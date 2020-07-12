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

#ifndef __UNDO_EDIT_PLUGINS_ACTION_H__
#define __UNDO_EDIT_PLUGINS_ACTION_H__

#include "actions/undoable_action.h"
#include "gui/backend/mixer_selections.h"
#include "plugins/plugin.h"

/**
 * @addtogroup actions
 *
 * @{
 */

typedef enum EditPluginsActionType
{
  EDIT_PLUGINS_ACTION_TYPE_CHANGE_PARAM,
} EditPluginsActionType;

static const cyaml_strval_t
  edit_plugins_action_type_strings[] =
{
  { "change param",
    EDIT_PLUGINS_ACTION_TYPE_CHANGE_PARAM    },
};

typedef struct Plugin Plugin;
typedef struct Channel Channel;
typedef struct MixerSelections MixerSelections;

/* TODO */
typedef struct EditPluginsAction
{
  UndoableAction  parent_instance;

  /**
   * The plugins to operate on.
   *
   * These are clones and should not be used in the
   * project.
   */
  MixerSelections * ms;

  /** Action type. */
  EditPluginsActionType type;

} EditPluginsAction;

static const cyaml_schema_field_t
  edit_plugins_action_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "parent_instance", CYAML_FLAG_DEFAULT,
    EditPluginsAction, parent_instance,
    undoable_action_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "ms", CYAML_FLAG_POINTER,
    EditPluginsAction, ms,
    mixer_selections_fields_schema),
  CYAML_FIELD_ENUM (
    "type", CYAML_FLAG_DEFAULT,
    EditPluginsAction, type,
    edit_plugins_action_type_strings,
    CYAML_ARRAY_LEN (
      edit_plugins_action_type_strings)),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  edit_plugins_action_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER, EditPluginsAction,
    edit_plugins_action_fields_schema),
};

void
edit_plugins_action_init_loaded (
  EditPluginsAction * self);

UndoableAction *
edit_plugins_action_new (
  MixerSelections *     ms,
  EditPluginsActionType type);

int
edit_plugins_action_do (
  EditPluginsAction * self);

int
edit_plugins_action_undo (
  EditPluginsAction * self);

char *
edit_plugins_action_stringize (
  EditPluginsAction * self);

void
edit_plugins_action_free (
  EditPluginsAction * self);

/**
 * @}
 */

#endif
