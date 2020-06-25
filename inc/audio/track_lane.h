/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Track lanes for each track.
 */

#ifndef __AUDIO_TRACK_LANE_H__
#define __AUDIO_TRACK_LANE_H__

#include "audio/region.h"
#include "utils/yaml.h"

typedef struct _TrackLaneWidget TrackLaneWidget;
typedef struct CustomButtonWidget
  CustomButtonWidget;
typedef void MIDI_FILE;

/**
 * @addtogroup audio
 *
 * @{
 */
#define MAX_REGIONS 300

/**
 * A TrackLane belongs to a Track (can have many
 * TrackLanes in a Track) and contains Regions.
 *
 * Only Tracks that have Regions can have TrackLanes,
 * such as InstrumentTrack and AudioTrack.
 */
typedef struct TrackLane
{
  /** Position in the Track. */
  int                 pos;

  /** Name of lane, e.g. "Lane 1". */
  char *              name;

  /** TrackLaneWidget for this lane. */
  TrackLaneWidget *   widget;

  /** Y local to track. */
  int                 y;

  /** Position of handle. */
  int                 height;

  /** Muted or not. */
  int                 mute;

  /** Soloed or not. */
  int                 solo;

  /** Regions in this track. */
  ZRegion **           regions;
  int                 num_regions;
  size_t              regions_size;

  /** Position of owner Track in the Tracklist. */
  int                 track_pos;

  /**
   * MIDI channel, if MIDI lane, starting at 1.
   *
   * If this is set to 0, the value will be
   * inherited from the Track.
   */
  uint8_t             midi_ch;

  /** Buttons used by the track widget. */
  CustomButtonWidget * buttons[8];
  int                  num_buttons;

} TrackLane;

static const cyaml_schema_field_t
track_lane_fields_schema[] =
{
	CYAML_FIELD_INT (
    "pos", CYAML_FLAG_DEFAULT,
    TrackLane, pos),
  CYAML_FIELD_STRING_PTR (
    "name", CYAML_FLAG_POINTER,
    TrackLane, name,
   	0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "height", CYAML_FLAG_DEFAULT,
    TrackLane, height),
	CYAML_FIELD_INT (
    "mute", CYAML_FLAG_DEFAULT,
    TrackLane, mute),
	CYAML_FIELD_INT (
    "solo", CYAML_FLAG_DEFAULT,
    TrackLane, solo),
  CYAML_FIELD_SEQUENCE_COUNT (
    "regions",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    TrackLane, regions, num_regions,
    &region_schema, 0, CYAML_UNLIMITED),
	CYAML_FIELD_INT (
    "track_pos", CYAML_FLAG_DEFAULT,
    TrackLane, track_pos),
	CYAML_FIELD_UINT (
    "midi_ch", CYAML_FLAG_DEFAULT,
    TrackLane, midi_ch),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
track_lane_schema = {
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    TrackLane, track_lane_fields_schema),
};

void
track_lane_init_loaded (
  TrackLane * lane);

/**
 * Creates a new TrackLane at the given pos in the
 * given Track.
 *
 * @param track The Track to create the TrackLane
 *   for.
 * @param pos The position (index) in the Track that
 *   this lane will be placed in.
 */
TrackLane *
track_lane_new (
  Track * track,
  int     pos);

/**
 * Adds a ZRegion to the given TrackLane.
 */
void
track_lane_add_region (
  TrackLane * self,
  ZRegion *    region);

/**
 * Removes but does not free the region.
 */
void
track_lane_remove_region (
  TrackLane * self,
  ZRegion *   region);

/**
 * Unselects all arranger objects.
 */
void
track_lane_unselect_all (
  TrackLane * self);

/**
 * Removes all objects recursively from the track
 * lane.
 */
void
track_lane_clear (
  TrackLane * self);

/**
 * Updates the frames of each position in each child
 * of the track recursively.
 */
void
track_lane_update_frames (
  TrackLane * self);

/**
 * Sets the track position to the lane and all its
 * members recursively.
 */
void
track_lane_set_track_pos (
  TrackLane * self,
  const int   pos);

/**
 * Clones the TrackLane.
 *
 * Mainly used when cloning Track's.
 */
TrackLane *
track_lane_clone (
  TrackLane * lane);

/**
 * Writes the lane to the given MIDI file.
 */
void
track_lane_write_to_midi_file (
  TrackLane * self,
  MIDI_FILE * mf);

Track *
track_lane_get_track (
  TrackLane * self);

/**
 * Frees the TrackLane.
 */
void
track_lane_free (
  TrackLane * lane);

/**
 * @}
 */

#endif // __AUDIO_TRACK_LANE_H__
