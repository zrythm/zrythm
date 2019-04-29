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

#ifndef __UNDO_CREATE_PLUGIN_ACTION_H__
#define __UNDO_CREATE_PLUGIN_ACTION_H__

#include "actions/undoable_action.h"
#include "plugins/plugin.h"

/**
 * @addtogroup actions
 *
 * @{
 */

typedef struct Plugin Plugin;
typedef struct Channel Channel;

typedef struct CreatePluginAction
{
  UndoableAction  parent_instance;

  /** Slot. */
  int              slot;

  /** Channel ID. */
  int              ch_id;

  /** PluginDescriptor, used when doing. */
  PluginDescriptor descr;
} CreatePluginAction;

UndoableAction *
create_plugin_action_new (
  PluginDescriptor * descr,
  Channel * to_ch,
  int       to_slot);

int
create_plugin_action_do (
	CreatePluginAction * self);

int
create_plugin_action_undo (
	CreatePluginAction * self);

void
create_plugin_action_free (
	CreatePluginAction * self);

/**
 * @}
 */

#endif
