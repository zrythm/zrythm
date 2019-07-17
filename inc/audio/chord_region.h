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
 * Region for ChordObject's.
 */

#ifndef __AUDIO_CHORD_REGION_H__
#define __AUDIO_CHORD_REGION_H__

typedef struct Position Position;
typedef struct ChordObject ChordObject;
typedef struct Region Region;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Creates a new Region for chords.
 *
 * @param is_main If this is 1 it
 *   will create the additional Region (
 *   main_transient).
 */
Region *
chord_region_new (
  const Position * start_pos,
  const Position * end_pos,
  const int        is_main);

/**
 * Adds a ChordObject to the Region.
 */
void
chord_region_add_chord_object (
  Region *      self,
  ChordObject * chord);

/**
 * Removes a ChordObject from the Region.
 *
 * @param free Optionally free the ChordObject.
 */
void
chord_region_remove_chord_object (
  Region *      self,
  ChordObject * chord,
  int           free);

/**
 * Frees members only but not the Region itself.
 *
 * Regions should be free'd using region_free.
 */
void
chord_region_free_members (
  Region * self);

/**
 * @}
 */

#endif
