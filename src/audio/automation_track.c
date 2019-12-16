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

#include <math.h>

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/automation_point.h"
#include "audio/automation_region.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/backend/events.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/custom_button.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"

void
automation_track_init_loaded (
  AutomationTrack * self)
{
  self->automatable->track = self->track;
  automatable_init_loaded (self->automatable);

  /* init regions */
  self->regions_size =
    (size_t) self->num_regions;
  int i;
  Region * region;
  for (i = 0; i < self->num_regions; i++)
    {
      region = self->regions[i];
      region->at = self;
      arranger_object_init_loaded (
        (ArrangerObject *) region);
    }
}

AutomationTrack *
automation_track_new (Automatable *   a)
{
  g_warn_if_fail (a->track);

  AutomationTrack * self =
    calloc (1, sizeof (AutomationTrack));

  self->track = a->track;
  self->automatable = a;

  self->regions_size = 1;
  self->regions =
    malloc (self->regions_size *
            sizeof (Region *));

  self->height = TRACK_DEF_HEIGHT;

  return self;
}

/**
 * Adds an automation Region to the AutomationTrack.
 */
void
automation_track_add_region (
  AutomationTrack * self,
  Region *          region)
{
  g_return_if_fail (self);
  g_return_if_fail (
    region->type == REGION_TYPE_AUTOMATION);

  array_double_size_if_full (
    self->regions, self->num_regions,
    self->regions_size, Region *);
  array_append (self->regions,
                self->num_regions,
                region);

  region_set_automation_track (region, self);
}

AutomationTracklist *
automation_track_get_automation_tracklist (
  AutomationTrack * self)
{
  return
    track_get_automation_tracklist (self->track);
}

/**
 * Returns the Region that surrounds the
 * given Position, if any.
 */
Region *
automation_track_get_region_before_pos (
  const AutomationTrack * self,
  const Position *        pos)
{
  Region * region;
  ArrangerObject * r_obj;
  for (int i = self->num_regions - 1;
       i >= 0; i--)
    {
      region = self->regions[i];
      r_obj = (ArrangerObject *) region;
      if (position_is_before_or_equal (
            &r_obj->pos, pos) &&
          position_is_after_or_equal (
            &r_obj->end_pos, pos))
        return region;
    }
  return NULL;
}

/**
 * Returns the automation point before the Position
 * on the timeline.
 */
AutomationPoint *
automation_track_get_ap_before_pos (
  const AutomationTrack * self,
  const Position *        pos)
{
  Region * r =
    automation_track_get_region_before_pos (
      self, pos);

  if (!r)
    {
      return NULL;
    }

  long local_pos =
    region_timeline_frames_to_local (
      r, pos->frames, 1);

  AutomationPoint * ap;
  ArrangerObject * obj;
  for (int i = r->num_aps - 1; i >= 0; i--)
    {
      ap = r->aps[i];
      obj = (ArrangerObject *) ap;
      if (obj->pos.frames <= local_pos)
        return ap;
    }

  return NULL;
}

/**
 * Sets the index of the AutomationTrack in the
 * AutomationTracklist.
 */
void
automation_track_set_index (
  AutomationTrack * self,
  int               index)
{
  self->index = index;

  for (int i = 0; i < self->num_regions; i++)
    {
      g_return_if_fail (self->regions[i]);
      self->regions[i]->at_index = index;
    }

  Track * track = self->track;
  if (self->top_left_buttons[0] &&
      track && track->widget &&
      track->widget->layout)
    {
      char * text =
        g_strdup_printf (
          /*"%d - %s",*/
          "%s",
          /*self->index, */
          self->automatable->label);
      custom_button_widget_set_text (
        self->top_left_buttons[0],
        track->widget->layout, text);
      g_free (text);
    }
}

/**
 * Returns the normalized value (0.0-1.0) at the
 * given position (global).
 *
 * If there is no automation point/curve during
 * the position, it returns negative.
 */
float
automation_track_get_normalized_val_at_pos (
  AutomationTrack * self,
  Position *        pos)
{
  g_return_val_if_fail (self, 0.f);
  AutomationPoint * ap =
    automation_track_get_ap_before_pos (
      self, pos);
  ArrangerObject * ap_obj =
    (ArrangerObject *) ap;

  /* no automation points yet, return negative
   * (no change) */
  if (!ap)
    return -1.f;

  long localp =
    region_timeline_frames_to_local (
      ap->region, pos->frames, 1);

  AutomationPoint * next_ap =
    automation_region_get_next_ap (
      ap->region, ap);
  ArrangerObject * next_ap_obj =
    (ArrangerObject *) next_ap;
  /*g_message ("prev fvalue %f next %f",*/
             /*prev_ap->fvalue,*/
             /*next_ap->fvalue);*/

  /* return value at last ap */
  if (!next_ap)
    return ap->normalized_val;

  int prev_ap_lower =
    ap->normalized_val <= next_ap->normalized_val;
  float cur_next_diff =
    (float)
    fabsf (
      ap->normalized_val - next_ap->normalized_val);

  /* ratio of how far in we are in the curve */
  long ap_frames =
    position_to_frames (&ap_obj->pos);
  long next_ap_frames =
    position_to_frames (&next_ap_obj->pos);
  double ratio =
    (double) (localp - ap_frames) /
    (double) (next_ap_frames - ap_frames);
  /*g_message ("ratio %f",*/
             /*ratio);*/

  float result =
    (float)
    automation_point_get_normalized_value_in_curve (
      ap, ratio);
  result = result * cur_next_diff;
  /*g_message ("halfbaked result %f start at lower %d",*/
             /*result, prev_ap_lower);*/
  if (prev_ap_lower)
    result +=
      ap->normalized_val;
  else
    result +=
      next_ap->normalized_val;

  /*g_message ("result of %s: %f",*/
             /*self->automatable->label, result);*/
  return result;
}

/**
 * Updates the frames of each position in each child
 * of the automation track recursively.
 */
void
automation_track_update_frames (
  AutomationTrack * self)
{
  for (int i = 0; i < self->num_regions; i++)
    {
      arranger_object_update_frames (
        (ArrangerObject *) self->regions[i]);
    }
}

/**
 * Returns the y pixels from the value based on the
 * allocation of the automation track.
 */
int
automation_track_get_y_px_from_normalized_val (
  AutomationTrack * self,
  float             normalized_val)
{
  return self->height -
    (int) (normalized_val * (float) self->height);
}

/**
 * Gets the last Region in the AutomationTrack.
 *
 * FIXME cache.
 */
Region *
automation_track_get_last_region (
  AutomationTrack * self)
{
  Position pos;
  position_init (&pos);
  Region * region, * last_region = NULL;
  ArrangerObject * r_obj;
  for (int i = 0; i < self->num_regions; i++)
    {
      region = self->regions[i];
      r_obj = (ArrangerObject *) region;
      if (position_is_after (
            &r_obj->end_pos, &pos))
        {
          last_region = region;
          position_set_to_pos (
            &pos, &r_obj->end_pos);
        }
    }
  return last_region;
}

/**
 * Clones the AutomationTrack.
 */
AutomationTrack *
automation_track_clone (
  AutomationTrack * src)
{
  AutomationTrack * dest =
    calloc (1, sizeof (AutomationTrack));

  dest->regions_size = (size_t) src->num_regions;
  dest->num_regions = src->num_regions;
  dest->regions =
    malloc (dest->regions_size *
            sizeof (Region *));

  dest->automatable =
    automatable_clone (src->automatable);

  Region * src_region;
  for (int j = 0; j < src->num_regions; j++)
    {
      src_region = src->regions[j];
      dest->regions[j] =
        (Region *)
        arranger_object_clone (
          (ArrangerObject *) src_region,
          ARRANGER_OBJECT_CLONE_COPY_MAIN);
    }

  return dest;
}

void
automation_track_free (AutomationTrack * self)
{
  free (self);
}
