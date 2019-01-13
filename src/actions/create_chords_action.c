/*
 * actions/create_chords_action.c - UndoableAction
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "audio/chord.h"
#include "audio/chord_track.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "actions/create_chords_action.h"

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
create_chords_action_new (Chord **  chords,
                          int       num_chords)
{
  CreateChordsAction * self =
    calloc (1, sizeof (CreateChordsAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_CREATE_CHORDS;

  self->num_chords = num_chords;
  for (int i = 0; i < num_chords; i++)
    {
      self->chords[i] = chords[i];
    }

  return ua;
}

void
create_chords_action_do (CreateChordsAction * self)
{
  for (int i = 0; i < self->num_chords; i++)
    {
      Chord * chord = self->chords[i];
      chord_track_add_chord (CHORD_TRACK,
                             chord);
      timeline_arranger_widget_refresh_children (
        MW_TIMELINE);
    }
}

void
create_chords_action_undo (CreateChordsAction * self)
{
  for (int i = 0; i < self->num_chords; i++)
    {
      Chord * chord = self->chords[i];
      chord_track_remove_chord (CHORD_TRACK,
                                chord);
      timeline_arranger_widget_refresh_children (
        MW_TIMELINE);
    }
}

