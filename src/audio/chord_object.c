/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>

#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "audio/position.h"
#include "gui/widgets/chord_object.h"
#include "project.h"
#include "utils/flags.h"

#define SET_POS(_c,pos_name,_pos,_trans_only) \
  POSITION_SET_ARRANGER_OBJ_POS ( \
    chord_object, _c, pos_name, _pos, _trans_only)

/**
 * Init the ChordObject after the Project is loaded.
 */
void
chord_object_init_loaded (
  ChordObject * self)
{
}

DEFINE_ARRANGER_OBJ_SET_POS (
  ChordObject, chord_object);

/**
 * Creates a ChordObject.
 */
ChordObject *
chord_object_new (
  ChordDescriptor * descr,
  int               is_main)
{
  ChordObject * self =
    calloc (1, sizeof (ChordObject));

  self->descr = descr;

  if (is_main)
    {
      /* set it as main */
      ChordObject * main_trans =
        chord_object_clone (
          self, CHORD_OBJECT_CLONE_COPY);
      ChordObject * lane =
        chord_object_clone (
          self, CHORD_OBJECT_CLONE_COPY);
      ChordObject * lane_trans =
        chord_object_clone (
          self, CHORD_OBJECT_CLONE_COPY);
      arranger_object_info_init_main (
        self,
        main_trans,
        lane,
        lane_trans,
        AOI_TYPE_CHORD);
    }

  return self;
}

/**
 * Finds the ChordObject in the project
 * corresponding to the given one.
 */
ChordObject *
chord_object_find (
  ChordObject * clone)
{
  for (int i = 0;
       i < P_CHORD_TRACK->num_chords; i++)
    {
      if (chord_object_is_equal (
            P_CHORD_TRACK->chords[i],
            clone))
        return P_CHORD_TRACK->chords[i];
    }
  return NULL;
}

/**
 * Finds the ChordObject in the project
 * corresponding to the given one's position.
 */
ChordObject *
chord_object_find_by_pos (
  ChordObject * clone)
{
  for (int i = 0;
       i < P_CHORD_TRACK->num_chords; i++)
    {
      if (position_is_equal (
            &P_CHORD_TRACK->chords[i]->pos,
            &clone->pos))
        return P_CHORD_TRACK->chords[i];
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

  ChordDescriptor * descr =
    chord_descriptor_clone (src->descr);
  ChordObject * chord =
    chord_object_new (descr, is_main);

  position_set_to_pos (
    &chord->pos, &src->pos);

  return chord;
}

DEFINE_IS_ARRANGER_OBJ_SELECTED (
  ChordObject, chord_object, timeline_selections,
  TL_SELECTIONS);

/**
 * Sets the Track of the chord.
 */
void
chord_object_set_track (
  ChordObject * self,
  Track *  track)
{
  self->track = track;
  self->track_pos = track->pos;
}

/**
 * Generates a widget for the chord.
 */
void
chord_object_gen_widget (
  ChordObject * self)
{
  ChordObject * c = self;
  for (int i = 0; i < 2; i++)
    {
      if (i == 0)
        c = chord_object_get_main_chord_object (c);
      else if (i == 1)
        c = chord_object_get_main_trans_chord_object (c);

      c->widget = chord_object_widget_new (c);
    }
}

DEFINE_ARRANGER_OBJ_MOVE (
  ChordObject, chord_object);

/**
 * Shifts the ChordObject by given number of ticks
 * on x.
 */
void
chord_object_shift (
  ChordObject * self,
  long ticks)
{
  if (ticks)
    {
      Position tmp;
      position_set_to_pos (
        &tmp, &self->pos);
      position_add_ticks (&tmp, ticks);
      SET_POS (self, pos, (&tmp), F_NO_TRANS_ONLY);
    }
}

/**
 * Returns the Track this ChordObject is in.
 */
Track *
chord_object_get_track (
  ChordObject * self)
{
  return TRACKLIST->tracks[self->track_pos];
}

/**
 * Frees the ChordObject.
 */
void
chord_object_free (
  ChordObject * self)
{
  chord_descriptor_free (self->descr);
  free (self);
}
