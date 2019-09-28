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

#include "actions/duplicate_timeline_selections_action.h"
#include "audio/chord_track.h"
#include "audio/chord_object.h"
#include "audio/marker.h"
#include "audio/marker_track.h"
#include "audio/scale_object.h"
#include "audio/track.h"
#include "gui/backend/timeline_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/region.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/timeline_arranger.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
duplicate_timeline_selections_action_new (
  TimelineSelections * ts,
  long                 ticks,
  int                  delta)
{
  DuplicateTimelineSelectionsAction * self =
    calloc (1, sizeof (
                 DuplicateTimelineSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_DUPLICATE_TL_SELECTIONS;

  self->ts = timeline_selections_clone (ts);
  self->ticks = ticks;
  self->delta = delta;

  return ua;
}

#define DO_OBJECT( \
  caps,cc,sc,add_to_track_code,remember_code) \
  cc * sc; \
	for (i = 0; i < self->ts->num_##sc##s; i++) \
    { \
      /* clone the clone */ \
      sc = \
        sc##_clone ( \
          self->ts->sc##s[i], \
          caps##_CLONE_COPY_MAIN); \
      /* add to track */ \
      add_to_track_code; \
      /* shift */ \
      sc##_shift_by_ticks ( \
        sc, self->ticks, AO_UPDATE_ALL); \
      /* shift the clone too so we can find it
       * when undoing */ \
      sc##_shift_by_ticks ( \
        self->ts->sc##s[i], self->ticks, \
        AO_UPDATE_THIS); \
      /* select it */ \
      sc##_select ( \
        sc, F_SELECT); \
      remember_code; \
    }

#define UNDO_OBJECT(cc,sc,remove_code) \
  cc * sc; \
	for (i = 0; i < self->ts->num_##sc##s; i++) \
    { \
      /* find the actual object */ \
      sc = \
        sc##_find ( \
          self->ts->sc##s[i]); \
      /* unselect it */ \
      sc##_select ( \
        sc, F_NO_SELECT); \
      /* remove it */ \
      remove_code; \
      /* unshift the clone */ \
      sc##_shift_by_ticks ( \
        self->ts->sc##s[i], - self->ticks, \
        AO_UPDATE_ALL); \
    }

int
duplicate_timeline_selections_action_do (
  DuplicateTimelineSelectionsAction * self)
{
  int i;

  DO_OBJECT (
    REGION, Region, region,
    /* add */
    track_add_region (
      TRACKLIST->tracks[region->track_pos],
      region, NULL, 0, F_GEN_NAME,
      F_PUBLISH_EVENTS),
    /* remember the new name */
    g_free (self->ts->regions[i]->name);
    self->ts->regions[i]->name =
      g_strdup (region->name));

  DO_OBJECT (
    SCALE_OBJECT, ScaleObject, scale_object,
    /* add */
    chord_track_add_scale (
      P_CHORD_TRACK, scale_object),);

  DO_OBJECT (
    MARKER, Marker, marker,
    /* add */
    marker_track_add_marker (
      P_MARKER_TRACK, marker),
    /* remember the new name */
    g_free (self->ts->markers[i]->name);
    self->ts->markers[i]->name =
      g_strdup (marker->name));

  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

int
duplicate_timeline_selections_action_undo (
  DuplicateTimelineSelectionsAction * self)
{
  int i;

  UNDO_OBJECT (
    Region, region,
    /* remove */
    track_remove_region (
      region->lane->track,
      region, F_PUBLISH_EVENTS, F_FREE));
  UNDO_OBJECT (
    ScaleObject, scale_object,
    /* remove */
    chord_track_remove_scale (
      P_CHORD_TRACK,
      scale_object, F_FREE));
  UNDO_OBJECT (
    Marker, marker,
    /* remove */
    marker_track_remove_marker (
      P_MARKER_TRACK,
      marker, F_FREE));

  EVENTS_PUSH (ET_TL_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

char *
duplicate_timeline_selections_action_stringize (
  DuplicateTimelineSelectionsAction * self)
{
  return g_strdup (
    _("Duplicate Object(s)"));
}

void
duplicate_timeline_selections_action_free (
  DuplicateTimelineSelectionsAction * self)
{
  timeline_selections_free (self->ts);

  free (self);
}
