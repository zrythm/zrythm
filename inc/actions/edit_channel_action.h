/*
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __UNDO_EDIT_CHANNEL_ACTION_H__
#define __UNDO_EDIT_CHANNEL_ACTION_H__

#include "actions/undoable_action.h"

typedef enum EditChannelActionType
{
  EDIT_CHANNEL_ACTION_TYPE_CHANGE_VOLUME,
  EDIT_CHANNEL_ACTION_TYPE_CHANGE_PAN,
} EditChannelActionType;

typedef struct Channel Channel;

typedef struct EditChannelAction
{
  UndoableAction              parent_instance;
  EditChannelActionType       type;

  int                         channel_id;

  /**
   * Params to be changed.
   */
  float                       vol_prev;
  float                       vol_new;
  float                       pan_prev;
  float                       pan_new;
} EditChannelAction;

UndoableAction *
edit_channel_action_new_vol (Channel * channel,
                             float vol_prev,
                             float vol_new);

UndoableAction *
edit_channel_action_new_pan (Channel * channel,
                             float pan_prev,
                             float pan_new);

void
edit_channel_action_do (EditChannelAction * self);

void
edit_channel_action_undo (EditChannelAction * self);

void
edit_channel_action_free (EditChannelAction * self);

#endif
