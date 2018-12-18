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

#define REGION_PRINTF_FILENAME "%d_%s_%s.mid"

typedef struct _RegionWidget RegionWidget;
typedef struct Channel Channel;
typedef struct Track Track;
typedef struct MidiNote MidiNote;

typedef enum RegionType
{
  REGION_TYPE_MIDI,
  REGION_TYPE_AUDIO
} RegionType;

typedef struct Region
{
  /**
   * ID for saving/loading projects
   */
  int          id;

  /**
   * Region name to be shown on the region.
   */
  char         * name;

  RegionType   type;
  Position     start_pos; ///< start position
  Position     end_pos; ///< end position

  /**
   * Position where the first unit of repeation ends.
   *
   * If end pos > unit_end_pos, then region is repeating.
   */
  Position        unit_end_pos;

  /**
   * Region widget.
   */
  RegionWidget * widget;

  /**
   * Owner track.
   */
  Track        * track;

  /**
   * Linked parent region.
   *
   * Either the midi notes from this region, or the midi
   * notes from the linked region are used
   */
  struct Region       * linked_region;
} Region;

/**
 * Only to be used by implementing structs.
 */
void
region_init (Region *   region,
             RegionType type,
             Track *    track,
             Position * start_pos,
             Position * end_pos);

/**
 * Clamps position then sets it.
 */
void
region_set_start_pos (Region * region,
                      Position * pos,
                      int      moved); ///< region moved or not (to move notes as
                                          ///< well)

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
 * Generates the filename for this region.
 *
 * MUST be free'd.
 */
char *
region_generate_filename (Region * region);

#endif // __AUDIO_REGION_H__
