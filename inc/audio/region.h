/*
 * audio/region.h - A region in the timeline having a start
 *   and an end
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

#ifndef __AUDIO_REGION_H__
#define __AUDIO_REGION_H__

#include "audio/position.h"

#define REGION_PRINTF_FILENAME "%s%sregions%s%d_%s_%s.mid"

typedef struct RegionWidget RegionWidget;
typedef struct Channel Channel;
typedef struct Track Track;
typedef struct MidiNote MidiNote;

typedef struct Region
{
  int          id;
  char         * name; ///< region name
  Position     start_pos;
  Position     end_pos;
  RegionWidget * widget;
  Track        * track; ///< owner track

  /* either the midi notes from this region, or the midi notes from the
   * linked region are used */
  struct Region       * linked_region; ///< linked parent region
  MidiNote     * midi_notes[200]; /// midi notes (if MIDI region)
  int          num_midi_notes;
} Region;

Region *
region_new (Track * track,
            Position * start_pos,
            Position * end_pos);

/**
 * Checks if position is valid then sets it.
 */
void
region_set_start_pos (Region * region,
                      Position * start_pos);

/**
 * Checks if position is valid then sets it.
 */
void
region_set_end_pos (Region * region,
                    Position * end_pos);

/**
 * Returns the region at the given position in the given channel
 */
Region *
region_at_position (Track    * track, ///< the track to look in
                    Position * pos); ///< the position

/**
 * Adds midi note to region
 */
void
region_add_midi_note (Region * region,
                      MidiNote * midi_note);

#endif // __AUDIO_REGION_H__
