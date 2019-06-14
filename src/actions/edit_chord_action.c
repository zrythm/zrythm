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

#include "actions/edit_chord_action.h"
#include "audio/chord_descriptor.h"
#include "audio/chord_object.h"
#include "project.h"

#include <glib/gi18n.h>

UndoableAction *
edit_chord_action_new (
  ChordObject *     chord,
  ChordDescriptor * descr)
{
	EditChordAction * self =
    calloc (1, sizeof (
    	EditChordAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_EDIT_CHORD;

  self->chord =
    chord_object_clone (
      chord, CHORD_OBJECT_CLONE_COPY_MAIN);
  self->descr = chord_descriptor_clone (descr);

  return ua;
}

int
edit_chord_action_do (
	EditChordAction * self)
{
  /* get the actual chord */
  ChordObject * chord =
    chord_object_find (self->chord);

  /* set the new descriptor */
  ChordDescriptor * old = chord->descr;
  chord->descr =
    chord_descriptor_clone (self->descr);
  chord_descriptor_free (old);

  EVENTS_PUSH (ET_CHORD_CHANGED, chord);

  return 0;
}

int
edit_chord_action_undo (
	EditChordAction * self)
{
  /* get the actual chord */
  ChordDescriptor * prev = self->chord->descr;
  self->chord->descr = self->descr;
  ChordObject * chord =
    chord_object_find (self->chord);
  self->chord->descr = prev;

  /* set the old descriptor */
  prev = chord->descr;
  chord->descr =
    chord_descriptor_clone (self->chord->descr);
  chord_descriptor_free (prev);

  EVENTS_PUSH (ET_CHORD_CHANGED, chord);

  return 0;
}

char *
edit_chord_action_stringize (
	EditChordAction * self)
{
  return g_strdup (_("Change Chord"));
}

void
edit_chord_action_free (
	EditChordAction * self)
{
  chord_descriptor_free (self->descr);
  chord_object_free (self->chord);

  free (self);
}
