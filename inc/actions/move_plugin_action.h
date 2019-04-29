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

#ifndef __UNDO_MOVE_PLUGIN_ACTION_H__
#define __UNDO_MOVE_PLUGIN_ACTION_H__

#include "actions/undoable_action.h"

typedef struct Plugin Plugin;
typedef struct Channel Channel;

typedef struct MovePluginAction
{
  UndoableAction  parent_instance;

  /** From slot. */
  int             from_slot;

  /** To slot. */
  int             to_slot;

  /** From channel ID. */
  int             from_ch_id;

  /** To channel ID. */
  int             to_ch_id;

  /** The plugin to move. */
  int             pl_id;
} MovePluginAction;

UndoableAction *
move_plugin_action_new (
  Plugin *  pl,
  Channel * to_ch,
  int       to_slot);

void
move_plugin_action_do (
	MovePluginAction * self);

void
move_plugin_action_undo (
	MovePluginAction * self);

void
move_plugin_action_free (
	MovePluginAction * self);

#endif
