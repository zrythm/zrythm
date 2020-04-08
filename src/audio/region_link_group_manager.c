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

#include "audio/region_link_group_manager.h"
#include "utils/arrays.h"

void
region_link_group_manager_init (
  RegionLinkGroupManager * self)
{
}

/**
 * Adds a group and returns its index.
 */
int
region_link_group_manager_add_group (
  RegionLinkGroupManager * self)
{
  array_double_size_if_full (
    self->groups, self->num_groups, self->groups_size,
    RegionLinkGroup);
  region_link_group_init (
    &self->groups[self->num_groups],
    self->num_groups);
  self->num_groups++;

  return self->num_groups - 1;
}

RegionLinkGroup *
region_link_group_manager_get_group (
  RegionLinkGroupManager * self,
  int                      group_id)
{
  RegionLinkGroup * group = &self->groups[group_id];
  g_return_val_if_fail (
    IS_REGION_LINK_GROUP (group), NULL);
  return group;
}
