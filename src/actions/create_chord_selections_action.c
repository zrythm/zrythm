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

#include "actions/create_chord_selections_action.h"
#include "audio/chord_object.h"
#include "audio/chord_region.h"
#include "audio/chord_track.h"
#include "audio/marker.h"
#include "audio/marker_track.h"
#include "audio/scale_object.h"
#include "audio/track.h"
#include "gui/backend/chord_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

/**
 * The given ChordSelections must already
 * contain the created selections in the transient
 * arrays.
 *
 * Note: chord addresses are to be copied.
 */
UndoableAction *
create_chord_selections_action_new (
  ChordSelections * ts)
{
  CreateChordSelectionsAction * self =
    calloc (1, sizeof (
                 CreateChordSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_CREATE_CHORD_SELECTIONS;

  self->cs = chord_selections_clone (ts);
  self->first_run = 1;

  return ua;
}

int
create_chord_selections_action_do (
  CreateChordSelectionsAction * self)
{
  int i;

  if (!self->first_run)
    {
      /* clear current selections */
      chord_selections_clear (
        CHORD_SELECTIONS);
    }

  ChordObject * chord;
	for (i = 0;
       i < self->cs->num_chord_objects; i++)
    {
      /* check if the region already exists. due to
       * how the arranger creates regions, the region
       * should already exist the first time so no
       * need to do anything. when redoing we will
       * need to create a clone instead */
      if (chord_object_find (
            self->cs->chord_objects[i]))
        continue;

      /* clone the clone */
      chord =
        chord_object_clone (
          self->cs->chord_objects[i],
          CHORD_OBJECT_CLONE_COPY_MAIN);

      /* find the region */
      chord->region =
        region_find_by_name (chord->region_name);
      g_return_val_if_fail (chord->region, -1);

      /* add it to the region */
      chord_region_add_chord_object (
        chord->region, chord);

      /* select it */
      chord_object_select (
        chord, F_SELECT);
    }

  EVENTS_PUSH (ET_CHORD_OBJECT_CREATED,
               NULL);

  if (self->first_run)
    self->first_run = 0;

  return 0;
}

int
create_chord_selections_action_undo (
  CreateChordSelectionsAction * self)
{
  int i;
  ChordObject * chord_object;
  for (i = 0; i < self->cs->num_chord_objects; i++)
    {
      /* get the actual chord */
      chord_object =
        chord_object_find (
          self->cs->chord_objects[i]);

      /* remove it */
      chord_region_remove_chord_object (
        chord_object->region,
        chord_object, F_FREE);
    }

  EVENTS_PUSH (ET_CHORD_OBJECT_REMOVED,
               NULL);

  return 0;
}

char *
create_chord_selections_action_stringize (
  CreateChordSelectionsAction * self)
{
  return g_strdup (
    _("Create Chord(s)"));
}

void
create_chord_selections_action_free (
  CreateChordSelectionsAction * self)
{
  chord_selections_free (self->cs);

  free (self);
}
