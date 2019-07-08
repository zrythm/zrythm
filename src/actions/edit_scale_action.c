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

#include "actions/edit_scale_action.h"
#include "audio/scale.h"
#include "audio/scale_object.h"
#include "project.h"

#include <glib/gi18n.h>

UndoableAction *
edit_scale_action_new (
  ScaleObject *  scale,
  MusicalScale * descr)
{
	EditScaleAction * self =
    calloc (1, sizeof (
    	EditScaleAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_EDIT_SCALE;

  self->scale =
    scale_object_clone (
      scale, SCALE_OBJECT_CLONE_COPY_MAIN);
  self->descr = musical_scale_clone (descr);

  return ua;
}

int
edit_scale_action_do (
	EditScaleAction * self)
{
  /* get the actual scale */
  ScaleObject * scale =
    scale_object_find (self->scale);

  /* set the new descriptor */
  MusicalScale * old = scale->scale;
  scale->scale =
    musical_scale_clone (self->descr);
  musical_scale_free (old);

  EVENTS_PUSH (ET_SCALE_OBJECT_CHANGED, scale);

  return 0;
}

int
edit_scale_action_undo (
	EditScaleAction * self)
{
  /* get the actual scale */
  MusicalScale * prev = self->scale->scale;
  self->scale->scale = self->descr;
  ScaleObject * scale =
    scale_object_find (self->scale);
  self->scale->scale = prev;

  /* set the old descriptor */
  prev = scale->scale;
  scale->scale =
    musical_scale_clone (self->scale->scale);
  musical_scale_free (prev);

  EVENTS_PUSH (ET_SCALE_OBJECT_CHANGED, scale);

  return 0;
}

char *
edit_scale_action_stringize (
	EditScaleAction * self)
{
  return g_strdup (_("Change Scale"));
}

void
edit_scale_action_free (
	EditScaleAction * self)
{
  musical_scale_free (self->descr);
  scale_object_free (self->scale);

  free (self);
}
