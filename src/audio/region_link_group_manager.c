/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#include "audio/region_link_group_manager.h"
#include "utils/arrays.h"
#include "utils/mem.h"
#include "utils/objects.h"

void
region_link_group_manager_init (
  RegionLinkGroupManager * self)
{
  self->schema_version =
    REGION_LINK_GROUP_MANAGER_SCHEMA_VERSION;
}

void
region_link_group_manager_init_loaded (
  RegionLinkGroupManager * self)
{
  g_message ("Initializing...");

  for (int i = 0; i < self->num_groups; i++)
    {
      self->groups[i].magic =
        REGION_LINK_GROUP_MAGIC;
    }

  g_message ("done");
}

/**
 * Adds a group and returns its index.
 */
int
region_link_group_manager_add_group (
  RegionLinkGroupManager * self)
{
  g_warn_if_fail (
    self->num_groups >= 0);

  array_double_size_if_full (
    self->groups, self->num_groups,
    self->groups_size,
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
  g_warn_if_fail (
    self->num_groups >= 0);

  RegionLinkGroup * group = &self->groups[group_id];
  g_return_val_if_fail (
    IS_REGION_LINK_GROUP (group), NULL);
  return group;
}

/**
 * Removes the group.
 */
void
region_link_group_manager_remove_group (
  RegionLinkGroupManager * self,
  int                      group_id)
{
  g_warn_if_fail (
    self->num_groups > 0 &&
    self->groups_size > 0);

  /* only allow removing empty groups */
  RegionLinkGroup * group =
    region_link_group_manager_get_group (
      self, group_id);
  g_return_if_fail (group && group->num_ids == 0);

  --self->num_groups;
  for (int j = group_id; j < self->num_groups; j++)
    {
      group = &self->groups[j];
      RegionLinkGroup * next_group =
        &self->groups[j + 1];

      /* move the next group to the current group
       * slot */
      region_link_group_move (
        group, next_group);
    }

  g_warn_if_fail (self->num_groups >= 0);
}
