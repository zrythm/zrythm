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

#ifndef __AUDIO_GROUP_TRACK_H__
#define __AUDIO_GROUP_TRACK_H__

#include "audio/channel_track.h"
#include "audio/track.h"

typedef struct Position Position;
typedef struct _TrackWidget TrackWidget;
typedef struct Channel Channel;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable Automatable;

void
group_track_init (Track * track);

void
group_track_setup (Track * self);

/**
 * Frees the track.
 *
 * TODO
 */
void
group_track_free (Track * track);

#endif // __AUDIO_BUS_TRACK_H__
