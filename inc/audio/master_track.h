/*
 * audio/master_track.h - master track
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

#ifndef __AUDIO_MASTER_TRACK_H__
#define __AUDIO_MASTER_TRACK_H__

#include "audio/bus_track.h"
#include "audio/track.h"

typedef struct Position Position;
typedef struct _TrackWidget TrackWidget;
typedef struct Channel Channel;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable Automatable;

typedef struct MasterTrack
{
  BusTrack              parent; ///< base track
} MasterTrack;

MasterTrack *
master_track_new (Channel * channel);

void
master_track_setup (MasterTrack * self);

/**
 * Frees the track.
 *
 * TODO
 */
void
master_track_free (MasterTrack * track);

#endif // __AUDIO_MASTER_TRACK_H__
