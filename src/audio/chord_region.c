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

#include "audio/chord_region.h"
#include "audio/chord_object.h"
#include "gui/backend/events.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

/**
 * Creates a new Region for chords.
 *
 * @param is_main If this is 1 it
 *   will create the additional Region (
 *   main_transient).
 */
Region *
chord_region_new (
  const Position * start_pos,
  const Position * end_pos,
  const int        is_main)
{
  Region * self =
    calloc (1, sizeof (Region));

  self->chord_objects_size = 1;
  self->chord_objects =
    malloc (self->chord_objects_size *
            sizeof (ChordObject *));

  self->type = REGION_TYPE_CHORD;

  region_init (
    self, start_pos, end_pos, is_main);

  return self;
}

/**
 * Adds a ChordObject to the Region.
 */
void
chord_region_add_chord_object (
  Region *      self,
  ChordObject * chord)
{
  array_double_size_if_full (
    self->chord_objects, self->num_chord_objects,
    self->chord_objects_size, ChordObject *);
  array_append (
    self->chord_objects, self->num_chord_objects,
    chord);

  chord_object_set_region (chord, self);

  EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, chord);
}

/**
 * Removes a ChordObject from the Region.
 *
 * @param free Optionally free the ChordObject.
 */
void
chord_region_remove_chord_object (
  Region *      self,
  ChordObject * chord,
  int           free)
{
  /* deselect */
  arranger_object_select (
    (ArrangerObject *) chord, F_NO_SELECT,
    F_APPEND);

  array_delete (self->chord_objects,
                self->num_chord_objects,
                chord);


  if (free)
    free_later (chord, arranger_object_free);

  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_REMOVED,
    ARRANGER_OBJECT_TYPE_CHORD_OBJECT);
}

/**
 * Frees members only but not the Region itself.
 *
 * Regions should be free'd using region_free.
 */
void
chord_region_free_members (
  Region * self)
{
  int i;
  for (i = 0; i < self->num_chord_objects; i++)
    chord_region_remove_chord_object (
      self, self->chord_objects[i], F_FREE);

  free (self->chord_objects);
}
