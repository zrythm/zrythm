// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Region for ChordObject's.
 */

#ifndef __AUDIO_CHORD_REGION_H__
#define __AUDIO_CHORD_REGION_H__

typedef struct Position    Position;
typedef struct ChordObject ChordObject;
typedef struct Region      Region;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Creates a new Region for chords.
 *
 * @param idx Index inside chord track.
 */
NONNULL Region *
chord_region_new (const Position * start_pos, const Position * end_pos, int idx);

/**
 * Inserts a ChordObject to the Region.
 */
NONNULL void
chord_region_insert_chord_object (
  Region *      self,
  ChordObject * chord,
  int           pos,
  bool          fire_events);

/**
 * Adds a ChordObject to the Region.
 */
NONNULL void
chord_region_add_chord_object (
  Region *      self,
  ChordObject * chord,
  bool          fire_events);

/**
 * Removes a ChordObject from the Region.
 *
 * @param free Optionally free the ChordObject.
 */
NONNULL void
chord_region_remove_chord_object (
  Region *      self,
  ChordObject * chord,
  int           free,
  bool          fire_events);

NONNULL bool
chord_region_validate (Region * self);

/**
 * Frees members only but not the Region itself.
 *
 * Regions should be free'd using region_free.
 */
NONNULL void
chord_region_free_members (Region * self);

/**
 * @}
 */

#endif
