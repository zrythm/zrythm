/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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

/**
 * \file
 *
 * Instrument track backend.
 */

#ifndef __AUDIO_INSTRUMENT_TRACK_H__
#define __AUDIO_INSTRUMENT_TRACK_H__

#include "audio/channel_track.h"
#include "audio/track.h"

typedef struct Position Position;
typedef struct _TrackWidget TrackWidget;
typedef struct Channel Channel;
typedef struct MidiEvents MidiEvents;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable Automatable;
typedef struct Region MidiRegion;

typedef struct Track InstrumentTrack;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Initializes an instrument track.
 */
void
instrument_track_init (Track * track);

void
instrument_track_setup (Track * self);

/**
 * Frees the track.
 *
 * TODO
 */
void
instrument_track_free (InstrumentTrack * track);

/**
 * @}
 */

#endif // __AUDIO_INSTRUMENT_TRACK_H__
