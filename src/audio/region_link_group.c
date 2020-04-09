/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/region.h"
#include "audio/region_link_group.h"
#include "audio/region_link_group_manager.h"
#include "project.h"
#include "utils/arrays.h"

void
region_link_group_init (
  RegionLinkGroup * self,
  int               idx)
{
  self->num_ids = 0;
  self->ids_size = 0;
  self->ids = NULL;
  self->group_idx = idx;
  self->magic = REGION_LINK_GROUP_MAGIC;
}

bool
region_link_group_contains_region (
  RegionLinkGroup * self,
  ZRegion *         region)
{
  for (int i = 0; i < self->num_ids; i++)
    {
      if (region_identifier_is_equal (
            &self->ids[i], &region->id))
        return true;
    }
  return false;
}

void
region_link_group_add_region (
  RegionLinkGroup * self,
  ZRegion *         region)
{
  if (region_link_group_contains_region (
        self, region))
    return;

  g_return_if_fail (IS_REGION_LINK_GROUP (self));
  array_double_size_if_full (
    self->ids, self->num_ids, self->ids_size,
    RegionIdentifier);
  region->id.link_group = self->group_idx;
  region_identifier_copy (
    &self->ids[self->num_ids++], &region->id);
}

/**
 * Remove the region from the link group.
 *
 * @param autoremove_last_region_and_group
 *   Automatically remove the last region left in
 *   the group, and the group itself when empty.
 */
void
region_link_group_remove_region (
  RegionLinkGroup * self,
  ZRegion *         region,
  bool              autoremove_last_region_and_group)
{
  g_return_if_fail (
    IS_REGION_LINK_GROUP (self) &&
    IS_REGION (region));
  for (int i = 0; i < self->num_ids; i++)
    {
      if (region_identifier_is_equal (
            &self->ids[i], &region->id))
        {
          --self->num_ids;
          for (int j = i; j < self->num_ids; j++)
            {
              region_identifier_copy (
                &self->ids[j], &self->ids[j + 1]);
            }
          break;
        }
    }

  if (autoremove_last_region_and_group)
    {
      /* if only one region left in group, remove
       * it */
      if (self->num_ids == 1)
        {
          region =
            region_find (&self->ids[0]);
          region_link_group_remove_region (
            self, region, true);
        }
      /* if no regions left, remove the group */
      else if (self->num_ids == 0)
        {
          g_warn_if_fail (
            REGION_LINK_GROUP_MANAGER->num_groups >
            0);
          region_link_group_manager_remove_group (
            REGION_LINK_GROUP_MANAGER,
            self->group_idx);
        }
    }

  region->id.link_group = -1;
}

/**
 * Updates all other regions in the link group.
 *
 * @param region The region where the change
 *   happened.
 */
void
region_link_group_update (
  RegionLinkGroup * self,
  ZRegion *         main_region)
{
  for (int i = 0; i < self->num_ids; i++)
    {
      ZRegion * region = region_find (&self->ids[i]);
      g_return_if_fail (region);

      if (region_identifier_is_equal (
           &self->ids[i], &main_region->id))
        continue;

      /* delete and readd all children */
      region_remove_all_children (region);
      region_copy_children (region, main_region);
    }
}

/**
 * Moves the regions from \ref src to \ref dest.
 */
void
region_link_group_move (
  RegionLinkGroup * dest,
  RegionLinkGroup * src)
{
  dest->num_ids = src->num_ids;
  src->group_idx = dest->group_idx;
  for (int i = 0; dest->num_ids; i++)
    {
      ZRegion * region =
        region_find (&src->ids[i]);
      region_set_link_group (
        region, dest->group_idx);
      region_identifier_copy (
        &dest->ids[i], &region->id);
    }
}
