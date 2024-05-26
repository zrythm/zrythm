// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "dsp/chord_region.h"
#include "dsp/chord_track.h"
#include "dsp/scale.h"
#include "dsp/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

/**
 * Inits a chord track (e.g. when cloning).
 */
void
chord_track_init (Track * self)
{
  self->type = TrackType::TRACK_TYPE_CHORD;
  /* GTK color picker color */
  gdk_rgba_parse (&self->color, "#1C71D8");
  self->icon_name = g_strdup ("minuet-chords");
}

/**
 * Creates a new chord track.
 */
ChordTrack *
chord_track_new (int track_pos)
{
  ChordTrack * self = track_new (
    TrackType::TRACK_TYPE_CHORD, track_pos, _ ("Chords"), F_WITHOUT_LANE);

  return self;
}

/**
 * Inserts a chord region to the Track at the given
 * index.
 */
void
chord_track_insert_chord_region (Track * self, ZRegion * region, int idx)
{
  g_return_if_fail (idx >= 0);
  array_double_size_if_full (
    self->chord_regions, self->num_chord_regions, self->chord_regions_size,
    ZRegion *);
  for (int i = self->num_chord_regions; i > idx; i--)
    {
      self->chord_regions[i] = self->chord_regions[i - 1];
      self->chord_regions[i]->id.idx = i;
      region_update_identifier (self->chord_regions[i]);
    }
  self->num_chord_regions++;
  self->chord_regions[idx] = region;
  region->id.idx = idx;
  region_update_identifier (region);
}

/**
 * Inserts a scale to the track.
 */
void
chord_track_insert_scale (ChordTrack * self, ScaleObject * scale, int pos)
{
  g_warn_if_fail (self->type == TrackType::TRACK_TYPE_CHORD && scale);

  array_double_size_if_full (
    self->scales, self->num_scales, self->scales_size, ScaleObject *);
  array_insert (self->scales, self->num_scales, pos, scale);

  for (int i = pos; i < self->num_scales; i++)
    {
      ScaleObject * m = self->scales[i];
      scale_object_set_index (m, i);
    }

  EVENTS_PUSH (EventType::ET_ARRANGER_OBJECT_CREATED, scale);
}

/**
 * Adds a scale to the track.
 */
void
chord_track_add_scale (ChordTrack * self, ScaleObject * scale)
{
  chord_track_insert_scale (self, scale, self->num_scales);
}

/**
 * Returns the ScaleObject at the given Position
 * in the TimelineArranger.
 */
ScaleObject *
chord_track_get_scale_at_pos (const Track * ct, const Position * pos)
{
  ScaleObject *    scale = NULL;
  ArrangerObject * s_obj;
  for (int i = ct->num_scales - 1; i >= 0; i--)
    {
      scale = ct->scales[i];
      s_obj = (ArrangerObject *) scale;
      if (position_is_before_or_equal (&s_obj->pos, pos))
        return scale;
    }
  return NULL;
}

/**
 * Returns the ChordObject at the given Position
 * in the TimelineArranger.
 */
ChordObject *
chord_track_get_chord_at_pos (const Track * ct, const Position * pos)
{
  ZRegion * region = track_get_region_at_pos (ct, pos, false);

  if (!region)
    {
      return NULL;
    }

  signed_frame_t local_frames = (signed_frame_t)
    region_timeline_frames_to_local (region, pos->frames, F_NORMALIZE);

  ChordObject *    chord = NULL;
  ArrangerObject * c_obj;
  int              i;
  for (i = region->num_chord_objects - 1; i >= 0; i--)
    {
      chord = region->chord_objects[i];
      c_obj = (ArrangerObject *) chord;
      if (c_obj->pos.frames <= local_frames)
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
chord_track_clear (ChordTrack * self)
{
  g_return_if_fail (
    IS_TRACK (self) && self->type == TrackType::TRACK_TYPE_CHORD);

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

bool
chord_track_validate (Track * self)
{
  for (int i = 0; i < self->num_scales; i++)
    {
      ScaleObject * m = self->scales[i];
      g_return_val_if_fail (m->index == i, false);
    }

  for (int i = 0; i < self->num_chord_regions; i++)
    {
      ZRegion * r = self->chord_regions[i];
      g_return_val_if_fail (IS_REGION_AND_NONNULL (r), false);
      g_return_val_if_fail (chord_region_validate (r), false);
    }

  return true;
}

/**
 * Removes a scale from the chord Track.
 */
void
chord_track_remove_scale (ChordTrack * self, ScaleObject * scale, bool free)
{
  g_return_if_fail (IS_TRACK (self) && IS_SCALE_OBJECT (scale));

  /* deselect */
  arranger_object_select (
    (ArrangerObject *) scale, F_NO_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);

  int pos = -1;
  array_delete_return_pos (self->scales, self->num_scales, scale, pos);
  g_return_if_fail (pos >= 0);

  scale->index = -1;

  for (int i = pos; i < self->num_scales; i++)
    {
      ScaleObject * m = self->scales[i];
      scale_object_set_index (m, i);
    }

  if (free)
    {
      arranger_object_free ((ArrangerObject *) scale);
    }

  EVENTS_PUSH (
    EventType::ET_ARRANGER_OBJECT_REMOVED,
    ArrangerObjectType::ARRANGER_OBJECT_TYPE_SCALE_OBJECT);
}

/**
 * Removes a region from the chord track.
 */
void
chord_track_remove_region (ChordTrack * self, ZRegion * region)
{
  g_return_if_fail (IS_TRACK (self) && IS_REGION (region));

  array_delete (self->chord_regions, self->num_chord_regions, region);

  for (int i = region->id.idx; i < self->num_chord_regions; i++)
    {
      ZRegion * r = self->chord_regions[i];
      r->id.idx = i;
      region_update_identifier (r);
    }
}
