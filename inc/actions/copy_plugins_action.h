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

#ifndef __UNDO_COPY_PLUGINS_ACTION_H__
#define __UNDO_COPY_PLUGINS_ACTION_H__

#include "actions/undoable_action.h"
#include "gui/backend/mixer_selections.h"
#include "plugins/plugin.h"

/**
 * @addtogroup actions
 *
 * @{
 */

typedef struct Plugin Plugin;
typedef struct Channel Channel;
typedef struct MixerSelections MixerSelections;

typedef struct CopyPluginsAction
{
  UndoableAction  parent_instance;

  /** Type of starting slot to copy plugins to. */
  PluginSlotType  slot_type;

  /** Starting slot to copy plugins to. */
  int              slot;

  /** Whether the plugin will be copied into a new
   * Channel or not. */
  int              is_new_channel;

  /** Track position to copy to, if existing, or
   * track position copied to, when undoing. */
  int              track_pos;

  /** Clones of the original Plugins.
   *
   * These must not be used as is. They must be
   * cloned.
   *
   * When doing, their IDs should be set to the newly
   * copied plugins, so they can be found when
   * undoing.
   */
  MixerSelections * ms;
} CopyPluginsAction;

static const cyaml_schema_field_t
  copy_plugins_action_fields_schema[] =
{
  YAML_FIELD_MAPPING_EMBEDDED (
    CopyPluginsAction, parent_instance,
    undoable_action_fields_schema),
  YAML_FIELD_ENUM (
    CopyPluginsAction, slot_type,
    plugin_slot_type_strings),
  YAML_FIELD_INT (
    CopyPluginsAction, slot),
  YAML_FIELD_INT (
    CopyPluginsAction, is_new_channel),
  YAML_FIELD_INT (
    CopyPluginsAction, track_pos),
  YAML_FIELD_MAPPING_PTR (
    CopyPluginsAction, ms,
    mixer_selections_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  copy_plugins_action_schema =
{
  YAML_VALUE_PTR (
    CopyPluginsAction,
    copy_plugins_action_fields_schema),
};

void
copy_plugins_action_init_loaded (
  CopyPluginsAction * self);

/**
 * Create a new CopyPluginsAction.
 *
 * @param slot Starting slot to copy plugins to.
 * @param tr Track to copy to.
 * @param slot_type Slot type to copy to.
 */
UndoableAction *
copy_plugins_action_new (
  MixerSelections * ms,
  PluginSlotType   slot_type,
  Track *           tr,
  int               slot);

int
copy_plugins_action_do (
  CopyPluginsAction * self);

int
copy_plugins_action_undo (
  CopyPluginsAction * self);

char *
copy_plugins_action_stringize (
  CopyPluginsAction * self);

void
copy_plugins_action_free (
  CopyPluginsAction * self);

/**
 * @}
 */

#endif
