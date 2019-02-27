/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include <stdlib.h>

#include "actions/edit_track_action.h"
#include "actions/undo_manager.h"
#include "audio/audio_track.h"
#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/instrument_track.h"
#include "audio/master_track.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/backend/events.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/track.h"
#include "project.h"

void
track_init (Track * track)
{
  track->visible = 1;
  track->handle_pos = 1;
}

Track *
track_new (Channel * channel)
{
  switch (channel->type)
    {
    case CT_MIDI:
      return (Track *) instrument_track_new (channel);
      break;
    case CT_AUDIO:
      return (Track *) audio_track_new (channel);
      break;
    case CT_BUS:
      return (Track *) bus_track_new (channel);
      break;
    case CT_MASTER:
      return (Track *) master_track_new (channel);
      break;
    }
  g_assert_not_reached ();
  return NULL;
}

/**
 * Sets recording and connects/disconnects the JACK ports.
 */
void
track_set_recording (Track *   track,
                     int       recording)
{
  Channel * channel =
    track_get_channel (track);

  if (!channel)
    {
      ui_show_notification_idle (
        "Recording not implemented yet for this "
        "track.");
      return;
    }
  if (channel->type == CT_AUDIO)
    {
      /* TODO connect L and R audio ports for recording */
      if (recording)
        {
          port_connect (
            AUDIO_ENGINE->stereo_in->l,
            channel->stereo_in->l);
          port_connect (
            AUDIO_ENGINE->stereo_in->r,
            channel->stereo_in->r);
        }
      else
        {
          port_disconnect (
            AUDIO_ENGINE->stereo_in->l,
            channel->stereo_in->l);
          port_disconnect (
            AUDIO_ENGINE->stereo_in->r,
            channel->stereo_in->r);
        }
    }
  else if (channel->type == CT_MIDI)
    {
      /* find first plugin */
      Plugin * plugin = channel_get_first_plugin (channel);

      if (plugin)
        {
          /* Connect/Disconnect MIDI port to the plugin */
          for (int i = 0; i < plugin->num_in_ports; i++)
            {
              Port * port = plugin->in_ports[i];
              if (port->type == TYPE_EVENT &&
                  port->flow == FLOW_INPUT)
                {
                  g_message ("%d MIDI In port: %s",
                             i, port->label);
                  if (recording)
                    port_connect (
                      AUDIO_ENGINE->midi_in, port);
                  else
                    port_disconnect (
                      AUDIO_ENGINE->midi_in, port);
                }
            }
        }
    }

  track->recording = recording;

  EVENTS_PUSH (ET_TRACK_STATE_CHANGED,
               track);
}

/**
 * Sets track muted and optionally adds the action
 * to the undo stack.
 */
void
track_set_muted (Track * track,
                 int     mute,
                 int     trigger_undo)
{
  UndoableAction * action =
    edit_track_action_new_mute (track,
                                mute);
  g_message ("setting mute to %d",
             mute);
  if (trigger_undo)
    {
      undo_manager_perform (UNDO_MANAGER,
                            action);
    }
  else
    {
      edit_track_action_do (
        (EditTrackAction *) action);
    }
}

/**
 * Sets track soloed, updates UI and optionally
 * adds the action to the undo stack.
 */
void
track_set_soloed (Track * track,
                  int     solo,
                  int     trigger_undo)
{
  UndoableAction * action =
    edit_track_action_new_solo (track,
                                solo);
  if (trigger_undo)
    {
      undo_manager_perform (UNDO_MANAGER,
                            action);
    }
  else
    {
      edit_track_action_do (
        (EditTrackAction *) action);
    }
}

/**
 * Wrapper.
 */
void
track_setup (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_INSTRUMENT:
      instrument_track_setup (
        (InstrumentTrack *) track);
      break;
    case TRACK_TYPE_MASTER:
      master_track_setup (
        (MasterTrack *) track);
      break;
    case TRACK_TYPE_AUDIO:
      audio_track_setup (
        (AudioTrack *) track);
      break;
    case TRACK_TYPE_CHORD:
      break;
    case TRACK_TYPE_BUS:
      bus_track_setup (
        (BusTrack *) track);
      break;
    }
}

/**
 * Wrapper.
 */
void
track_add_region (Track * track,
                  Region * region)
{
  region_set_track (region, track);
  if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      instrument_track_add_region (
        (InstrumentTrack *) track,
        (MidiRegion *) region);
      EVENTS_PUSH (ET_REGION_CREATED,
                   region);
      return;
    }

  g_warning (
    "attempted to add region to a track type"
    " that does not accept regions");
}

/**
 * Wrapper.
 */
void
track_remove_region (Track * track,
                     Region * region,
                     int       delete)
{
  if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      instrument_track_remove_region (
        (InstrumentTrack *) track,
        (MidiRegion *) region);
    }
  else if (track->type == TRACK_TYPE_AUDIO)
    {
      audio_track_remove_region (
        (AudioTrack *) track,
        (AudioRegion *) region);
    }
  if (delete)
    region_free (region);
  EVENTS_PUSH (ET_REGION_REMOVED, track);
}

/**
 * Wrapper for each track type.
 */
void
track_free (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_INSTRUMENT:
      instrument_track_free (
        (InstrumentTrack *) track);
      break;
    case TRACK_TYPE_MASTER:
      master_track_free (
        (MasterTrack *) track);
      break;
    case TRACK_TYPE_AUDIO:
      audio_track_free (
        (AudioTrack *) track);
      break;
    case TRACK_TYPE_CHORD:
      chord_track_free (
        (ChordTrack *) track);
      break;
    case TRACK_TYPE_BUS:
      bus_track_free (
        (BusTrack *) track);
      break;
    }
}

/**
 * Returns the automation tracklist if the track type has one,
 * or NULL if it doesn't (like chord tracks).
 */
AutomationTracklist *
track_get_automation_tracklist (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_CHORD:
      break;
    case TRACK_TYPE_BUS:
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_MASTER:
        {
          ChannelTrack * bt = (ChannelTrack *) track;
          return &bt->automation_tracklist;
        }
    }

  return NULL;
}

/**
 * Wrapper for track types that have fader automatables.
 *
 * Otherwise returns NULL.
 */
Automatable *
track_get_fader_automatable (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_CHORD:
      break;
    case TRACK_TYPE_BUS:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_INSTRUMENT:
        {
          ChannelTrack * bt = (ChannelTrack *) track;
          return channel_get_fader_automatable (bt->channel);
        }
    }

  return NULL;
}

/**
 * Returns the channel of the track, if the track type has
 * a channel,
 * or NULL if it doesn't.
 */
Channel *
track_get_channel (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_CHORD:
      break;
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_INSTRUMENT:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_BUS:
        {
          ChannelTrack * bt = (ChannelTrack *) track;
          return bt->channel;
        }
    }

  return NULL;
}

/**
 * Returns the region at the given position, or NULL.
 */
Region *
track_get_region_at_pos (
  Track *    track,
  Position * pos)
{
  if (track->type == TRACK_TYPE_INSTRUMENT)
    {
      InstrumentTrack * it = (InstrumentTrack *) track;
      for (int i = 0; i < it->num_regions; i++)
        {
          Region * r = (Region *) it->regions[i];
          if (position_compare (pos,
                                &r->start_pos) >= 0 &&
              position_compare (pos,
                                &r->end_pos) <= 0)
            return r;
        }
    }
  else if (track->type == TRACK_TYPE_AUDIO)
    {
      AudioTrack * at = (AudioTrack *) track;
      for (int i = 0; i < at->num_regions; i++)
        {
          Region * r = (Region *) at->regions[i];
          if (position_compare (pos,
                                &r->start_pos) >= 0 &&
              position_compare (pos,
                                &r->end_pos) <= 0)
            return r;
        }
    }
  return NULL;
}

/**
 * Wrapper to get the track name.
 */
char *
track_get_name (Track * track)
{
  switch (track->type)
    {
    case TRACK_TYPE_CHORD:
        {
          ChordTrack * ct = (ChordTrack *) track;
          return ct->name;
        }
      break;
    case TRACK_TYPE_BUS:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_INSTRUMENT:
        {
          ChannelTrack * ct = (ChannelTrack *) track;
          return ct->channel->name;
        }
    default:
      return NULL;
    }
}
