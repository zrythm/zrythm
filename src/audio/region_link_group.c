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

void
region_link_group_add_region (
  RegionLinkGroup * self,
  ZRegion *         region)
{
  g_return_if_fail (IS_REGION_LINK_GROUP (self));
  array_double_size_if_full (
    self->ids, self->num_ids, self->ids_size,
    RegionIdentifier);
  region->id.link_group = self->group_idx;
  region_identifier_copy (
    &self->ids[self->num_ids++], &region->id);
}

void
region_link_group_remove_region (
  RegionLinkGroup * self,
  ZRegion *         region)
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
  region->id.link_group = -1;
}
