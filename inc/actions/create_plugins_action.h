/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __UNDO_CREATE_PLUGINS_ACTION_H__
#define __UNDO_CREATE_PLUGINS_ACTION_H__

#include "audio/channel.h"
#include "actions/undoable_action.h"
#include "plugins/plugin.h"

/**
 * @addtogroup actions
 *
 * @{
 */

typedef struct Plugin Plugin;
typedef struct Channel Channel;

typedef struct CreatePluginsAction
{
  UndoableAction  parent_instance;

  /** Type of slot to create to. */
  PluginSlotType  slot_type;

  /** Slot to create to.
   *
   * Also used for undoing. */
  int              slot;

  /** Position of track to create to. */
  int              track_pos;

  int              num_plugins;

  /**
   * PluginDescriptor to use when creating.
   */
  PluginDescriptor descr;
} CreatePluginsAction;

static const cyaml_schema_field_t
  create_plugins_action_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "parent_instance", CYAML_FLAG_DEFAULT,
    CreatePluginsAction, parent_instance,
    undoable_action_fields_schema),
  YAML_FIELD_ENUM (
    CreatePluginsAction, slot_type,
    plugin_slot_type_strings),
  YAML_FIELD_INT (
    CreatePluginsAction, slot),
  YAML_FIELD_INT (
    CreatePluginsAction, track_pos),
#if 0
  CYAML_FIELD_SEQUENCE_COUNT (
    "plugins", CYAML_FLAG_DEFAULT,
    CreatePluginsAction, plugins, num_plugins,
    &plugin_schema, 0, CYAML_UNLIMITED),
#endif
  YAML_FIELD_MAPPING_EMBEDDED (
    CreatePluginsAction, descr,
    plugin_descriptor_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  create_plugins_action_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER, CreatePluginsAction,
    create_plugins_action_fields_schema),
};

UndoableAction *
create_plugins_action_new (
  const PluginDescriptor * descr,
  PluginSlotType  slot_type,
  int       track_pos,
  int       slot,
  int       num_plugins);

int
create_plugins_action_do (
	CreatePluginsAction * self);

int
create_plugins_action_undo (
	CreatePluginsAction * self);

char *
create_plugins_action_stringize (
	CreatePluginsAction * self);

void
create_plugins_action_free (
	CreatePluginsAction * self);

/**
 * @}
 */

#endif
