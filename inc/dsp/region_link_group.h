// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Group of linked regions.
 */

#ifndef __AUDIO_REGION_LINK_GROUP_H__
#define __AUDIO_REGION_LINK_GROUP_H__

#include "dsp/region_identifier.h"

typedef struct Region Region;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define REGION_LINK_GROUP_MAGIC 1222013
#define IS_REGION_LINK_GROUP(x) \
  (((RegionLinkGroup *) (x))->magic == REGION_LINK_GROUP_MAGIC)

/**
 * A group of linked regions.
 */
typedef struct RegionLinkGroup
{
  /** Identifiers for regions in this link group. */
  RegionIdentifier * ids;
  int                num_ids;
  size_t             ids_size;

  int magic;

  /** Group index. */
  int group_idx;
} RegionLinkGroup;

NONNULL void
region_link_group_init_loaded (RegionLinkGroup * self);

RegionLinkGroup *
region_link_group_new (int idx);

NONNULL void
region_link_group_add_region (RegionLinkGroup * self, Region * region);

/**
 * Remove the region from the link group.
 *
 * @param autoremove_last_region_and_group
 *   Automatically remove the last region left in
 *   the group, and the group itself when empty.
 */
NONNULL void
region_link_group_remove_region (
  RegionLinkGroup * self,
  Region *          region,
  bool              autoremove_last_region_and_group,
  bool              update_identifier);

NONNULL bool
region_link_group_contains_region (RegionLinkGroup * self, Region * region);

NONNULL void
region_link_group_print (RegionLinkGroup * self);

/**
 * Updates all other regions in the link group.
 *
 * @param region The region where the change
 *   happened.
 */
NONNULL void
region_link_group_update (RegionLinkGroup * self, Region * region);

NONNULL bool
region_link_group_validate (RegionLinkGroup * self);

RegionLinkGroup *
region_link_group_clone (const RegionLinkGroup * src);

void
region_link_group_free (RegionLinkGroup * self);

/**
 * @}
 */

#endif
