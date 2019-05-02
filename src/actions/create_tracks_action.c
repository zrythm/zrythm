/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "actions/create_tracks_action.h"
#include "audio/audio_region.h"
#include "audio/channel.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/tracklist.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/ui.h"

#include <glib/gi18n.h>

UndoableAction *
create_tracks_action_new (
  TrackType          type,
  PluginDescriptor * pl_descr,
  FileDescriptor *   file_descr,
  int                pos,
  int                num_tracks)
{
	CreateTracksAction * self =
    calloc (1, sizeof (
    	CreateTracksAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_CREATE_TRACKS;
  if (pl_descr)
    plugin_clone_descr (pl_descr, &self->pl_descr);
  else if (file_descr)
    file_manager_clone_descr (
      file_descr, &self->file_descr);
  else
    self->is_empty = 1;
  self->pos = pos;
  self->type = type;
  self->num_tracks = num_tracks;

  return ua;
}

static int
create_instrument (
  CreateTracksAction * self,
  int                  idx)
{
  Channel * ch;
  if (self->is_empty)
    {
      ch =
        channel_new (
          CT_MIDI, _("Instrument Track"),
          F_ADD_TO_PROJ);
      mixer_add_channel (MIXER, ch, 1);
      tracklist_insert_track (
        TRACKLIST,
        ch->track,
        self->pos,
        F_NO_PUBLISH_EVENTS);
    }
  else
    {
      Plugin * pl=
        plugin_new_from_descr (
          &self->pl_descr,
          F_NO_ADD_TO_PROJ);

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

      ChannelType ct = CT_MIDI;
      ch =
        channel_new (
          ct, self->pl_descr.name, F_ADD_TO_PROJ);
      mixer_add_channel (MIXER, ch, 1);
      track_new (ch, "label");
      tracklist_insert_track (
        TRACKLIST,
        ch->track,
        self->pos,
        F_NO_PUBLISH_EVENTS);
      channel_add_plugin (
        ch, 0, pl, 1, 1, 1);

      if (g_settings_get_int (
            S_PREFERENCES,
            "open-plugin-uis-on-instantiate"))
        {
          pl->visible = 1;
          EVENTS_PUSH (
            ET_PLUGIN_VISIBILITY_CHANGED, pl);
        }
    }

  self->track_ids[idx] = ch->track->id;

  EVENTS_PUSH (ET_TRACKS_CREATED, ch->track);

  return 0;
}

static int
create_audio (
  CreateTracksAction * self,
  int                  idx)
{
  Channel * ch;
  if (self->is_empty)
    {
      ch =
        channel_new (
          CT_AUDIO, _("Audio Track"),
          F_ADD_TO_PROJ);
      mixer_add_channel (MIXER, ch, 1);
      tracklist_insert_track (
        TRACKLIST,
        ch->track,
        self->pos,
        F_NO_PUBLISH_EVENTS);
    }
  else
    {
      /* create a channel/track */
      ch =
        channel_new (CT_AUDIO, "Audio Track",
                     F_ADD_TO_PROJ);
      mixer_add_channel (MIXER, ch, 1);
      tracklist_insert_track (
        TRACKLIST,
        ch->track,
        self->pos,
        F_NO_PUBLISH_EVENTS);

      /* create an audio region & add to track */
      Position start_pos;
      position_set_to_pos (&start_pos,
                           &PLAYHEAD);
      AudioRegion * ar =
        audio_region_new (
          ch->track,
          self->file_descr.absolute_path,
          &start_pos,
          F_ADD_TO_PROJ);
      track_add_region (
        ch->track, ar);
    }

  self->track_ids[idx] = ch->track->id;

  EVENTS_PUSH (ET_TRACKS_CREATED, ch->track);

  return 0;
}

static int
create_bus (
  CreateTracksAction * self,
  int                  idx)
{
  Channel * ch;
  if (self->is_empty)
    {
      ch =
        channel_new (CT_BUS, _("Bus Track"),
                        F_ADD_TO_PROJ);
      mixer_add_channel (MIXER, ch, 1);
      tracklist_insert_track (
        TRACKLIST,
        ch->track,
        self->pos,
        F_NO_PUBLISH_EVENTS);
    }
  else
    {
      Plugin * pl=
        plugin_new_from_descr (
          &self->pl_descr,
          F_NO_ADD_TO_PROJ);

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

      ChannelType ct = CT_BUS;
      ch =
        channel_new (
          ct, self->pl_descr.name, F_ADD_TO_PROJ);
      mixer_add_channel (MIXER, ch, 1);
      tracklist_insert_track (
        TRACKLIST,
        ch->track,
        self->pos,
        F_NO_PUBLISH_EVENTS);
      channel_add_plugin (
        ch, 0, pl, 1, 1, 1);

      if (g_settings_get_int (
            S_PREFERENCES,
            "open-plugin-uis-on-instantiate"))
        {
          pl->visible = 1;
          EVENTS_PUSH (
            ET_PLUGIN_VISIBILITY_CHANGED, pl);
        }
    }

  self->track_ids[idx] = ch->track->id;

  EVENTS_PUSH (ET_TRACKS_CREATED, ch->track);

  return 0;
}

int
create_tracks_action_do (
	CreateTracksAction * self)
{
  int ret;
  for (int i = 0; i < self->num_tracks; i++)
    {
      switch (self->type)
        {
        case TRACK_TYPE_AUDIO:
          ret = create_audio (self, i);
          break;
        case TRACK_TYPE_INSTRUMENT:
          ret = create_instrument (self, i);
          break;
        case TRACK_TYPE_BUS:
          ret = create_bus (self, i);
          break;
        default:
          g_warn_if_reached ();
          return -1;
        }
      if (ret)
        {
          g_warn_if_reached ();
          return -1;
        }
    }

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
  for (int i = 0; i < self->num_tracks; i++)
    {
      track =
        project_get_track (self->track_ids[i]);
      if (!track)
        {
          g_warn_if_reached ();
          return -1;
        }

      tracklist_remove_track (
        TRACKLIST,
        track,
        F_FREE,
        F_NO_PUBLISH_EVENTS);
    }

  EVENTS_PUSH (ET_TRACKS_REMOVED, NULL);

  return 0;
}

char *
create_tracks_action_stringize (
	CreateTracksAction * self)
{
  if (self->num_tracks == 1)
    return g_strdup_printf (
      _("Create %s Track"),
      track_stringize_type (
        self->type));
  else
    return g_strdup_printf (
      _("Create %d %s Tracks"),
      self->num_tracks,
      track_stringize_type (
        self->type));
}

void
create_tracks_action_free (
	CreateTracksAction * self)
{
  free (self);
}
