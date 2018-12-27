/*
 * audio/channel_track.c - channel track
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
#include "audio/channel_track.h"

void
channel_track_setup (ChannelTrack * self)
{
  Track * track = (Track *) self;

  self->automation_tracklist =
    automation_tracklist_new (track);

  automation_tracklist_setup (self->automation_tracklist);
}

/**
 * Frees the track.
 *
 * TODO
 */
void
channel_track_free (ChannelTrack * track)
{

}
