/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Object to hold information for the Marker track.
 */

#ifndef __AUDIO_MARKER_TRACK_H__
#define __AUDIO_MARKER_TRACK_H__

#include <stdint.h>

#include "audio/track.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define P_MARKER_TRACK (TRACKLIST->marker_track)

typedef struct Marker             Marker;
typedef struct _MarkerTrackWidget MarkerTrackWidget;

/** MarkerTrack is just a Track. */
typedef struct Track MarkerTrack;

/**
 * Creates the default marker track.
 */
MarkerTrack *
marker_track_default (int track_pos);

/**
 * Inits the marker track.
 */
void
marker_track_init (Track * track);

/**
 * Inserts a marker to the track.
 */
void
marker_track_insert_marker (
  MarkerTrack * self,
  Marker *      marker,
  int           pos);

/**
 * Adds a marker to the track.
 */
void
marker_track_add_marker (
  MarkerTrack * self,
  Marker *      marker);

/**
 * Removes all objects from the marker track.
 *
 * Mainly used in testing.
 */
void
marker_track_clear (MarkerTrack * self);

/**
 * Removes a marker, optionally freeing it.
 */
void
marker_track_remove_marker (
  MarkerTrack * self,
  Marker *      marker,
  int           free);

bool
marker_track_validate (MarkerTrack * self);

/**
 * Returns the start marker.
 */
Marker *
marker_track_get_start_marker (const Track * track);

/**
 * Returns the end marker.
 */
Marker *
marker_track_get_end_marker (const Track * track);

/**
 * @}
 */

#endif
