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

#include "actions/edit_marker_action.h"
#include "audio/marker.h"
#include "project.h"

#include <glib/gi18n.h>

UndoableAction *
edit_marker_action_new (
  Marker *     marker,
  const char * text)
{
	EditMarkerAction * self =
    calloc (1, sizeof (
    	EditMarkerAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_EDIT_MARKER;

  self->marker =
    marker_clone (
      marker, MARKER_CLONE_COPY_MAIN);
  self->text = g_strdup (text);

  return ua;
}

int
edit_marker_action_do (
	EditMarkerAction * self)
{
  /* get the actual marker */
  Marker * marker =
    marker_find (self->marker);

  /* set the new name */
  marker_set_name (marker, self->text);

  EVENTS_PUSH (ET_MARKER_CHANGED, marker);

  return 0;
}

int
edit_marker_action_undo (
	EditMarkerAction * self)
{
  /* get the actual marker */
  char * prev = self->marker->name;
  self->marker->name = self->text;
  Marker * marker =
    marker_find (self->marker);
  self->marker->name = prev;

  /* set the old name */
  prev = marker->name;
  marker_set_name (marker, self->text);
  g_free (prev);

  EVENTS_PUSH (ET_MARKER_CHANGED, marker);

  return 0;
}

char *
edit_marker_action_stringize (
	EditMarkerAction * self)
{
  return g_strdup (_("Change Marker"));
}

void
edit_marker_action_free (
	EditMarkerAction * self)
{
  marker_free (self->marker);
  g_free (self->text);

  free (self);
}
