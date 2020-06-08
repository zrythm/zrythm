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
 * Object to hold information for the Tempo track.
 */

#ifndef __AUDIO_TEMPO_TRACK_H__
#define __AUDIO_TEMPO_TRACK_H__

#include <stdint.h>

#include "audio/track.h"
#include "utils/types.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define P_TEMPO_TRACK (TRACKLIST->tempo_track)

/**
 * Creates the default tempo track.
 */
Track *
tempo_track_default (
  int   track_pos);

/**
 * Inits the tempo track.
 */
void
tempo_track_init (
  Track * track);

/**
 * Removes all objects from the tempo track.
 *
 * Mainly used in testing.
 */
void
tempo_track_clear (
  Track * self);

/**
 * Returns the BPM at the given pos.
 */
bpm_t
tempo_track_get_bpm_at_pos (
  const Track *    track,
  const Position * pos);

/**
 * Frees the Track.
 */
void
tempo_track_free (
  Track * self);

/**
 * @}
 */

#endif
