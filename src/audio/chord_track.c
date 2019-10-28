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

#include "audio/chord_track.h"
#include "audio/scale.h"
#include "audio/track.h"
#include "gui/backend/events.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

/**
 * Inits a chord track (e.g. when cloning).
 */
void
chord_track_init (
  Track * self)
{
  self->type = TRACK_TYPE_CHORD;

  gdk_rgba_parse (&self->color, "#0328fa");
}

/**
 * Creates a new chord track.
 */
ChordTrack *
chord_track_new ()
{
  ChordTrack * self =
    track_new (
      TRACK_TYPE_CHORD, _("Chords"),
      F_WITHOUT_LANE);

  gdk_rgba_parse (&self->color, "#0328fa");

  return self;
}

/**
 * Adds a ChordObject to the Track.
 *
 * @param gen_widget Create a widget for the chord.
 */
void
chord_track_add_scale (
  ChordTrack *  track,
  ScaleObject * scale)
{
  g_warn_if_fail (
    track->type == TRACK_TYPE_CHORD && scale);
  g_warn_if_fail (
    scale_object_is_main (scale));
  g_warn_if_fail (
    scale_object_get_main (scale) &&
    scale_object_get_main_trans (scale));

  scale_object_set_track (scale, track);
  array_double_size_if_full (
    track->scales, track->num_scales,
    track->scales_size, ScaleObject *);
  array_append (track->scales,
                track->num_scales,
                scale);

  EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, scale);
}

/**
 * Returns the ScaleObject at the given Position
 * in the TimelineArranger.
 */
ScaleObject *
chord_track_get_scale_at_pos (
  const Track * ct,
  const Position * pos)
{
  ScaleObject * scale = NULL;
  ArrangerObject * s_obj;
  for (int i = ct->num_scales - 1; i >= 0; i--)
    {
      scale = ct->scales[i];
      s_obj = (ArrangerObject *) scale;
      if (position_is_before_or_equal (
            &s_obj->pos, pos))
        return scale;
    }
  return NULL;
}

/**
 * Returns the ChordObject at the given Position
 * in the TimelineArranger.
 */
ChordObject *
chord_track_get_chord_at_pos (
  const Track * ct,
  const Position * pos)
{
  Region * region =
    track_get_region_at_pos (ct, pos);

  if (!region)
    return NULL;

  ChordObject * chord = NULL;
  ArrangerObject * c_obj;
  int i;
  for (i = region->num_chord_objects - 1;
       i >= 0; i--)
    {
      chord = region->chord_objects[i];
      c_obj = (ArrangerObject *) chord;
      if (position_is_before_or_equal (
            &c_obj->pos, pos))
        return chord;
    }
  return NULL;
}

/**
 * Removes a scale from the chord Track.
 */
void
chord_track_remove_scale (
  ChordTrack *  self,
  ScaleObject * scale,
  int free)
{
  /* deselect */
  arranger_object_select (
    (ArrangerObject *) scale, F_NO_SELECT,
    F_APPEND);

  array_delete (
    self->scales, self->num_scales, scale);
  if (free)
    free_later (scale, arranger_object_free_all);

  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_REMOVED,
    ARRANGER_OBJECT_TYPE_SCALE_OBJECT);
}

void
chord_track_free (ChordTrack * self)
{
  /* remove chords */
  for (int i = 0; i < self->num_chord_regions; i++)
    arranger_object_free_all (
      (ArrangerObject *) self->chord_regions[i]);
}
