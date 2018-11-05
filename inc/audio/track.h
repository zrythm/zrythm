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
  Region          * regions[200];     ///< regions in this track
  int             num_regions;  ///< counter
  TrackWidget     * widget; ///< track widget, 1 track has 1 widget
  Channel         * channel;  ///< owner channel, 1 channel has 1 track
  AutomationTrack * automation_tracks[4000]; ///< automation tracks for this track
                          ///< these should be updated with ALL of the automatables
                          ///< available in the channel and its plugins, every time
                          ///< there is an update
  int             num_automation_tracks;
  int             automations_visible; ///< flag to set automations visible or not
} Track;

/**
 * Updates the automation tracks in the track. (adds missing)
 *
 * Builds an automation track for each automatable in the channel and its plugins,
 * unless it already exists.
 */
void
track_update_automation_tracks (Track * track);

Track *
track_new (Channel * channel);

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

void
track_delete_automation_track (Track *           track,
                               AutomationTrack * at);

/**
 * Frees the track.
 *
 * TODO
 */
void
track_free (Track * track);

#endif // __AUDIO_TRACK_H__
