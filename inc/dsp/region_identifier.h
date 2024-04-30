// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Region identifier.
 *
 * This is in its own file to avoid recursive
 * inclusion.
 */

#ifndef __AUDIO_REGION_IDENTIFIER_H__
#define __AUDIO_REGION_IDENTIFIER_H__

#include <stdbool.h>

#include "utils/general.h"

#include <gtk/gtk.h>

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Type of Region.
 *
 * Bitfield instead of plain enum so multiple values can be passed to some
 * functions (eg to collect all Regions of the given types in a Track).
 */
typedef enum RegionType
{
  REGION_TYPE_MIDI = 1 << 0,
  REGION_TYPE_AUDIO = 1 << 1,
  REGION_TYPE_AUTOMATION = 1 << 2,
  REGION_TYPE_CHORD = 1 << 3,
} RegionType;

static const char * region_type_bitvals[] = {
  "midi",
  "audio",
  "automation",
  "chord",
};

/**
 * Index/identifier for a Region, so we can
 * get Region objects quickly with it without
 * searching by name.
 */
typedef struct RegionIdentifier
{
  RegionType type;

  /** Link group index, if any, or -1. */
  int link_group;

  unsigned int track_name_hash;
  int          lane_pos;

  /** Automation track index in the automation tracklist, if
   * automation region. */
  int at_idx;

  /** Index inside lane or automation track. */
  int idx;
} RegionIdentifier;

void
region_identifier_init (RegionIdentifier * self);

static inline int
region_identifier_is_equal (
  const RegionIdentifier * a,
  const RegionIdentifier * b)
{
  return a->idx == b->idx && a->track_name_hash == b->track_name_hash
         && a->lane_pos == b->lane_pos && a->at_idx == b->at_idx
         && a->link_group == b->link_group && a->type == b->type;
}

NONNULL static inline void
region_identifier_copy (RegionIdentifier * dest, const RegionIdentifier * src)
{
  dest->idx = src->idx;
  dest->track_name_hash = src->track_name_hash;
  dest->lane_pos = src->lane_pos;
  dest->at_idx = src->at_idx;
  dest->type = src->type;
  dest->link_group = src->link_group;
}

bool
region_identifier_validate (RegionIdentifier * self);

static inline const char *
region_identifier_get_region_type_name (RegionType type)
{
  g_return_val_if_fail (
    type >= REGION_TYPE_MIDI && type <= REGION_TYPE_CHORD, NULL);

  return region_type_bitvals[utils_get_uint_from_bitfield_val (type)];
}

static inline void
region_identifier_print (const RegionIdentifier * self)
{
  g_message (
    "Region identifier: "
    "type: %s, track name hash %u, lane pos %d, "
    "at index %d, index %d, link_group: %d",
    region_identifier_get_region_type_name (self->type), self->track_name_hash,
    self->lane_pos, self->at_idx, self->idx, self->link_group);
}

void
region_identifier_free (RegionIdentifier * self);

/**
 * @}
 */

#endif
