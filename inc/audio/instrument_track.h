/*
 * audio/instrument_track.h - instrument track
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

#ifndef __AUDIO_INSTRUMENT_TRACK_H__
#define __AUDIO_INSTRUMENT_TRACK_H__

#include "audio/track.h"

#include <jack/jack.h>

typedef struct Position Position;
typedef struct _TrackWidget TrackWidget;
typedef struct Channel Channel;
typedef struct MidiEvents MidiEvents;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable Automatable;
typedef struct MidiRegion MidiRegion;
typedef jack_nframes_t nframes_t;

/**
 * The track struct.
 */
typedef struct InstrumentTrack
{
  Track                 parent; ///< base track

  /**
   * MIDI regions in this track.
   */
  MidiRegion *          regions[200];
  int                   num_regions;  ///< counter
  Channel *             channel; ///< owner channel, 1 channel has 1 track
  AutomationTracklist * automation_tracklist;
} InstrumentTrack;

InstrumentTrack *
instrument_track_new (Channel * channel);

void
instrument_track_setup (InstrumentTrack * self);

/**
 * NOTE: real time func
 */
void
instrument_track_fill_midi_events (InstrumentTrack      * track,
                        Position   * pos, ///< start position to check
                        nframes_t  nframes, ///< n of frames from start pos
                        MidiEvents * midi_events); ///< midi events to fill

void
instrument_track_add_region (InstrumentTrack      * track,
                  MidiRegion     * region);

void
instrument_track_remove_region (InstrumentTrack    * track,
                     MidiRegion   * region);

/**
 * Frees the track.
 *
 * TODO
 */
void
instrument_track_free (InstrumentTrack * track);

#endif // __AUDIO_INSTRUMENT_TRACK_H__
