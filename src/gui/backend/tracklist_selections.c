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
#include "audio/master_track.h"
#include "audio/position.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

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

  self->schema_version =
    TRACKLIST_SELECTIONS_SCHEMA_VERSION;

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

  g_debug (
    "%s currently recording: %d, have channel: %d",
    track->name, track->recording,
    track->channel != NULL);

  /* if recording is not already
   * on, turn these on */
  if (!track->recording && track->channel)
    {
      track_set_recording (
        track, true, fire_events);
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

/**
 * Clears the selections.
 */
void
tracklist_selections_clear (
  TracklistSelections * self)
{
  g_message ("clearing tracklist selections...");

  for (int i = self->num_tracks - 1; i >= 0; i--)
    {
      Track * track = self->tracks[i];
      tracklist_selections_remove_track (
        self, track, 0);

      if (track->is_project)
        {
          /* process now because the track might
           * get deleted after this */
          if (GTK_IS_WIDGET (track->widget))
            {
              gtk_widget_set_visible (
                GTK_WIDGET (track->widget),
                track->visible);
              track_widget_force_redraw (
                track->widget);
            }
        }
    }

  if (self->is_project && ZRYTHM_HAVE_UI &&
      PROJECT->loaded)
    {
      EVENTS_PUSH (
        ET_TRACKLIST_SELECTIONS_CHANGED, NULL);
    }

  g_message ("done");
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
              g_return_if_fail (
                IS_TRACK_AND_NONNULL (highest) &&
                IS_TRACK_AND_NONNULL (lowest));
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

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_TRACKLIST_SELECTIONS_CHANGED, NULL);
    }
}

bool
tracklist_selections_contains_undeletable_track (
  TracklistSelections * self)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];

      if (!track_type_is_deletable (track->type))
        {
          return true;
        }
    }

  return false;
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
  Track *               track,
  bool                  fire_events)
{
  tracklist_selections_clear (ts);

  tracklist_selections_add_track (
    ts, track, 0);

  if (fire_events)
    {
      EVENTS_PUSH (
        ET_TRACKLIST_SELECTIONS_CHANGED, NULL);
    }
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
      TRACKLIST,
      TRACKLIST_PIN_OPTION_BOTH, true);
  g_warn_if_fail (track);
  tracklist_selections_select_single (
    ts, track, F_PUBLISH_EVENTS);
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

#if 0
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
#endif

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

/**
 * Marks the tracks to be bounced.
 *
 * @param with_parents Also mark all the track's
 *   parents recursively.
 * @param mark_master Also mark the master track.
 *   Set to true when exporting the mixdown, false
 *   otherwise.
 */
void
tracklist_selections_mark_for_bounce (
  TracklistSelections * ts,
  bool                  with_parents,
  bool                  mark_master)
{
  engine_reset_bounce_mode (AUDIO_ENGINE);

  for (int i = 0; i < ts->num_tracks; i++)
    {
      Track * track = ts->tracks[i];
      track_mark_for_bounce (
        track, F_BOUNCE, F_MARK_REGIONS,
        F_MARK_CHILDREN, with_parents);
    }

  if (mark_master)
    {
      track_mark_for_bounce (
        P_MASTER_TRACK, F_BOUNCE, F_NO_MARK_REGIONS,
        F_NO_MARK_CHILDREN, F_NO_MARK_PARENTS);
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
          g_return_if_fail (
            IS_TRACK_AND_NONNULL (self->tracks[i]));

          /* skip project tracks */
          if (self->tracks[i]->is_project)
            continue;

          object_free_w_func_and_null (
            track_free, self->tracks[i]);
        }
    }

  object_zero_and_free (self);
}
