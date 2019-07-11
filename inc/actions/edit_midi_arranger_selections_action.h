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

/**
 * \file
 *
 * UndoableAction for MidiArrangerSelections edits.
 */

#ifndef __UNDO_EDIT_MA_SELECTIONS_ACTION_H__
#define __UNDO_EDIT_MA_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"

typedef struct MidiArrangerSelections
  MidiArrangerSelections;

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * Type of action.
 */
typedef enum EditMidiArrangerSelectionsType
{
  EMAS_TYPE_RESIZE_L,
  EMAS_TYPE_RESIZE_R,
  EMAS_TYPE_VELOCITY_CHANGE,
} EditMidiArrangerSelectionsType;

/**
 * The UndoableAction.
 */
typedef struct EditMidiArrangerSelectionsAction
{
  UndoableAction              parent_instance;

  /** Clone of selections. */
  MidiArrangerSelections * mas;

  /** Type of action. */
  EditMidiArrangerSelectionsType   type;

  /** Ticks when resizing. */
  long                   ticks;

  /** Difference for velocity changes, etc. */
  int                    diff;

  /** If this is the first time calling do. */
  int                    first_time;

} EditMidiArrangerSelectionsAction;

/**
 * Simple way to create an action for Velocity
 * change.
 */
#define emas_action_new_vel_change( \
  mas, diff) \
  edit_midi_arranger_selections_action_new ( \
    mas, EMAS_TYPE_VELOCITY_CHANGE, -1, diff)

/**
 * Simple way to create an action for resizing L.
 */
#define emas_action_new_resize_l( \
  mas, ticks) \
  edit_midi_arranger_selections_action_new ( \
    mas, EMAS_TYPE_RESIZE_L, ticks, -1)

/**
 * Simple way to create an action for resizing R.
 */
#define emas_action_new_resize_r( \
  mas, ticks) \
  edit_midi_arranger_selections_action_new ( \
    mas, EMAS_TYPE_RESIZE_R, ticks, -1)

/**
 * Create the new action.
 */
UndoableAction *
edit_midi_arranger_selections_action_new (
  MidiArrangerSelections * mas,
  EditMidiArrangerSelectionsType type,
  long                     ticks,
  int                      diff);

int
edit_midi_arranger_selections_action_do (
	EditMidiArrangerSelectionsAction * self);

int
edit_midi_arranger_selections_action_undo (
	EditMidiArrangerSelectionsAction * self);

char *
edit_midi_arranger_selections_action_stringize (
	EditMidiArrangerSelectionsAction * self);

void
edit_midi_arranger_selections_action_free (
	EditMidiArrangerSelectionsAction * self);

#endif
