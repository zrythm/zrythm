/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Object to hold information for the chord track.
 *
 * Contains project scale, chord markers, etc.
 */

#ifndef __AUDIO_CHORD_TRACK_H__
#define __AUDIO_CHORD_TRACK_H__

#include <stdint.h>

#include "audio/track.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define P_CHORD_TRACK (TRACKLIST->chord_track)

typedef struct ChordObject       ChordObject;
typedef struct _ChordTrackWidget ChordTrackWidget;
typedef struct MusicalScale      MusicalScale;

typedef struct Track ChordTrack;

/**
 * Creates a new chord Track.
 */
ChordTrack *
chord_track_new (int track_pos);

/**
 * Inits a chord track (e.g. when cloning).
 */
void
chord_track_init (Track * track);

/**
 * Inserts a chord region to the Track at the given
 * index.
 */
void
chord_track_insert_chord_region (
  ChordTrack * track,
  ZRegion *    region,
  int          idx);

/**
 * Inserts a scale to the track.
 */
void
chord_track_insert_scale (
  ChordTrack *  track,
  ScaleObject * scale,
  int           pos);

/**
 * Adds a scale to the track.
 */
void
chord_track_add_scale (ChordTrack * track, ScaleObject * scale);

/**
 * Removes a scale from the chord Track.
 */
void
chord_track_remove_scale (
  ChordTrack *  self,
  ScaleObject * scale,
  bool          free);

/**
 * Removes a region from the chord track.
 */
void
chord_track_remove_region (ChordTrack * self, ZRegion * region);

bool
chord_track_validate (Track * self);

/**
 * Returns the current chord.
 */
#define chord_track_get_chord_at_playhead(ct) \
  chord_track_get_chord_at_pos (ct, PLAYHEAD)

/**
 * Returns the ChordObject at the given Position
 * in the TimelineArranger.
 */
ChordObject *
chord_track_get_chord_at_pos (
  const Track *    ct,
  const Position * pos);

/**
 * Returns the current scale.
 */
#define chord_track_get_scale_at_playhead(ct) \
  chord_track_get_scale_at_pos (ct, PLAYHEAD)

/**
 * Returns the ScaleObject at the given Position
 * in the TimelineArranger.
 */
ScaleObject *
chord_track_get_scale_at_pos (
  const Track *    ct,
  const Position * pos);

/**
 * Removes all objects from the chord track.
 *
 * Mainly used in testing.
 */
void
chord_track_clear (ChordTrack * self);

/**
 * @}
 */

#endif
