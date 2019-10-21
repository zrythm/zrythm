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

#include "actions/create_timeline_selections_action.h"
#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "audio/marker.h"
#include "audio/marker_track.h"
#include "audio/scale_object.h"
#include "audio/track.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

/**
 * The given TimelineSelections must already
 * contain the created selections in the transient
 * arrays.
 *
 * Note: chord addresses are to be copied.
 */
UndoableAction *
create_timeline_selections_action_new (
  TimelineSelections * ts)
{
  CreateTimelineSelectionsAction * self =
    calloc (1, sizeof (
                 CreateTimelineSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_CREATE_TL_SELECTIONS;

  self->ts = timeline_selections_clone (ts);

  return ua;
}

int
create_timeline_selections_action_do (
  CreateTimelineSelectionsAction * self)
{
  int i;

  timeline_selections_clear (TL_SELECTIONS);

  Track * track;
  AutomationTrack * at;
  Region * region;
	for (i = 0; i < self->ts->num_regions; i++)
    {
      /* check if the region already exists. due to
       * how the arranger creates regions, the region
       * should already exist the first time so no
       * need to do anything. when redoing we will
       * need to create a clone instead */
      Region * found =
        region_find (self->ts->regions[i]);
      if (found)
        {
          region_select (found, 1);
          continue;
        }

      /* clone the clone */
      region =
        region_clone (
          self->ts->regions[i],
          REGION_CLONE_COPY_MAIN);
      g_return_val_if_fail (
        region->track_pos >= 0, -1);

      /* add it to track */
      track =
        TRACKLIST->tracks[region->track_pos];
      if (region->type == REGION_TYPE_AUTOMATION)
        {
          at =
            track->automation_tracklist.
              ats[region->at_index];
          track_add_region (
            track, region, at, -1, F_GEN_NAME,
            F_PUBLISH_EVENTS);
        }
      else if (region->type == REGION_TYPE_CHORD)
        {
          track_add_region (
            P_CHORD_TRACK, region, NULL, -1,
            F_GEN_NAME, F_PUBLISH_EVENTS);
        }
      else
        {
          track_add_region (
            track, region, NULL, region->lane_pos,
            F_GEN_NAME,
            F_PUBLISH_EVENTS);
        }

      /* select it */
      region_select (region, 1);

      /* remember its name */
      g_free (self->ts->regions[i]->name);
      self->ts->regions[i]->name =
        g_strdup (region->name);
    }
  ScaleObject * scale;
	for (i = 0;
       i < self->ts->num_scale_objects; i++)
    {
      /* check if the region already exists. due to
       * how the arranger creates regions, the region
       * should already exist the first time so no
       * need to do anything. when redoing we will
       * need to create a clone instead */
      ScaleObject * found =
        scale_object_find (
          self->ts->scale_objects[i]);
      if (found)
        {
          scale_object_select (found, 1);
          continue;
        }

      /* clone the clone */
      scale =
        scale_object_clone (
          self->ts->scale_objects[i],
          SCALE_OBJECT_CLONE_COPY_MAIN);

      /* add it to track */
      chord_track_add_scale (
        P_CHORD_TRACK, scale);

      /* select it */
      scale_object_select (scale, 1);
    }
  Marker * marker;
	for (i = 0;
       i < self->ts->num_markers; i++)
    {
      /* check if the region already exists. due to
       * how the arranger creates regions, the region
       * should already exist the first time so no
       * need to do anything. when redoing we will
       * need to create a clone instead */
      Marker * found =
        marker_find (
          self->ts->markers[i]);
      if (found)
        {
          marker_select (found, 1);
          continue;
        }

      /* clone the clone */
      marker =
        marker_clone (
          self->ts->markers[i],
          MARKER_CLONE_COPY_MAIN);

      /* add it to track */
      marker_track_add_marker (
        P_MARKER_TRACK, marker);

      /* select it */
      marker_select (marker, 1);
    }

  EVENTS_PUSH (ET_TL_SELECTIONS_CREATED,
               NULL);

  return 0;
}

int
create_timeline_selections_action_undo (
  CreateTimelineSelectionsAction * self)
{
  int i;
  Region * region;
  for (i = 0; i < self->ts->num_regions; i++)
    {
      /* get the actual region */
      region = region_find (self->ts->regions[i]);

      /* remove it */
      track_remove_region (
        region_get_track (region), region,
        F_PUBLISH_EVENTS, F_FREE);
    }
  ScaleObject * scale_object;
  for (i = 0; i < self->ts->num_scale_objects; i++)
    {
      /* get the actual region */
      scale_object =
        scale_object_find (
          self->ts->scale_objects[i]);

      /* remove it */
      chord_track_remove_scale (
        P_CHORD_TRACK, scale_object, F_FREE);
    }
  Marker * marker;
  for (i = 0; i < self->ts->num_markers; i++)
    {
      /* get the actual region */
      marker = marker_find (self->ts->markers[i]);

      /* remove it */
      marker_track_remove_marker (
        P_MARKER_TRACK, marker, F_FREE);
    }
  EVENTS_PUSH (ET_TL_SELECTIONS_REMOVED,
               NULL);

  return 0;
}

char *
create_timeline_selections_action_stringize (
  CreateTimelineSelectionsAction * self)
{
  return g_strdup (
    _("Create Object(s)"));
}

void
create_timeline_selections_action_free (
  CreateTimelineSelectionsAction * self)
{
  timeline_selections_free (self->ts);

  free (self);
}
