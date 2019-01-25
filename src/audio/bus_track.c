/*
 * audio/bus_track.c - bus track
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

#include "audio/automation_tracklist.h"
#include "audio/bus_track.h"

BusTrack *
bus_track_new (Channel * channel)
{
  BusTrack * self =
    calloc (1, sizeof (BusTrack));

  Track * track = (Track *) self;
  track->type = TRACK_TYPE_BUS;
  gdk_rgba_parse (&track->color, "#F9CA1B");
  track_init (track);

  ChannelTrack * ct = (ChannelTrack *) self;
  ct->channel = channel;

  return self;
}

void
bus_track_setup (BusTrack * self)
{
  channel_track_setup ((ChannelTrack *) self);
}

/**
 * Frees the track.
 *
 * TODO
 */
void
bus_track_free (BusTrack * track)
{

}
