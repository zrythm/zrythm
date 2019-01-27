/*
 * audio/track.c - the back end for a timeline track
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "audio/audio_track.h"
#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/instrument_track.h"
#include "audio/master_track.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/timeline_arranger.h"

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
      arranger_widget_refresh (
        Z_ARRANGER_WIDGET (MW_TIMELINE));
      return;
    }

  g_warning ("attempted to add region to a track type"
             " that does not accept regions");
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
