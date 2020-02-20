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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/create_tracks_action.h"
#include "audio/audio_region.h"
#include "audio/channel.h"
#include "audio/midi_file.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/supported_file.h"
#include "audio/tracklist.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/ui.h"

#include <ext/midilib/src/midifile.h>

#include <glib/gi18n.h>

/**
 * @param add_to_project Used when the track to
 *   create is meant to be used in the project (ie
 *   not one of the tracks in CreateTracksAction.
 */
static int
create (
  CreateTracksAction * self,
  int                  idx,
  int                  add_to_project)
{
  Track * track;
  int pos = self->pos + idx;

  if (self->is_empty)
    {
      char * tmp =
        track_stringize_type (self->type);
      char * label;
      label =
        g_strdup_printf (_("%s Track"), tmp);
      g_free (tmp);

      track =
        track_new (
          self->type, pos, label, F_WITH_LANE);
      if (add_to_project)
        tracklist_insert_track (
          TRACKLIST, track, pos,
          F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);
    }
  else // track is not empty
    {
      Plugin * pl = NULL;

      if (self->file_descr &&
          self->type == TRACK_TYPE_AUDIO)
        {
          char * basename =
            io_path_get_basename (
              self->file_descr->abs_path);
          track =
            track_new (
              self->type, pos, basename,
              F_WITH_LANE);
          g_free (basename);
        }
      else if (self->file_descr &&
               self->type == TRACK_TYPE_MIDI)
        {
          char * basename =
            io_path_get_basename (
              self->file_descr->abs_path);
          track =
            track_new (
              self->type, pos, basename,
              F_WITH_LANE);
          g_free (basename);
        }
      /* at this point we can assume it has a
       * plugin */
      else
        {
          track =
            track_new (
              self->type, pos, self->pl_descr.name,
              F_WITH_LANE);

          pl=
            plugin_new_from_descr (
              &self->pl_descr, track->pos, 0);

          if (plugin_instantiate (pl) < 0)
            {
              char * message =
                g_strdup_printf (
                  _("Error instantiating plugin %s. "
                    "Please see log for details."),
                  pl->descr->name);

              if (MAIN_WINDOW)
                ui_show_error_message (
                  GTK_WINDOW (MAIN_WINDOW),
                  message);
              g_free (message);
              plugin_free (pl);
              return -1;
            }
        }

      if (add_to_project)
        tracklist_insert_track (
          TRACKLIST, track, track->pos,
          F_NO_PUBLISH_EVENTS, F_NO_RECALC_GRAPH);

      if (track->channel && pl)
        {
          channel_add_plugin (
            track->channel, pl->id.slot, pl,
            1, 1, F_NO_RECALC_GRAPH);
          g_warn_if_fail (
            pl->id.track_pos == track->pos);
        }

      if (self->type == TRACK_TYPE_AUDIO)
        {
          /* create an audio region & add to
           * track */
          Position start_pos;
          position_set_to_pos (
            &start_pos, PLAYHEAD);
          ZRegion * ar =
            audio_region_new (
              add_to_project ? self->pool_id : -1,
              add_to_project ?
                NULL :
                self->file_descr->abs_path,
              NULL, 0, 0,
              &start_pos, pos, 0, 0);
          if (!add_to_project)
            self->pool_id =
              ar->pool_id;
          track_add_region (
            track, ar, NULL, 0, F_GEN_NAME,
            F_PUBLISH_EVENTS);
        }
      else if (self->type == TRACK_TYPE_MIDI)
        {
          /* create a MIDI region from the MIDI
           * file & add to track */
          Position start_pos;
          position_set_to_pos (
            &start_pos, PLAYHEAD);
          ZRegion * mr =
            midi_region_new_from_midi_file (
              &start_pos,
              self->file_descr->abs_path,
              pos, 0, 0, idx);
          if (mr)
            {
              track_add_region (
                track, mr, NULL, 0,
                /* name was already generated based
                 * on the track name in the MIDI
                 * file */
                F_NO_GEN_NAME,
                F_NO_PUBLISH_EVENTS);
            }
        }

      if (pl && ZRYTHM_HAVE_UI &&
          g_settings_get_int (
            S_PREFERENCES,
            "open-plugin-uis-on-instantiate") &&
          add_to_project)
        {
          pl->visible = 1;
          EVENTS_PUSH (
            ET_PLUGIN_VISIBILITY_CHANGED, pl);
        }
    }

  return 0;
}

/**
 * Creates a new CreateTracksAction.
 *
 * @param pos Position to make the tracks at.
 */
UndoableAction *
create_tracks_action_new (
  TrackType          type,
  const PluginDescriptor * pl_descr,
  SupportedFile *    file,
  int                pos,
  int                num_tracks)
{
  CreateTracksAction * self =
    calloc (1, sizeof (
      CreateTracksAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_CREATE_TRACKS;
  if (pl_descr)
    {
      plugin_descriptor_copy (
        pl_descr, &self->pl_descr);
    }
  else if (file)
    {
      self->file_descr =
        supported_file_clone (file);
    }
  else
    {
      self->is_empty = 1;
    }
  self->pos = pos;
  self->type = type;

  /* calculate number of tracks */
  if (file && type == TRACK_TYPE_MIDI)
    {
      self->num_tracks =
        midi_file_get_num_tracks (
          self->file_descr->abs_path);
    }
  else
    {
      self->num_tracks = num_tracks;
    }
  for (int i = 0; i < num_tracks; i++)
    {
      /* create clones for reference */
      create (self, i, 0);
    }

  return ua;
}

int
create_tracks_action_do (
  CreateTracksAction * self)
{
  int ret;
  for (int i = 0; i < self->num_tracks; i++)
    {
      ret = create (self, i, 1);
      g_return_val_if_fail (!ret, -1);

      /* TODO select each plugin that was selected */
    }

  EVENTS_PUSH (ET_TRACKS_ADDED, NULL);

  mixer_recalc_graph (MIXER);

  return 0;
}

/**
 * Deletes the track.
 */
int
create_tracks_action_undo (
  CreateTracksAction * self)
{
  Track * track;
  for (int i = self->num_tracks - 1; i >= 0; i--)
    {
      track =
        TRACKLIST->tracks[self->pos + i];
      g_return_val_if_fail (track, -1);

      tracklist_remove_track (
        TRACKLIST, track,
        F_REMOVE_PL, F_FREE,
        F_NO_PUBLISH_EVENTS,
        F_NO_RECALC_GRAPH);
    }

  EVENTS_PUSH (ET_TRACKS_REMOVED, NULL);

  mixer_recalc_graph (MIXER);

  return 0;
}

char *
create_tracks_action_stringize (
  CreateTracksAction * self)
{
  char * type =
    track_stringize_type (
      self->type);
  char * ret;
  if (self->num_tracks == 1)
    ret = g_strdup_printf (
      _("Create %s Track"), type);
  else
    ret = g_strdup_printf (
      _("Create %d %s Tracks"),
      self->num_tracks, type);

  g_free (type);

  return ret;
}

void
create_tracks_action_free (
  CreateTracksAction * self)
{
  if (self->file_descr)
    supported_file_free (self->file_descr);
  free (self);
}
