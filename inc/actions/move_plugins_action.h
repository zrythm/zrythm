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

#ifndef __UNDO_MOVE_PLUGINS_ACTION_H__
#define __UNDO_MOVE_PLUGINS_ACTION_H__

#include "actions/undoable_action.h"

typedef struct Plugin Plugin;
typedef struct Channel Channel;
typedef struct MixerSelections MixerSelections;

/**
 * Restrict selections to a channel.
 */
typedef struct MovePluginsAction
{
  UndoableAction  parent_instance;

  /** To slot.
   *
   * The rest of the slots will start from this so
   * they can be calculated when doing/undoing. */
  int             to_slot;

  /** To track position. */
  int             to_track_pos;

  /** From track position. */
  int             from_track_pos;

  /** MixerSelections clone containing all the
   * necessary information.
   */
  MixerSelections * ms;
} MovePluginsAction;

UndoableAction *
move_plugins_action_new (
  MixerSelections * ms,
  Channel *  to_ch,
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
