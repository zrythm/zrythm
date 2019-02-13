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

#ifndef __UNDO_EDIT_TRACK_ACTION_H__
#define __UNDO_EDIT_TRACK_ACTION_H__

#include "actions/undoable_action.h"

typedef enum EditTrackActionType
{
  EDIT_TRACK_ACTION_TYPE_SOLO,
  EDIT_TRACK_ACTION_TYPE_MUTE,
  EDIT_TRACK_ACTION_TYPE_RECORD,
} EditTrackActionType;

typedef struct Track Track;

typedef struct EditTrackAction
{
  UndoableAction              parent_instance;
  EditTrackActionType         type;

  int                         track_id;

  /**
   * Params to be changed.
   */
  int                         solo_new;
  int                         mute_new;
} EditTrackAction;

UndoableAction *
edit_track_action_new_solo (Track * track,
                            int       solo);

UndoableAction *
edit_track_action_new_mute (Track * track,
                            int       mute);

void
edit_track_action_do (EditTrackAction * self);

void
edit_track_action_undo (EditTrackAction * self);

void
edit_track_action_free (EditTrackAction * self);

#endif

