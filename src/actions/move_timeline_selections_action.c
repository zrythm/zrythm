/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/move_timeline_selections_action.h"
#include "audio/track.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"

#include <glib/gi18n.h>

/**
 * Creates a new MoveTimelineSelectionsAction.
 */
UndoableAction *
move_timeline_selections_action_new (
  TimelineSelections * ts,
  const long           ticks,
  const int            delta,
  const int            delta_lanes)
{
	MoveTimelineSelectionsAction * self =
    calloc (1, sizeof (
    	MoveTimelineSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_MOVE_TL_SELECTIONS;

  self->ts = timeline_selections_clone (ts);
  self->delta = delta;
  self->delta_lanes = delta_lanes;
  self->ticks = ticks;
  self->first_call = 1;

  return ua;
}

#define SHIFT_OBJ(cc,sc,ticks) \
  cc * sc; \
  for (i = 0; i < self->ts->num_##sc##s; i++) \
    { \
      /* get the actual object */ \
      sc = sc##_find (self->ts->sc##s[i]); \
      g_warn_if_fail (sc); \
      /* shift the actual object */ \
      sc##_shift_by_ticks ( \
        sc, ticks, AO_UPDATE_ALL); \
      /* also shift the copy */ \
      sc##_shift_by_ticks ( \
        self->ts->sc##s[i], ticks, AO_UPDATE_THIS); \
    }

int
move_timeline_selections_action_do (
	MoveTimelineSelectionsAction * self)
{
  int i;

#define SHIFT_OBJ_POSITIVE(cc,sc) \
  SHIFT_OBJ (cc, sc, self->ticks)

  if (!self->first_call)
    {
      SHIFT_OBJ_POSITIVE (
        Region, region);
      SHIFT_OBJ_POSITIVE (
        ScaleObject, scale_object);
      SHIFT_OBJ_POSITIVE (
        Marker, marker);
    }

  if (self->delta)
    {
      Region * region = NULL;
      for (i = 0; i < self->ts->num_regions; i++)
        {
          /* get the actual object */
          region =
            region_find (self->ts->regions[i]);
          g_warn_if_fail (region);

          Track * track_to_move_to =
            tracklist_get_visible_track_after_delta (
              TRACKLIST,
              region_get_track (region),
              self->delta);

          /* shift the actual object by tracks */
          region_move_to_track (
            region, track_to_move_to, 0);
        }
    }
  else if (self->delta_lanes)
    {
      Region * region = NULL;
      for (i = 0; i < self->ts->num_regions; i++)
        {
          /* get the actual object */
          region =
            region_find (self->ts->regions[i]);
          g_warn_if_fail (region);

          Track * region_track =
            region_get_track (region);
          track_create_missing_lanes (
            region_track,
            region->lane->pos + self->delta_lanes);
          TrackLane * lane_to_move_to =
            region_track->lanes[
              region->lane->pos + self->delta_lanes];

          /* shift the actual object by lanes */
          region_move_to_lane (
            region, lane_to_move_to, 0);
        }
    }

  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);

  self->first_call = 0;

  return 0;
}

int
move_timeline_selections_action_undo (
	MoveTimelineSelectionsAction * self)
{
  int i;

#define SHIFT_OBJ_NEGATIVE(cc,sc) \
  SHIFT_OBJ (cc, sc, - self->ticks)

  SHIFT_OBJ_NEGATIVE (
    Region, region);
  SHIFT_OBJ_NEGATIVE (
    ScaleObject, scale_object);
  SHIFT_OBJ_NEGATIVE (
    Marker, marker);
  if (self->delta)
    {
      for (i = 0; i < self->ts->num_regions; i++)
        {
          /* get the actual object */
          region =
            region_find (self->ts->regions[i]);
          g_warn_if_fail (region);

          Track * track_to_move_to =
            tracklist_get_visible_track_after_delta (
              TRACKLIST,
              region_get_track (region),
              - self->delta);

          /* shift the actual object by tracks */
          region_move_to_track (
            region, track_to_move_to, 0);
        }
    }
  else if (self->delta_lanes)
    {
      for (i = 0; i < self->ts->num_regions; i++)
        {
          /* get the actual object */
          region =
            region_find (self->ts->regions[i]);
          g_warn_if_fail (region);

          Track * region_track =
            region_get_track (region);
          track_create_missing_lanes (
            region_track,
            region->lane->pos - self->delta_lanes);
          TrackLane * lane_to_move_to =
            region_track->lanes[
              region->lane->pos - self->delta_lanes];

          /* shift the actual object by lanes */
          region_move_to_lane (
            region, lane_to_move_to, 0);
        }
    }

  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

char *
move_timeline_selections_action_stringize (
	MoveTimelineSelectionsAction * self)
{
  return g_strdup (
    _("Move Object(s)"));
}

void
move_timeline_selections_action_free (
	MoveTimelineSelectionsAction * self)
{
  timeline_selections_free (self->ts);

  free (self);
}
