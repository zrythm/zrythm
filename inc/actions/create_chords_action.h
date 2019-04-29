/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __UNDO_CREATE_CHORDS_ACTION_H__
#define __UNDO_CREATE_CHORDS_ACTION_H__

#include "actions/undoable_action.h"

typedef struct ZChord ZChord;

typedef struct CreateChordsAction
{
  UndoableAction              parent_instance;

  ZChord *                     chords[800];
  int                         num_chords;
} CreateChordsAction;

UndoableAction *
create_chords_action_new (ZChord ** chords,
                          int       num_chords);

void
create_chords_action_do (CreateChordsAction * self);

void
create_chords_action_undo (CreateChordsAction * self);

void
create_chords_action_free (CreateChordsAction * self);

#endif
