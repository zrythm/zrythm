/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#define P_MARKER_TRACK (RULER_TRACKLIST->marker_track)

typedef struct Marker Marker;
typedef struct _MarkerTrackWidget MarkerTrackWidget;

/** MarkerTrack is just a Track. */
typedef struct Track MarkerTrack;

/**
 * Creates the default marker track.
 */
MarkerTrack *
marker_track_default ();

void
marker_track_add_marker (
  MarkerTrack * self,
  Marker *      marker);

void
marker_track_remove_marker (
  MarkerTrack * self,
  Marker *      marker);

/**
 * Frees the MarkerTrack.
 */
void
marker_track_free (
  MarkerTrack * self);

/**
 * @}
 */

#endif
