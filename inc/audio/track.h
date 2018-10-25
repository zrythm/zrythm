/*
 * audio/track.h - the back end for a timeline track
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

#ifndef __AUDIO_TRACK_H__
#define __AUDIO_TRACK_H__

#include <jack/jack.h>

typedef struct Region Region;
typedef struct Position Position;
typedef struct TrackWidget TrackWidget;
typedef struct Channel Channel;
typedef struct MidiEvents MidiEvents;
typedef struct AutomationTrack AutomationTrack;
typedef struct Automatable Automatable;
typedef jack_nframes_t nframes_t;


/**
 * The track struct.
 */
typedef struct Track {
  Region          * regions[200];     ///< array of region pointers
  int             num_regions;
  TrackWidget     * widget;
  Channel         * channel;  ///< owner
  AutomationTrack * automation_tracks[400];
  int             num_automation_tracks;
  Automatable     * automatables[4000];
  int             num_automatables;
} Track;

Track *
track_new (Channel * channel);

/**
 * (re)Generates automatables for the track.
 */
void
track_generate_automatables (Track * track);

/**
 * NOTE: real time func
 */
void
track_fill_midi_events (Track      * track,
                        Position   * pos, ///< start position to check
                        nframes_t  nframes, ///< n of frames from start pos
                        MidiEvents * midi_events); ///< midi events to fill

void
track_add_region (Track      * track,
                  Region     * region);

void
track_remove_region (Track    * track,
                     Region   * region);

/**
 * Convenience function to get the fader automatable of the track.
 */
Automatable *
track_get_fader_automatable (Track * track);

#endif // __AUDIO_TRACK_H__
