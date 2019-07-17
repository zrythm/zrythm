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
#include "audio/automation_curve.h"
#include "audio/automation_track.h"
#include "audio/automation_point.h"
#include "audio/automation_region.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/backend/events.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
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
  g_return_if_fail (
    region->type == REGION_TYPE_AUTOMATION);

  array_double_size_if_full (
    self->regions, self->num_regions,
    self->regions_size, Region *);
  array_append (self->regions,
                self->num_regions,
                region);
}


/**
 * Returns the automation curve at the given pos.
 *
 * This is supposed to be global so it belongs here
 * instead of the automation Region.
 */
AutomationCurve *
automation_track_get_ac_at_pos (
  AutomationTrack * self,
  Position *        pos)
{
  AutomationPoint * prev_ap =
    automation_track_get_ap_before_pos (
      self, pos);
  if (!prev_ap)
    return NULL;
  AutomationPoint * next_ap =
    automation_region_get_next_ap (
      prev_ap->region, prev_ap);
  if (!next_ap)
    return NULL;

  return
    automation_region_get_next_curve_ac (
      next_ap->region, prev_ap);
}

/**
 * Returns the last Region before the given
 * Position.
 */
Region *
automation_track_get_region_before_pos (
  const AutomationTrack * self,
  const Position *        pos)
{
  Region * region;
  for (int i = 0; i < self->num_regions; i++)
    {
      region = self->regions[i];

      if (position_is_before_or_equal (
            &region->start_pos, pos) &&
          position_is_after_or_equal (
            &region->end_pos, pos))
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

  Position local_pos;
  region_timeline_pos_to_local (
    r, pos, &local_pos, 1);

  AutomationPoint * ap;
  for (int i = r->num_aps - 1; i >= 0; i--)
    {
      ap = r->aps[i];

      if (position_is_before_or_equal (
            &ap->pos, &local_pos))
        return ap;
    }

  return NULL;
}

/**
 * Returns the normalized value (0.0-1.0) at the
 * given position.
 *
 * If there is no automation point/curve during
 * the position, it returns negative.
 */
float
automation_track_get_normalized_val_at_pos (
  AutomationTrack * self,
  Position *        pos)
{
  AutomationCurve * ac =
    automation_track_get_ac_at_pos (
      self, pos);
  AutomationPoint * prev_ap =
    automation_track_get_ap_before_pos (
      self, pos);

  /* no automation points yet, return negative
   * (no change) */
  if (!ac && !prev_ap)
    return -1.f;

  AutomationPoint * next_ap =
    automation_region_get_next_ap (
      prev_ap->region, prev_ap);
  /*g_message ("prev fvalue %f next %f",*/
             /*prev_ap->fvalue,*/
             /*next_ap->fvalue);*/
  float prev_ap_normalized_val =
    automation_point_get_normalized_value (
      prev_ap);

  /* return value at last ap */
  if (!next_ap)
    return prev_ap_normalized_val;

  float next_ap_normalized_val =
    automation_point_get_normalized_value (
      next_ap);
  int prev_ap_lower =
    prev_ap_normalized_val <= next_ap_normalized_val;
  float prev_next_diff =
    fabs (prev_ap_normalized_val -
          next_ap_normalized_val);

  /* ratio of how far in we are in the curve */
  int pos_ticks = position_to_ticks (pos);
  int prev_ap_ticks =
    position_to_ticks (&prev_ap->pos);
  int next_ap_ticks =
    position_to_ticks (&next_ap->pos);
  double ratio =
    (double) (pos_ticks - prev_ap_ticks) /
    (next_ap_ticks - prev_ap_ticks);
  /*g_message ("ratio %f",*/
             /*ratio);*/

  double result =
    automation_curve_get_y_normalized (
      ratio, ac->curviness, !prev_ap_lower);
  result = result * prev_next_diff;
  /*g_message ("halfbaked result %f start at lower %d",*/
             /*result, prev_ap_lower);*/
  if (prev_ap_lower)
    result +=
      prev_ap_normalized_val;
  else
    result +=
      next_ap_normalized_val;

  /*g_message ("result of %s: %f",*/
             /*self->automatable->label, result);*/
  return result;
}

/**
 * Gets the last Region in the AutomationTrack.
 */
Region *
automation_track_get_last_region (
  AutomationTrack * self)
{
  Position pos;
  position_init (&pos);
  Region * region, * last_region = NULL;
  for (int i = 0; i < self->num_regions; i++)
    {
      region = self->regions[i];

      if (position_is_after (
            &region->end_pos, &pos))
        {
          last_region = region;
          position_set_to_pos (
            &pos, &region->end_pos);
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

  dest->regions_size = src->num_regions;
  dest->num_regions = src->num_regions;
  dest->regions =
    malloc (dest->regions_size *
            sizeof (Region *));

  Region * src_region;
  for (int j = 0; j < src->num_regions; j++)
    {
      src_region = src->regions[j];
      dest->regions[j] =
        region_clone (
          src_region, REGION_CLONE_COPY_MAIN);
    }

  return dest;
}

void
automation_track_free (AutomationTrack * self)
{
  if (self->widget && GTK_IS_WIDGET (self->widget))
    gtk_widget_destroy (GTK_WIDGET (self->widget));

  free (self);
}
