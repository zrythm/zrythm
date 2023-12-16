// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Manager of linked region groups.
 */

#ifndef __AUDIO_REGION_LINK_GROUP_MANAGER_H__
#define __AUDIO_REGION_LINK_GROUP_MANAGER_H__

#include "dsp/region_link_group.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define REGION_LINK_GROUP_MANAGER (PROJECT->region_link_group_manager)

/**
 * Manager of region link groups.
 */
typedef struct RegionLinkGroupManager
{
  /** Region link groups. */
  RegionLinkGroup ** groups;
  int                num_groups;
  size_t             groups_size;
} RegionLinkGroupManager;

void
region_link_group_manager_init_loaded (RegionLinkGroupManager * self);

RegionLinkGroupManager *
region_link_group_manager_new (void);

/**
 * Adds a group and returns its index.
 */
int
region_link_group_manager_add_group (RegionLinkGroupManager * self);

RegionLinkGroup *
region_link_group_manager_get_group (RegionLinkGroupManager * self, int group_id);

/**
 * Removes the group.
 */
void
region_link_group_manager_remove_group (
  RegionLinkGroupManager * self,
  int                      group_id);

NONNULL bool
region_link_group_manager_validate (RegionLinkGroupManager * self);

NONNULL void
region_link_group_manager_print (RegionLinkGroupManager * self);

NONNULL RegionLinkGroupManager *
region_link_group_manager_clone (RegionLinkGroupManager * src);

NONNULL void
region_link_group_manager_free (RegionLinkGroupManager * self);

/**
 * @}
 */

#endif
