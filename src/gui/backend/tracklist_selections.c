/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/main_window.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

void
tracklist_selections_init_loaded (
  TracklistSelections * self)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];
      int track_pos = track->pos;
      track_init_loaded (track, NULL, self);
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
    track->name,
    track_type_can_record (track->type) &&
      track_get_recording (track),
    track->channel != NULL);

  /* if recording is not already on, turn these on */
  if (track_type_can_record (track->type) &&
      !track_get_recording (track) && track->channel)
    {
      track_set_recording (
        track, true, fire_events);
      track->record_set_automatically = true;
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

      if (track_is_in_active_project (track))
        {
          /* process now because the track might
           * get deleted after this */
          if (GTK_IS_WIDGET (track->widget))
            {
              gtk_widget_set_visible (
                GTK_WIDGET (track->widget),
                track->visible);
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
 * Make sure all children of foldable tracks in
 * the selection are also selected.
 */
void
tracklist_selections_select_foldable_children (
  TracklistSelections * self)
{
  int num_tracklist_sel = self->num_tracks;
  for (int i = 0; i < num_tracklist_sel; i++)
    {
      Track * cur_track = self->tracks[i];
      int cur_idx = cur_track->pos;
      if (track_type_is_foldable (cur_track->type))
        {
          for (int j = 1; j < cur_track->size; j++)
            {
              Track * child_track =
                TRACKLIST->tracks[j + cur_idx];
              if (!track_is_selected (child_track))
                {
                  track_select (
                    child_track, F_SELECT,
                    F_NOT_EXCLUSIVE,
                    F_NO_PUBLISH_EVENTS);
                }
            }
        }
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
          if (TRACKLIST_SELECTIONS->num_tracks > 1)
            {
              track_select (
                track, F_NO_SELECT, F_NOT_EXCLUSIVE,
                F_PUBLISH_EVENTS);
            }
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
     * off - unless currently recording */
  if (track->channel
      && track->record_set_automatically
      &&
      !(TRANSPORT_IS_RECORDING &&
        TRANSPORT_IS_ROLLING))
    {
      track_set_recording (track, 0, fire_events);
      track->record_set_automatically = false;
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
tracklist_selections_contains_uninstantiated_plugin (
  const TracklistSelections * self)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];

      if (track_contains_uninstantiated_plugin (
            track))
        {
          return true;
        }
    }

  return false;
}

bool
tracklist_selections_contains_undeletable_track (
  const TracklistSelections * self)
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

bool
tracklist_selections_contains_uncopyable_track (
  const TracklistSelections * self)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];

      if (!track_type_is_copyable (track->type))
        {
          return true;
        }
    }

  return false;
}

/**
 * Returns whether the selections contain a soloed
 * track if @ref soloed is true or an unsoloed track
 * if @ref soloed is false.
 *
 * @param soloed Whether to check for soloed or
 *   unsoloed tracks.
 */
bool
tracklist_selections_contains_soloed_track (
  TracklistSelections * self,
  bool                  soloed)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];
      if (!track_type_has_channel (track->type))
        continue;

      if (track_get_soloed (track) == soloed)
        return true;
    }

  return false;
}

/**
 * Returns whether the selections contain a muted
 * track if @ref muted is true or an unmuted track
 * if @ref muted is false.
 *
 * @param muted Whether to check for muted or
 *   unmuted tracks.
 */
bool
tracklist_selections_contains_muted_track (
  TracklistSelections * self,
  bool                  muted)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];
      if (!track_type_has_channel (track->type))
        continue;

      if (track_get_muted (track) == muted)
        return true;
    }

  return false;
}

/**
 * Returns whether the selections contain a listened
 * track if @ref listened is true or an unlistened
 * track if @ref listened is false.
 *
 * @param listened Whether to check for listened or
 *   unlistened tracks.
 */
bool
tracklist_selections_contains_listened_track (
  TracklistSelections * self,
  bool                  listened)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];
      if (!track_type_has_channel (track->type))
        continue;

      if (track_get_listened (track) == listened)
        return true;
    }

  return false;
}

bool
tracklist_selections_contains_enabled_track (
  TracklistSelections * self,
  bool                  enabled)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];
      if (track_is_enabled (track) == enabled)
        return true;
    }

  return false;
}

bool
tracklist_selections_contains_track (
  TracklistSelections * self,
  Track *               track)
{
  return
    array_contains (
      self->tracks, self->num_tracks, track);
}

bool
tracklist_selections_contains_track_index (
  TracklistSelections * self,
  int                   track_idx)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];
      if (track->pos == track_idx)
        return true;
    }

  return false;
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
sort_tracks_func_desc (const void *a, const void *b)
{
  Track * aa = * (Track * const *) a;
  Track * bb = * (Track * const *) b;
  return aa->pos < bb->pos;
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
 *
 * @param asc Ascending or not.
 */
void
tracklist_selections_sort (
  TracklistSelections * self,
  bool                  asc)
{
  qsort (
    self->tracks,
    (size_t) self->num_tracks,
    sizeof (Track *),
    asc ? sort_tracks_func : sort_tracks_func_desc);
}

void
tracklist_selections_get_plugins (
  TracklistSelections * self,
  GPtrArray *           arr)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      Track * track = self->tracks[i];
      track_get_plugins (track, arr);
    }
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
NONNULL_ARGS (1)
TracklistSelections *
tracklist_selections_clone (
  TracklistSelections * src,
  GError **             error)
{
  g_return_val_if_fail (!error || !*error, NULL);

  TracklistSelections * self =
    object_new (TracklistSelections);

  self->is_project = src->is_project;

  for (int i = 0; i < src->num_tracks; i++)
    {
      Track * r = src->tracks[i];

      GError * err = NULL;
      Track * new_r = track_clone (r, &err);
      if (!new_r)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err,
            _("Failed to clone track '%s'"),
            r->name);
          object_free_w_func_and_null (
            tracklist_selections_free, self);
          return NULL;
        }
      array_append (
        self->tracks, self->num_tracks,
        new_r);
    }

  self->is_project = false;

  return self;
}

/**
 * To be called after receiving tracklist selections
 * from the clipboard.
 */
void
tracklist_selections_post_deserialize (
  TracklistSelections * self)
{
  for (int i = 0; i < self->num_tracks; i++)
    {
      track_set_magic (self->tracks[i]);
    }
}

void
tracklist_selections_paste_to_pos (
  TracklistSelections * ts,
  int                   pos)
{
  GError * err = NULL;
  bool ret =
    tracklist_selections_action_perform_copy (
      ts, PORT_CONNECTIONS_MGR, pos, &err);
  if (!ret)
    {
      HANDLE_ERROR (
        err, "%s",
        _("Failed to paste tracks"));
    }
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
  if (!self->is_project || self->free_tracks)
    {
      for (int i = 0; i < self->num_tracks; i++)
        {
          g_return_if_fail (
            IS_TRACK_AND_NONNULL (self->tracks[i]));

#if 0
          /* skip project tracks */
          if (self->tracks[i]->is_project)
            continue;
#endif

          track_disconnect (
            self->tracks[i], F_NO_REMOVE_PL,
            F_NO_RECALC_GRAPH);

          object_free_w_func_and_null (
            track_free, self->tracks[i]);
        }
    }

  object_zero_and_free (self);
}
