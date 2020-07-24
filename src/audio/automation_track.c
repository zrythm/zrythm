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

#include <math.h>

#include "audio/automation_track.h"
#include "audio/automation_point.h"
#include "audio/automation_region.h"
#include "audio/control_port.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/backend/event_manager.h"
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
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
automation_track_init_loaded (
  AutomationTrack * self)
{
  /* init regions */
  self->regions_size =
    (size_t) self->num_regions;
  int i;
  ZRegion * region;
  for (i = 0; i < self->num_regions; i++)
    {
      region = self->regions[i];
      arranger_object_init_loaded (
        (ArrangerObject *) region);
    }
}

AutomationTrack *
automation_track_new (
  Port * port)
{
  AutomationTrack * self =
    calloc (1, sizeof (AutomationTrack));

  self->regions_size = 1;
  self->regions =
    malloc (self->regions_size *
            sizeof (ZRegion *));
  self->automation_mode = AUTOMATION_MODE_READ;

  self->height = TRACK_DEF_HEIGHT;

  port_identifier_copy (
    &self->port_id, &port->id);

  port->at = self;

  return self;
}

/**
 * Gets the automation mode as a localized string.
 */
void
automation_mode_get_localized (
  AutomationMode mode,
  char *         buf)
{
  switch (mode)
    {
    case AUTOMATION_MODE_READ:
      strcpy (buf, _("On"));
      break;
    case AUTOMATION_MODE_RECORD:
      strcpy (buf, _("Rec"));
      break;
    case AUTOMATION_MODE_OFF:
      strcpy (buf, _("Off"));
      break;
    default:
      g_return_if_reached ();
    }
}

/**
 * Gets the automation mode as a localized string.
 */
void
automation_record_mode_get_localized (
  AutomationRecordMode mode,
  char *         buf)
{
  switch (mode)
    {
    case AUTOMATION_RECORD_MODE_TOUCH:
      strcpy (buf, _("Touch"));
      break;
    case AUTOMATION_RECORD_MODE_LATCH:
      strcpy (buf, _("Latch"));
      break;
    default:
      g_return_if_reached ();
    }
}

/**
 * Adds an automation ZRegion to the AutomationTrack.
 *
 * @note This must not be used directly. Use
 *   track_add_region() instead.
 */
void
automation_track_add_region (
  AutomationTrack * self,
  ZRegion *          region)
{
  g_return_if_fail (self);
  g_return_if_fail (
    region->id.type == REGION_TYPE_AUTOMATION);

  array_double_size_if_full (
    self->regions, self->num_regions,
    self->regions_size, ZRegion *);
  array_append (self->regions,
                self->num_regions,
                region);

  region_set_automation_track (region, self);
  region->id.idx = self->num_regions - 1;
  region_update_identifier (region);
}

AutomationTracklist *
automation_track_get_automation_tracklist (
  AutomationTrack * self)
{
  Track * track =
    automation_track_get_track (self);
  return
    track_get_automation_tracklist (track);
}

/**
 * Returns the ZRegion that surrounds the
 * given Position, if any.
 */
ZRegion *
automation_track_get_region_before_pos (
  const AutomationTrack * self,
  const Position *        pos)
{
  ZRegion * region;
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
  ZRegion * r =
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
 * Finds the AutomationTrack associated with
 * `port`.
 *
 * @param track The track that owns the port, if
 *   known.
 */
AutomationTrack *
automation_track_find_from_port (
  Port *  port,
  Track * track,
  bool    basic_search)
{
  if (!track)
    {
      track = port_get_track (port, 1);
    }
  g_return_val_if_fail (track, NULL);

  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  g_return_val_if_fail (atl, NULL);
  for (int i = 0; i < atl->num_ats; i++)
    {
      AutomationTrack * at = atl->ats[i];
      if (basic_search)
        {
          PortIdentifier * src = &port->id;
          PortIdentifier * dest = &at->port_id;
          if (
            string_is_equal (
              dest->label, src->label, 0) &&
            dest->owner_type == src->owner_type &&
            dest->type == src->type &&
            dest->flow == src->flow &&
            dest->flags == src->flags &&
            dest->track_pos == src->track_pos)
            {
              return at;
            }
        }
      else if (port_identifier_is_equal (
            &port->id, &at->port_id))
        {
          return at;
        }
    }

  return NULL;
}

/**
 * @note This is expensive and should only be used
 *   if \ref PortIdentifier.at_idx is not set. Use
 *   port_get_automation_track() instead.
 *
 * @param basic_search If true, only basic port
 *   identifier members are checked.
 */
AutomationTrack *
automation_track_find_from_port_id (
  PortIdentifier * id,
  bool             basic_search)
{
  Port * port = port_find_from_identifier (id);
  g_return_val_if_fail (
    port &&
      port_identifier_is_equal (id, &port->id),
    NULL);

  return
    automation_track_find_from_port (
      port, NULL, basic_search);
}

/**
 * Returns whether the automation in the automation
 * track should be read.
 *
 * @param cur_time Current time from
 *   g_get_monotonic_time() passed here to ensure
 *   the same timestamp is used in sequential calls.
 */
bool
automation_track_should_read_automation (
  AutomationTrack * at,
  gint64            cur_time)
{
  g_return_val_if_fail (at, false);
  if (at->automation_mode == AUTOMATION_MODE_OFF)
    return false;

  /* TODO last argument should be true but doesnt
   * work properly atm */
  if (automation_track_should_be_recording (
        at, cur_time, false))
    return false;

  return true;
}

/**
 * Returns if the automation track should currently
 * be recording data.
 *
 * Returns false if in touch mode after the release
 * time has passed.
 *
 * This function assumes that the transport will
 * be rolling.
 *
 * @param cur_time Current time from
 *   g_get_monotonic_time() passed here to ensure
 *   the same timestamp is used in sequential calls.
 * @param record_aps If set to true, this function
 *   will return whether we should be recording
 *   automation point data. If set to false, this
 *   function will return whether we should be
 *   recording a region (eg, if an automation point
 *   was already created before and we are still
 *   recording inside a region regardless of whether
 *   we should create/edit automation points or not.
 */
bool
automation_track_should_be_recording (
  AutomationTrack * at,
  gint64            cur_time,
  bool              record_aps)
{
  if (at->automation_mode == AUTOMATION_MODE_RECORD)
    {
      if (at->record_mode ==
            AUTOMATION_RECORD_MODE_LATCH)
        {
          /* in latch mode, we are always recording,
           * even if the value doesn't change
           * (an automation point will be created
           * as soon as latch mode is armed) and
           * then only when changes are made) */
          return true;
        }
      if (at->record_mode ==
            AUTOMATION_RECORD_MODE_TOUCH)
        {
          Port * port =
            automation_track_get_port (at);
          gint64 diff =
            cur_time - port->last_change;
          if (diff <
                AUTOMATION_RECORDING_TOUCH_REL_MS * 1000)
            {
              /* still recording */
              return true;
            }
          else if (!record_aps)
            return at->recording_started;
        }
    }

  return false;
}

/**
 * Unselects all arranger objects.
 */
void
automation_track_unselect_all (
  AutomationTrack * self)
{
  for (int i = 0; i < self->num_regions; i++)
    {
      ZRegion * region = self->regions[i];
      arranger_object_select (
        (ArrangerObject *) region, false, false);
    }
}

/**
 * Removes all arranger objects recursively.
 */
void
automation_track_clear (
  AutomationTrack * self)
{
  for (int i = self->num_regions - 1; i >= 0; i--)
    {
      ZRegion * region = self->regions[i];
      Track * track =
        automation_track_get_track (self);
      track_remove_region (
        track, region, 0, 1);
    }
}

Track *
automation_track_get_track (
  AutomationTrack * self)
{
  g_return_val_if_fail (
    self &&
      self->port_id.track_pos <
        TRACKLIST->num_tracks,
    NULL);
  Track * track =
    TRACKLIST->tracks[self->port_id.track_pos];
  g_return_val_if_fail (track, NULL);

  return track;
}

Port *
automation_track_get_port (
  AutomationTrack * self)
{
  Port * port =
    port_find_from_identifier (&self->port_id);

  return port;
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
      ZRegion * region = self->regions[i];
      g_return_if_fail (region);
      region->id.at_idx = index;
      region_update_identifier (region);
    }
}

/**
 * Returns the actual parameter value at the given
 * position.
 *
 * If there is no automation point/curve during
 * the position, it returns the current value
 * of the parameter it is automating.
 *
 * @param normalized Whether to return the value
 *   normalized.
 */
float
automation_track_get_val_at_pos (
  AutomationTrack * self,
  Position *        pos,
  bool              normalized)
{
  g_return_val_if_fail (self, 0.f);
  AutomationPoint * ap =
    automation_track_get_ap_before_pos (
      self, pos);
  ArrangerObject * ap_obj =
    (ArrangerObject *) ap;

  Port * port =
    automation_track_get_port (self);
  g_return_val_if_fail (port, 0.f);

  /* no automation points yet, return negative
   * (no change) */
  if (!ap)
    {
      return
        port_get_control_value (port, normalized);
    }

  ZRegion * region =
    arranger_object_get_region (ap_obj);
  long localp =
    region_timeline_frames_to_local (
      region, pos->frames, true);

  AutomationPoint * next_ap =
    automation_region_get_next_ap (
      region, ap, false, false);
  ArrangerObject * next_ap_obj =
    (ArrangerObject *) next_ap;

  /* return value at last ap */
  if (!next_ap)
    {
      if (normalized)
        {
          return ap->normalized_val;
        }
      else
        {
          return ap->fvalue;
        }
    }

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
  g_return_val_if_fail (ratio >= 0, 0.f);

  float result =
    (float)
    automation_point_get_normalized_value_in_curve (
      ap, ratio);
  result = result * cur_next_diff;
  if (prev_ap_lower)
    result +=
      ap->normalized_val;
  else
    result +=
      next_ap->normalized_val;

  if (normalized)
    {
      return result;
    }
  else
    {
      return
        control_port_normalized_val_to_real (
          port, result);
    }
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
 * Gets the last ZRegion in the AutomationTrack.
 *
 * FIXME cache.
 */
ZRegion *
automation_track_get_last_region (
  AutomationTrack * self)
{
  Position pos;
  position_init (&pos);
  ZRegion * region, * last_region = NULL;
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
    malloc (
      dest->regions_size * sizeof (ZRegion *));
  dest->visible = src->visible;
  dest->created = src->created;
  dest->index = src->index;
  dest->y = src->y;
  dest->automation_mode = src->automation_mode;
  dest->record_mode = src->record_mode;
  dest->height = src->height;
  g_warn_if_fail (dest->height >= TRACK_MIN_HEIGHT);

  port_identifier_copy (
    &dest->port_id, &src->port_id);

  ZRegion * src_region;
  for (int j = 0; j < src->num_regions; j++)
    {
      src_region = src->regions[j];
      dest->regions[j] =
        (ZRegion *)
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
