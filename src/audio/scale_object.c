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

#include "audio/scale_object.h"
#include "audio/chord_track.h"
#include "audio/position.h"
#include "gui/widgets/scale_object.h"
#include "project.h"
#include "utils/flags.h"

#define SET_POS(_c,pos_name,_pos,_trans_only) \
  ARRANGER_OBJ_SET_POS ( \
    scale_object, _c, pos_name, _pos, _trans_only)

DEFINE_START_POS;

/**
 * Init the ScaleObject after the Project is loaded.
 */
void
scale_object_init_loaded (
  ScaleObject * self)
{
}

/**
 * Creates a ScaleObject.
 */
ScaleObject *
scale_object_new (
  MusicalScale * descr,
  int               is_main)
{
  ScaleObject * self =
    calloc (1, sizeof (ScaleObject));

  self->scale = descr;

  if (is_main)
    {
      ARRANGER_OBJECT_SET_AS_MAIN (
        SCALE_OBJECT, ScaleObject, scale_object);
    }

  return self;
}

/**
 * Finds the ScaleObject in the project
 * corresponding to the given one.
 */
ScaleObject *
scale_object_find (
  ScaleObject * clone)
{
  for (int i = 0;
       i < P_CHORD_TRACK->num_scales; i++)
    {
      if (scale_object_is_equal (
            P_CHORD_TRACK->scales[i],
            clone))
        return P_CHORD_TRACK->scales[i];
    }
  return NULL;
}

/**
 * Clones the given scale.
 */
ScaleObject *
scale_object_clone (
  ScaleObject * src,
  ScaleObjectCloneFlag flag)
{
  int is_main = 0;
  if (flag == SCALE_OBJECT_CLONE_COPY_MAIN)
    is_main = 1;

  MusicalScale * musical_scale =
    musical_scale_clone (src->scale);
  ScaleObject * scale =
    scale_object_new (musical_scale, is_main);

  position_set_to_pos (
    &scale->pos, &src->pos);

  return scale;
}

DEFINE_IS_ARRANGER_OBJ_SELECTED (
  ScaleObject, scale_object, timeline_selections,
  TL_SELECTIONS);

/**
 * Sets the Track of the scale.
 */
void
scale_object_set_track (
  ScaleObject * self,
  Track *  track)
{
  self->track = track;
  self->track_pos = track->pos;
}

ARRANGER_OBJ_DEFINE_GEN_WIDGET_LANELESS (
  ScaleObject, scale_object);

DEFINE_ARRANGER_OBJ_SET_POS (
  ScaleObject, scale_object);

/**
 * Moves the ScaleObject by the given amount of
 * ticks.
 *
 * @param use_cached_pos Add the ticks to the cached
 *   Position instead of its current Position.
 * @param trans_only Only move transients.
 * @return Whether moved or not.
 */
int
scale_object_move (
  ScaleObject * scale,
  long     ticks,
  int      use_cached_pos,
  int      trans_only)
{
  Position tmp;
  int moved;
  POSITION_MOVE_BY_TICKS (
    tmp, use_cached_pos, scale, pos, ticks, moved,
    trans_only);

  return moved;
}

ARRANGER_OBJ_DEFINE_SHIFT_TICKS (
  ScaleObject, scale_object);

void
scale_object_pos_setter (
  ScaleObject * scale_object,
  const Position * pos)
{
  if (position_is_after_or_equal (
        pos, START_POS))
    {
      scale_object_set_pos (
        scale_object, pos, F_NO_TRANS_ONLY);
    }
}

/**
 * Returns the Track this ScaleObject is in.
 */
Track *
scale_object_get_track (
  ScaleObject * self)
{
  return TRACKLIST->tracks[self->track_pos];
}

/**
 * Frees the ScaleObject.
 */
void
scale_object_free (
  ScaleObject * self)
{
  musical_scale_free (self->scale);
  free (self);
}

