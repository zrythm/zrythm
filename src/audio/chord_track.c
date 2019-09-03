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
  self->name = g_strdup (_("Chords"));

  gdk_rgba_parse (&self->color, "#0328fa");
}

/**
 * Creates a new chord track.
 */
ChordTrack *
chord_track_new ()
{
  ChordTrack * self =
    track_new (TRACK_TYPE_CHORD, _("Chords"));

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
    scale->obj_info.counterpart ==
      AOI_COUNTERPART_MAIN);
  g_warn_if_fail (
    scale->obj_info.main &&
    scale->obj_info.main_trans &&
    scale->obj_info.lane &&
    scale->obj_info.lane_trans);

  scale_object_set_track (scale, track);
  array_double_size_if_full (
    track->scales, track->num_scales,
    track->scales_size, ScaleObject *);
  array_append (track->scales,
                track->num_scales,
                scale);

  EVENTS_PUSH (ET_SCALE_OBJECT_CREATED, scale);
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
  for (int i = ct->num_scales - 1; i >= 0; i--)
    {
      scale = ct->scales[i];
      if (position_is_before_or_equal (
            &scale->pos, pos))
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
  int i;
  for (i = region->num_chord_objects - 1;
       i >= 0; i--)
    {
      chord = region->chord_objects[i];
      if (position_is_before_or_equal (
            &chord->pos, pos))
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
  timeline_selections_remove_scale_object (
    TL_SELECTIONS, scale);

  array_delete (self->scales,
                self->num_scales,
                scale);
  if (free)
    free_later (scale, scale_object_free);

  EVENTS_PUSH (ET_SCALE_OBJECT_REMOVED, self);
}

void
chord_track_free (ChordTrack * self)
{
  /* remove chords */
  for (int i = 0; i < self->num_chord_regions; i++)
    region_free (self->chord_regions[i]);
}
