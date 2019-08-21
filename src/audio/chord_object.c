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

#define SET_POS(_c,pos_name,_pos,_trans_only) \
  ARRANGER_OBJ_SET_POS ( \
    chord_object, _c, pos_name, _pos, _trans_only)

DEFINE_START_POS;

ARRANGER_OBJ_DEFINE_MOVABLE (
  ChordObject, chord_object, chord_selections,
  CHORD_SELECTIONS);

/**
 * Init the ChordObject after the Project is loaded.
 */
void
chord_object_init_loaded (
  ChordObject * self)
{
  ARRANGER_OBJECT_SET_AS_MAIN (
   CHORD_OBJECT, ChordObject, chord_object);
}

/**
 * Creates a ChordObject.
 */
ChordObject *
chord_object_new (
  int index,
  int is_main)
{
  ChordObject * self =
    calloc (1, sizeof (ChordObject));

  self->index = index;

  if (is_main)
    {
      ARRANGER_OBJECT_SET_AS_MAIN (
        CHORD_OBJECT, ChordObject, chord_object);
    }

  return self;
}

ARRANGER_OBJ_DECLARE_VALIDATE_POS (
  ChordObject, chord_object, pos)
{
  return
    position_is_after_or_equal (
      pos, START_POS);
}

void
chord_object_pos_setter (
  ChordObject * chord_object,
  const Position * pos)
{
  if (chord_object_validate_pos (chord_object, pos))
    {
      chord_object_set_pos (
        chord_object, pos, AO_UPDATE_ALL);
    }
}

/**
 * Finds the ChordObject in the project
 * corresponding to the given one.
 */
ChordObject *
chord_object_find (
  ChordObject * clone)
{
  /* get actual region - clone's region might be
   * an unused clone */
  Region *r = region_find (clone->region);

  ChordObject * chord;
  for (int i = 0; i < r->num_chord_objects; i++)
    {
      chord = r->chord_objects[i];
      if (chord_object_is_equal (
            chord, clone))
        return chord;
    }
  return NULL;
}

/**
 * Returns the ChordDescriptor associated with this
 * ChordObject.
 */
ChordDescriptor *
chord_object_get_chord_descriptor (
  ChordObject * self)
{
  return CHORD_EDITOR->chords[self->index];
}

/**
 * Updates the frames of each position in each child
 * of the ChordObject recursively.
 */
void
chord_object_update_frames (
  ChordObject * self)
{
  position_update_frames (&self->pos);
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
  Region *r = region_find (clone->region);

  ChordObject * chord;
  for (int i = 0; i < r->num_chord_objects; i++)
    {
      chord = r->chord_objects[i];
      if (position_is_equal (
            &chord->pos, &clone->pos))
        return chord;
    }
  return NULL;
}

/**
 * Clones the given chord.
 */
ChordObject *
chord_object_clone (
  ChordObject * src,
  ChordObjectCloneFlag flag)
{
  int is_main = 0;
  if (flag == CHORD_OBJECT_CLONE_COPY_MAIN)
    is_main = 1;

  ChordObject * chord =
    chord_object_new (src->index, is_main);

  position_set_to_pos (
    &chord->pos, &src->pos);

  return chord;
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
        co =
          chord_object_get_main_chord_object (
            self);
      else if (i == AOI_COUNTERPART_MAIN_TRANSIENT)
        co =
          chord_object_get_main_trans_chord_object (
            self);

      co->region = region;
      co->region_name = g_strdup (region->name);
    }
}

ARRANGER_OBJ_DEFINE_GEN_WIDGET_LANELESS (
  ChordObject, chord_object);

/**
 * Frees the ChordObject.
 */
void
chord_object_free (
  ChordObject * self)
{
  if (self->widget)
    {
      gtk_widget_destroy (
        GTK_WIDGET (self->widget));
    }

  if (self->region_name)
    g_free (self->region_name);

  free (self);
}
