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

  Region * region;
	for (i = 0; i < self->ts->num_regions; i++)
    {
      /* check if the region already exists. due to
       * how the arranger creates regions, the region
       * should already exist the first time so no
       * need to do anything. when redoing we will
       * need to create a clone instead */
      if (region_find (self->ts->regions[i]))
        continue;

      /* clone the clone */
      region =
        region_clone (
          self->ts->regions[i],
          REGION_CLONE_COPY_MAIN);
      g_return_val_if_fail (
        region->track_pos >= 0, -1);

      /* add it to track */
      track_add_region (
        TRACKLIST->tracks[region->track_pos],
        region, 0, F_GEN_NAME, F_GEN_WIDGET);

      /* remember its name */
      g_free (self->ts->regions[i]->name);
      self->ts->regions[i]->name =
        g_strdup (region->name);
    }
  ChordObject * chord;
	for (i = 0;
       i < self->ts->num_chord_objects; i++)
    {
      /* check if the region already exists. due to
       * how the arranger creates regions, the region
       * should already exist the first time so no
       * need to do anything. when redoing we will
       * need to create a clone instead */
      if (chord_object_find (
            self->ts->chord_objects[i]))
        continue;

      /* clone the clone */
      chord =
        chord_object_clone (
          self->ts->chord_objects[i],
          CHORD_OBJECT_CLONE_COPY_MAIN);

      /* add it to track */
      chord_track_add_chord (
        P_CHORD_TRACK,
        chord, F_GEN_WIDGET);
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
      if (scale_object_find (
            self->ts->scale_objects[i]))
        continue;

      /* clone the clone */
      scale =
        scale_object_clone (
          self->ts->scale_objects[i],
          SCALE_OBJECT_CLONE_COPY_MAIN);

      /* add it to track */
      chord_track_add_scale (
        P_CHORD_TRACK,
        scale, F_GEN_WIDGET);
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
      if (marker_find (
            self->ts->markers[i]))
        continue;

      /* clone the clone */
      marker =
        marker_clone (
          self->ts->markers[i],
          MARKER_CLONE_COPY_MAIN);

      /* add it to track */
      marker_track_add_marker (
        P_MARKER_TRACK,
        marker, F_GEN_WIDGET);
    }
  AutomationPoint * ap;
	for (i = 0;
       i < self->ts->num_automation_points; i++)
    {
      /* check if the ap already exists. due to
       * how the arranger creates regions, the region
       * should already exist the first time so no
       * need to do anything. when redoing we will
       * need to create a clone instead */
      if (automation_point_find (
            self->ts->automation_points[i]))
        continue;

      g_message ("NOT FOUND");

      /* clone the clone */
      ap =
        automation_point_clone (
          self->ts->automation_points[i],
          AUTOMATION_POINT_CLONE_COPY_MAIN);

      /* add it to track */
      automation_track_add_ap (
        ap->at, ap, F_GEN_WIDGET,
        F_GEN_CURVE_POINTS);

      /* remember new index */
      automation_point_set_automation_track_and_index (
        self->ts->automation_points[i],
        ap->at, ap->index);
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
        region->lane->track, region, F_FREE);
    }
  ChordObject * chord_object;
  for (i = 0; i < self->ts->num_chord_objects; i++)
    {
      /* get the actual region */
      chord_object =
        chord_object_find (
          self->ts->chord_objects[i]);

      /* remove it */
      chord_track_remove_chord (
        P_CHORD_TRACK, chord_object, F_FREE);
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
  AutomationPoint * ap;
  for (i = 0;
       i < self->ts->num_automation_points; i++)
    {
      /* get the actual region */
      ap =
        automation_point_find (
          self->ts->automation_points[i]);
      g_warn_if_fail (ap);

      /* remove it */
      automation_track_remove_ap (
        ap->at, ap, F_FREE);
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
