/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __UNDO_CREATE_PLUGINS_ACTION_H__
#define __UNDO_CREATE_PLUGINS_ACTION_H__

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

  /** Slot to create to.
   *
   * Also used for undoing. */
  int              slot;

  /** Position of track to create to. */
  int              track_pos;

  /**
   * PluginDescriptor to use when creating.
   */
  PluginDescriptor descr;

  int              num_plugins;
} CreatePluginsAction;

UndoableAction *
create_plugins_action_new (
  PluginDescriptor * descr,
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
