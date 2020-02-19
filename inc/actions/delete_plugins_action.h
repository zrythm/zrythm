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

#ifndef __UNDO_DELETE_PLUGINS_ACTION_H__
#define __UNDO_DELETE_PLUGINS_ACTION_H__

#include "actions/undoable_action.h"
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

  /** Slot to start deleting from. */
  //int              slot;

  /** Track to delete from. */
  //int              tr_pos;

  /** Plugin clones.
   *
   * These must not be used in the project. They must
   * be cloned again before using. */
  MixerSelections * ms;
} DeletePluginsAction;

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
