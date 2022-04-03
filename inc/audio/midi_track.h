/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __AUDIO_MIDI_TRACK_H__
#define __AUDIO_MIDI_TRACK_H__

#include "audio/channel_track.h"
#include "audio/track.h"

typedef struct Position        Position;
typedef struct _TrackWidget    TrackWidget;
typedef struct Channel         Channel;
typedef struct MidiEvents      MidiEvents;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable     Automatable;
typedef struct ZRegion         MidiRegion;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Initializes an midi track.
 */
void
midi_track_init (Track * track);

void
midi_track_setup (Track * self);

/**
 * Fills MIDI event queue from track.
 *
 * The events are dequeued right after the call to
 * this function.
 *
 * @note The engine splits the cycle so transport
 *   loop related logic is not needed.
 *
 * Caveats:
 * - This will not work properly if the loop sizes
 *   (region or transport) are smaller than nframes,
 *   so small sizes should not be allowed.
 *
 * @param g_start_frames Global start frame.
 * @param local_start_frame The start frame offset
 *   from 0 in this cycle.
 * @param nframes Number of frames at start
 *   Position.
 * @param midi_events MidiEvents to fill (from
 *   Piano Roll Port for example).
 */
REALTIME
void
midi_track_fill_midi_events (
  Track *         track,
  const long      g_start_frames,
  const nframes_t local_start_frame,
  nframes_t       nframes,
  MidiEvents *    midi_events);

/**
 * Frees the track.
 *
 * TODO
 */
void
midi_track_free (Track * track);

/**
 * @}
 */

#endif // __AUDIO_MIDI_TRACK_H__
