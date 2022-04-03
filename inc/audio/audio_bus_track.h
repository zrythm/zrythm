/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_AUDIO_BUS_TRACK_H__
#define __AUDIO_AUDIO_BUS_TRACK_H__

#include "audio/channel_track.h"
#include "audio/track.h"

typedef struct Position        Position;
typedef struct _TrackWidget    TrackWidget;
typedef struct Channel         Channel;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable     Automatable;

typedef struct Track AudioBusTrack;

void
audio_bus_track_init (Track * track);

void
audio_bus_track_setup (AudioBusTrack * self);

#endif // __AUDIO_AUDIO_BUS_TRACK_H__
