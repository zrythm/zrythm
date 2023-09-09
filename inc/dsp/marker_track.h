// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Object to hold information for the Marker track.
 */

#ifndef __AUDIO_MARKER_TRACK_H__
#define __AUDIO_MARKER_TRACK_H__

#include <stdint.h>

#include "dsp/track.h"

/**
 * @addtogroup dsp
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
marker_track_insert_marker (MarkerTrack * self, Marker * marker, int pos);

/**
 * Adds a marker to the track.
 */
void
marker_track_add_marker (MarkerTrack * self, Marker * marker);

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
marker_track_remove_marker (MarkerTrack * self, Marker * marker, int free);

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
