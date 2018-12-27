/*
 * audio/channel_track.h - channel track
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

#ifndef __AUDIO_CHANNEL_TRACK_H__
#define __AUDIO_CHANNEL_TRACK_H__

#include "audio/track.h"

typedef struct Position Position;
typedef struct _TrackWidget TrackWidget;
typedef struct Channel Channel;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable Automatable;

/**
 * This track is for convenience. It contains common
 * variables for tracks that correspond to a channel in
 * the mixer. Should never be instantiated.
 */
typedef struct ChannelTrack
{
  Track                 parent; ///< base track

  /**
   * Owner channel.
   *
   * 1 channel has 1 track.
   */
  Channel *             channel;

  AutomationTracklist * automation_tracklist;
} ChannelTrack;

void
channel_track_setup (ChannelTrack * self);

/**
 * Frees the track.
 *
 * TODO
 */
void
channel_track_free (ChannelTrack * track);

#endif // __AUDIO_CHANNEL_TRACK_H__
