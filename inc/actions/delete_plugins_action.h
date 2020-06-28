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

#ifndef __UNDO_DELETE_PLUGINS_ACTION_H__
#define __UNDO_DELETE_PLUGINS_ACTION_H__

#include "actions/undoable_action.h"
#include "audio/automation_track.h"
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

typedef struct DeletePluginsAction
{
  UndoableAction  parent_instance;

  /** Plugin clones.
   *
   * These must not be used in the project. They must
   * be cloned again before using. */
  MixerSelections * ms;

  /**
   * Automation tracks associated with the plugins.
   *
   * These are used when undoing so we can readd
   * the automation events.
   */
  AutomationTrack * ats[8000];
  int               num_ats;
} DeletePluginsAction;

static const cyaml_schema_field_t
  delete_plugins_action_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "parent_instance", CYAML_FLAG_DEFAULT,
    DeletePluginsAction, parent_instance,
    undoable_action_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "ms", CYAML_FLAG_POINTER,
    DeletePluginsAction, ms,
    mixer_selections_fields_schema),
  CYAML_FIELD_SEQUENCE_COUNT (
    "ats", CYAML_FLAG_DEFAULT,
    DeletePluginsAction, ats, num_ats,
    &automation_track_schema, 0, CYAML_UNLIMITED),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  delete_plugins_action_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER, DeletePluginsAction,
    delete_plugins_action_fields_schema),
};

void
delete_plugins_action_init_loaded (
  DeletePluginsAction * self);

UndoableAction *
delete_plugins_action_new (
  MixerSelections * ms);

int
delete_plugins_action_do (
  DeletePluginsAction * self);

int
delete_plugins_action_undo (
  DeletePluginsAction * self);

char *
delete_plugins_action_stringize (
  DeletePluginsAction * self);

void
delete_plugins_action_free (
  DeletePluginsAction * self);

/**
 * @}
 */

#endif
