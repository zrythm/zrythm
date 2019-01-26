/*
 * audio/audio_track.c - audio track
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
#include "audio/automation_tracklist.h"

AudioTrack *
audio_track_new (Channel * channel)
{
  AudioTrack * self =
    calloc (1, sizeof (AudioTrack));

  Track * track = (Track *) self;
  track->type = TRACK_TYPE_AUDIO;
  track_init (track);
  project_add_track (track);

  ChannelTrack * bt = (ChannelTrack *) self;
  bt->channel = channel;

  return self;

}

void
audio_track_setup (AudioTrack * self)
{
  ChannelTrack * bt = (ChannelTrack *) self;

  channel_track_setup (bt);
}

void
audio_track_add_region (AudioTrack *  track,
                        AudioRegion * region)
{

}

void
audio_track_remove_region (AudioTrack *  track,
                           AudioRegion * region)
{

}

/**
 * Frees the track.
 *
 * TODO
 */
void
audio_track_free (AudioTrack * track)
{

}
