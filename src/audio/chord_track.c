/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

/**
 * Inits a chord track (e.g. when cloning).
 */
void
chord_track_init (
  Track * self)
{
  self->type = TRACK_TYPE_CHORD;

  gdk_rgba_parse (&self->color, "#0348fa");
}

/**
 * Creates a new chord track.
 */
ChordTrack *
chord_track_new (
  int     track_pos)
{
  ChordTrack * self =
    track_new (
      TRACK_TYPE_CHORD, track_pos, _("Chords"),
      F_WITHOUT_LANE);

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

  array_double_size_if_full (
    track->scales, track->num_scales,
    track->scales_size, ScaleObject *);
  array_append (
    track->scales, track->num_scales, scale);
  scale->index = track->num_scales - 1;

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
  ZRegion * region =
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
 * Removes all objects from the chord track.
 *
 * Mainly used in testing.
 */
void
chord_track_clear (
  ChordTrack * self)
{
  g_return_if_fail (
    IS_TRACK (self) &&
    self->type == TRACK_TYPE_CHORD);

  for (int i = 0; i < self->num_scales; i++)
    {
      ScaleObject * scale = self->scales[i];
      chord_track_remove_scale (self, scale, 1);
    }
  for (int i = 0; i < self->num_chord_regions; i++)
    {
      ZRegion * region = self->chord_regions[i];
      track_remove_region (self, region, 0, 1);
    }
}

/**
 * Removes a scale from the chord Track.
 */
void
chord_track_remove_scale (
  ChordTrack *  self,
  ScaleObject * scale,
  bool          free)
{
  g_return_if_fail (
    IS_TRACK (self) && IS_SCALE_OBJECT (scale));

  /* deselect */
  arranger_object_select (
    (ArrangerObject *) scale, F_NO_SELECT,
    F_APPEND);

  array_delete (
    self->scales, self->num_scales, scale);

  scale->index = -1;

  if (free)
    {
      free_later (scale, arranger_object_free);
    }

  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_REMOVED,
    ARRANGER_OBJECT_TYPE_SCALE_OBJECT);
}
