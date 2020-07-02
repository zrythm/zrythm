/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/chord_track.h"
#include "audio/engine.h"
#include "audio/position.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/midi_region.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"

#include <gtk/gtk.h>

void
tracklist_selections_init_loaded (
  TracklistSelections * self)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];
      int track_pos = track->pos;
      track_init_loaded (track, false);
      if (self->is_project)
        {
          self->tracks[i] =
            TRACKLIST->tracks[track_pos];
          /* TODO */
          /*track_disconnect (track, true, false);*/
          /*track_free (track);*/
        }
    }
}

/**
 * Gets highest track in the selections.
 */
Track *
tracklist_selections_get_highest_track (
  TracklistSelections * ts)
{
  Track * track;
  int min_pos = 1000;
  Track * min_track = NULL;
  for (int i = 0; i < ts->num_tracks; i++)
    {
      track = ts->tracks[i];

      if (track->pos < min_pos)
        {
          min_pos = track->pos;
          min_track = track;
        }
    }

  return min_track;
}

/**
 * Gets lowest track in the selections.
 */
Track *
tracklist_selections_get_lowest_track (
  TracklistSelections * ts)
{
  Track * track;
  int max_pos = -1;
  Track * max_track = NULL;
  for (int i = 0; i < ts->num_tracks; i++)
    {
      track = ts->tracks[i];

      if (track->pos > max_pos)
        {
          max_pos = track->pos;
          max_track = track;
        }
    }

  return max_track;
}

/**
 * @param is_project Whether these selections are
 *   the project selections (as opposed to clones).
 */
TracklistSelections *
tracklist_selections_new (
  bool  is_project)
{
  TracklistSelections * self =
    object_new (TracklistSelections);

  self->is_project = is_project;

  return self;
}

/**
 * Adds a Track to the selections.
 */
void
tracklist_selections_add_track (
  TracklistSelections * self,
  Track *               track,
  bool                  fire_events)
{
  if (!array_contains (self->tracks,
                      self->num_tracks,
                      track))
    {
      array_append (self->tracks,
                    self->num_tracks,
                    track);

      if (fire_events)
        {
          EVENTS_PUSH (ET_TRACK_CHANGED, track);
          EVENTS_PUSH (
            ET_TRACKLIST_SELECTIONS_CHANGED, NULL);
        }
    }

  /* if recording is not already
   * on, turn these on */
  if (!track->recording && track->channel)
    {
      track_set_recording (track, true, fire_events);
      track->channel->record_set_automatically =
        true;
    }
}

void
tracklist_selections_add_tracks_in_range (
  TracklistSelections * self,
  int                   min_pos,
  int                   max_pos,
  bool                  fire_events)
{
  g_message (
    "selecting tracks from %d to %d...",
    min_pos, max_pos);

  tracklist_selections_clear (self);

  for (int i = min_pos; i <= max_pos; i++)
    {
      Track * track = TRACKLIST->tracks[i];
      tracklist_selections_add_track (
        self, track, fire_events);
    }

  g_message ("done");
}

void
tracklist_selections_clear (
  TracklistSelections * self)
{
  for (int i = self->num_tracks - 1; i >= 0; i--)
    {
      Track * track = self->tracks[i];
      tracklist_selections_remove_track (
        self, track, 0);

      if (track->is_project)
        {
          EVENTS_PUSH (ET_TRACK_CHANGED, track);
        }
    }

  if (self->is_project)
    {
      EVENTS_PUSH (
        ET_TRACKLIST_SELECTIONS_CHANGED, NULL);
    }
}

/**
 * Handle a click selection.
 */
void
tracklist_selections_handle_click (
  Track * track,
  bool    ctrl,
  bool    shift,
  bool    dragged)
{
  bool is_selected = track_is_selected (track);
  if (is_selected)
    {
      if ((ctrl || shift) && !dragged)
        {
          track_select (
            track, F_NO_SELECT, F_NOT_EXCLUSIVE,
            F_PUBLISH_EVENTS);
        }
      else
        {
          /* do nothing */
        }
    }
  else /* not selected */
    {
      if (shift)
        {
          if (TRACKLIST_SELECTIONS->num_tracks > 0)
            {
              Track * highest =
                tracklist_selections_get_highest_track (
                  TRACKLIST_SELECTIONS);
              Track * lowest =
                tracklist_selections_get_lowest_track (
                  TRACKLIST_SELECTIONS);
              if (track->pos > highest->pos)
                {
                  /* select all tracks in between */
                  tracklist_selections_add_tracks_in_range (
                    TRACKLIST_SELECTIONS,
                    highest->pos, track->pos,
                    F_PUBLISH_EVENTS);
                }
              else if (track->pos < lowest->pos)
                {
                  /* select all tracks in between */
                  tracklist_selections_add_tracks_in_range (
                    TRACKLIST_SELECTIONS,
                    track->pos, lowest->pos,
                    F_PUBLISH_EVENTS);
                }
            }
        }
      else if (ctrl)
        {
          track_select (
            track, F_SELECT, F_NOT_EXCLUSIVE,
            F_PUBLISH_EVENTS);
        }
      else
        {
          track_select (
            track, F_SELECT, F_EXCLUSIVE,
            F_PUBLISH_EVENTS);
        }
    }
}

/**
 * Selects all Track's.
 *
 * @param visible_only Only select visible tracks.
 */
void
tracklist_selections_select_all (
  TracklistSelections * ts,
  int                   visible_only)
{
  Track * track;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (track->visible || !visible_only)
        {
          tracklist_selections_add_track (
            ts, track, 0);
        }
    }

  EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
               NULL);
}

void
tracklist_selections_remove_track (
  TracklistSelections * ts,
  Track *               track,
  int                   fire_events)
{
  if (!array_contains (
        ts->tracks, ts->num_tracks, track))
    {
      if (fire_events)
        {
          EVENTS_PUSH (ET_TRACK_CHANGED, track);
          EVENTS_PUSH (
            ET_TRACKLIST_SELECTIONS_CHANGED, NULL);
        }
      return;
    }

    /* if record mode was set automatically
     * when the track was selected, turn record
     * off */
  if (track->channel &&
      track->channel->record_set_automatically)
    {
      track_set_recording (track, 0, fire_events);
      track->channel->record_set_automatically = 0;
    }

  array_delete (
    ts->tracks,
    ts->num_tracks,
    track);

  EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
               NULL);
}

int
tracklist_selections_contains_track (
  TracklistSelections * self,
  Track *               track)
{
  return
    array_contains (
      self->tracks, self->num_tracks, track);
}

/**
 * For debugging.
 */
void
tracklist_selections_print (
  TracklistSelections * self)
{
  g_message ("------ tracklist selections ------");

  Track * track;
  for (int i = 0; i < self->num_tracks; i++)
    {
      track = self->tracks[i];
      g_message ("[idx %d] %s (pos %d)",
                 i, track->name,
                 track->pos);
    }
  g_message ("-------- end --------");
}

/**
 * Selects a single track after clearing the
 * selections.
 */
void
tracklist_selections_select_single (
  TracklistSelections * ts,
  Track *               track)
{
  tracklist_selections_clear (ts);

  tracklist_selections_add_track (
    ts, track, 0);

  EVENTS_PUSH (ET_TRACKLIST_SELECTIONS_CHANGED,
               NULL);
}

/**
 * Selects the last visible track after clearing the
 * selections.
 */
void
tracklist_selections_select_last_visible (
  TracklistSelections * ts)
{
  Track * track =
    tracklist_get_last_track (
      TRACKLIST, 0, 1);
  g_warn_if_fail (track);
  tracklist_selections_select_single (
    ts, track);
}

static int
sort_tracks_func (const void *a, const void *b)
{
  Track * aa = * (Track * const *) a;
  Track * bb = * (Track * const *) b;
  return aa->pos > bb->pos;
}

/**
 * Sorts the tracks by position.
 */
void
tracklist_selections_sort (
  TracklistSelections * self)
{
  qsort (self->tracks,
         (size_t) self->num_tracks,
         sizeof (Track *),
         sort_tracks_func);
}

/**
 * Toggle visibility of the selected tracks.
 */
void
tracklist_selections_toggle_visibility (
  TracklistSelections * ts)
{
  Track * track;
  for (int i = 0; i < ts->num_tracks; i++)
    {
      track = ts->tracks[i];
      track->visible = !track->visible;
    }

  EVENTS_PUSH (ET_TRACK_VISIBILITY_CHANGED, NULL);
}

/**
 * Toggle pin/unpin of the selected tracks.
 */
void
tracklist_selections_toggle_pinned (
  TracklistSelections * ts)
{
  Track * track;
  for (int i = 0; i < ts->num_tracks; i++)
    {
      track = ts->tracks[i];
      tracklist_set_track_pinned (
        TRACKLIST, track, !track->pinned,
        F_PUBLISH_EVENTS, F_RECALC_GRAPH);
    }
}

/**
 * Clone the struct for copying, undoing, etc.
 */
TracklistSelections *
tracklist_selections_clone (
  TracklistSelections * src)
{
  TracklistSelections * new_ts =
    object_new (TracklistSelections);

  for (int i = 0; i < src->num_tracks; i++)
    {
      Track * r = src->tracks[i];
      Track * new_r =
        track_clone (r, src->is_project);
      array_append (
        new_ts->tracks, new_ts->num_tracks,
        new_r);
    }

  return new_ts;
}

void
tracklist_selections_paste_to_pos (
  TracklistSelections * ts,
  int           pos)
{
  /* TODO */
  g_warn_if_reached ();
  return;
}

void
tracklist_selections_mark_for_bounce (
  TracklistSelections * ts)
{
  engine_reset_bounce_mode (AUDIO_ENGINE);

  for (int i = 0; i < ts->num_tracks; i++)
    {
      Track * track = ts->tracks[i];
      track->bounce = 1;

      for (int j = 0; j < track->num_lanes; j++)
        {
          TrackLane * lane = track->lanes[j];

          for (int k = 0; k < lane->num_regions; k++)
            {
              ZRegion * r = lane->regions[k];
              r->bounce = 1;
            }
        }
    }
}


void
tracklist_selections_free (
  TracklistSelections * self)
{
  if (!self->is_project)
    {
      for (int i = 0; i < self->num_tracks; i++)
        {
          object_free_w_func_and_null (
            track_free, self->tracks[i]);
        }
    }

  object_zero_and_free (self);
}

SERIALIZE_SRC (
  TracklistSelections, tracklist_selections)
DESERIALIZE_SRC (
  TracklistSelections, tracklist_selections)
PRINT_YAML_SRC (
  TracklistSelections, tracklist_selections)
