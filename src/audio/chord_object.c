/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>

#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "audio/position.h"
#include "gui/widgets/chord_object.h"
#include "project.h"
#include "utils/flags.h"

/**
 * Creates a ChordObject.
 */
ChordObject *
chord_object_new (
  Region * region,
  int index,
  int is_main)
{
  ChordObject * self =
    calloc (1, sizeof (ChordObject));

  ArrangerObject * obj =
    (ArrangerObject *) self;
  obj->type = ARRANGER_OBJECT_TYPE_CHORD_OBJECT;

  self->index = index;
  self->region = region;
  self->region_name =
    g_strdup (region->name);

  if (is_main)
    {
      arranger_object_set_as_main (obj);
    }

  return self;
}

/**
 * Returns the ChordDescriptor associated with this
 * ChordObject.
 */
ChordDescriptor *
chord_object_get_chord_descriptor (
  ChordObject * self)
{
  g_return_val_if_fail (CLIP_EDITOR, NULL);
  return CHORD_EDITOR->chords[self->index];
}

int
chord_object_is_equal (
  ChordObject * a,
  ChordObject * b)
{
  ArrangerObject * obj_a =
    (ArrangerObject *) a;
  ArrangerObject * obj_b =
    (ArrangerObject *) b;
  return
    position_is_equal (&obj_a->pos, &obj_b->pos) &&
    a->index == b->index;
}

/**
 * Finds the ChordObject in the project
 * corresponding to the given one's position.
 *
 * Used in actions.
 */
ChordObject *
chord_object_find_by_pos (
  ChordObject * clone)
{
  /* get actual region - clone's region might be
   * an unused clone */
  Region *r =
    (Region *)
    arranger_object_find (
      (ArrangerObject *) clone->region);
  g_return_val_if_fail (r, NULL);

  ChordObject * chord;
  ArrangerObject * c_obj;
  ArrangerObject * clone_obj =
    (ArrangerObject *) clone;
  for (int i = 0; i < r->num_chord_objects; i++)
    {
      chord = r->chord_objects[i];
      c_obj = (ArrangerObject *) chord;
      if (position_is_equal (
            &c_obj->pos, &clone_obj->pos))
        return chord;
    }
  return NULL;
}

/**
 * Sets the Track of the chord.
 */
void
chord_object_set_region (
  ChordObject * self,
  Region *      region)
{
  ChordObject * co;
  for (int i = 0; i < 2; i++)
    {
      if (i == AOI_COUNTERPART_MAIN)
        co = chord_object_get_main (self);
      else if (i == AOI_COUNTERPART_MAIN_TRANSIENT)
        co = chord_object_get_main_trans (self);

      co->region = region;
      co->region_name = g_strdup (region->name);
    }
}
