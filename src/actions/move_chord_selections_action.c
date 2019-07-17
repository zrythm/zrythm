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

#include "actions/move_chord_selections_action.h"
#include "audio/track.h"
#include "gui/backend/chord_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "project.h"

#include <glib/gi18n.h>

/**
 * Note: this action will add delta beats
 * to start and end pos of all selected midi_notes */
UndoableAction *
move_chord_selections_action_new (
  ChordSelections * ts,
  long                 ticks,
  int                  delta)
{
	MoveChordSelectionsAction * self =
    calloc (1, sizeof (
    	MoveChordSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_MOVE_CHORD_SELECTIONS;

  self->ts = chord_selections_clone (ts);
  self->delta = delta;
  self->ticks = ticks;
  self->first_call = 1;

  return ua;
}

#define SHIFT_OBJ(cc,sc,ticks) \
  cc * sc; \
  for (i = 0; i < self->ts->num_##sc##s; i++) \
    { \
      /* get the actual object */ \
      sc = sc##_find (self->ts->sc##s[i]); \
      g_warn_if_fail (sc); \
      /* shift the actual object */ \
      sc##_shift_by_ticks ( \
        sc, ticks, AO_UPDATE_ALL); \
      /* also shift the copy */ \
      sc##_shift_by_ticks ( \
        self->ts->sc##s[i], ticks, AO_UPDATE_THIS); \
    }

int
move_chord_selections_action_do (
	MoveChordSelectionsAction * self)
{
  int i;

#define SHIFT_OBJ_POSITIVE(cc,sc) \
  SHIFT_OBJ (cc, sc, self->ticks)

  if (!self->first_call)
    {
      SHIFT_OBJ_POSITIVE (
        ChordObject, chord_object);
    }

  EVENTS_PUSH (ET_CHORD_SELECTIONS_CHANGED,
               NULL);

  self->first_call = 0;

  return 0;
}

int
move_chord_selections_action_undo (
	MoveChordSelectionsAction * self)
{
  int i;

#define SHIFT_OBJ_NEGATIVE(cc,sc) \
  SHIFT_OBJ (cc, sc, - self->ticks)

  SHIFT_OBJ_NEGATIVE (
    ChordObject, chord_object);

  EVENTS_PUSH (ET_CHORD_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

char *
move_chord_selections_action_stringize (
	MoveChordSelectionsAction * self)
{
  return g_strdup (
    _("Move Chord(s)"));
}

void
move_chord_selections_action_free (
	MoveChordSelectionsAction * self)
{
  chord_selections_free (self->ts);

  free (self);
}
