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

#include "actions/edit_midi_arranger_selections_action.h"
#include "audio/track.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/midi_arranger.h"
#include "project.h"

#include <glib/gi18n.h>

UndoableAction *
edit_midi_arranger_selections_action_new (
  MidiArrangerSelections * mas)
{
	EditMidiArrangerSelectionsAction * self =
    calloc (1, sizeof (
    	EditMidiArrangerSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_EDIT_MA_SELECTIONS;

  self->mas = midi_arranger_selections_clone (mas);

  return ua;
}

int
edit_midi_arranger_selections_action_do (
	EditMidiArrangerSelectionsAction * self)
{
  return 0;
}

int
edit_midi_arranger_selections_action_undo (
	EditMidiArrangerSelectionsAction * self)
{
  return 0;
}

char *
edit_midi_arranger_selections_action_stringize (
	EditMidiArrangerSelectionsAction * self)
{
  return g_strdup (
    _("Edit Object(s)"));
}

void
edit_midi_arranger_selections_action_free (
	EditMidiArrangerSelectionsAction * self)
{
  midi_arranger_selections_free (self->mas);

  free (self);
}

