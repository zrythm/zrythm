/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * ZRegion for ChordObject's.
 */

#ifndef __AUDIO_CHORD_REGION_H__
#define __AUDIO_CHORD_REGION_H__

#include <stdbool.h>

typedef struct Position    Position;
typedef struct ChordObject ChordObject;
typedef struct ZRegion     ZRegion;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Creates a new ZRegion for chords.
 *
 * @param idx Index inside chord track.
 */
NONNULL
ZRegion *
chord_region_new (
  const Position * start_pos,
  const Position * end_pos,
  int              idx);

/**
 * Inserts a ChordObject to the Region.
 */
NONNULL
void
chord_region_insert_chord_object (
  ZRegion *     self,
  ChordObject * chord,
  int           pos,
  bool          fire_events);

/**
 * Adds a ChordObject to the Region.
 */
NONNULL
void
chord_region_add_chord_object (
  ZRegion *     self,
  ChordObject * chord,
  bool          fire_events);

/**
 * Removes a ChordObject from the Region.
 *
 * @param free Optionally free the ChordObject.
 */
NONNULL
void
chord_region_remove_chord_object (
  ZRegion *     self,
  ChordObject * chord,
  int           free,
  bool          fire_events);

NONNULL
bool
chord_region_validate (ZRegion * self);

/**
 * Frees members only but not the ZRegion itself.
 *
 * Regions should be free'd using region_free.
 */
NONNULL
void
chord_region_free_members (ZRegion * self);

/**
 * @}
 */

#endif
