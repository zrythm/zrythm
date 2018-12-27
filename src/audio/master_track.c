/*
 * audio/master_track.c - master track
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
#include "audio/master_track.h"

#include <gtk/gtk.h>

MasterTrack *
master_track_new (Channel * channel)
{
  MasterTrack * self =
    calloc (1, sizeof (MasterTrack));

  Track * track = (Track *) self;
  track->type = TRACK_TYPE_MASTER;
  track_init (track);

  ChannelTrack * bt = (ChannelTrack *) self;
  bt->channel = channel;

  return self;
}

void
master_track_setup (MasterTrack * self)
{
  BusTrack * track = (BusTrack *) self;

  bus_track_setup (track);
}

/**
 * Frees the track.
 *
 * TODO
 */
void
master_track_free (MasterTrack * track)
{

}

