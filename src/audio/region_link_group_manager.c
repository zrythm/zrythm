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

#include <stdlib.h>

#include "audio/region_link_group_manager.h"
#include "utils/arrays.h"
#include "utils/mem.h"
#include "utils/objects.h"

void
region_link_group_manager_init_loaded (
  RegionLinkGroupManager * self)
{
  self->groups_size = (size_t) self->num_groups;
  for (int i = 0; i < self->num_groups; i++)
    {
      RegionLinkGroup * link_group =
        self->groups[i];
      region_link_group_init_loaded (link_group);
    }
}

bool
region_link_group_manager_validate (
  RegionLinkGroupManager * self)
{
  for (int i = 0; i < self->num_groups; i++)
    {
      RegionLinkGroup * link_group =
        self->groups[i];
      if (!region_link_group_validate (link_group))
        {
          g_return_val_if_reached (false);
        }
    }

  return true;
}

/**
 * Adds a group and returns its index.
 */
int
region_link_group_manager_add_group (
  RegionLinkGroupManager * self)
{
  g_warn_if_fail (self->num_groups >= 0);

  array_double_size_if_full (
    self->groups, self->num_groups,
    self->groups_size, RegionLinkGroup *);
  self->groups[self->num_groups] =
    region_link_group_new (self->num_groups);
  self->num_groups++;

  return self->num_groups - 1;
}

RegionLinkGroup *
region_link_group_manager_get_group (
  RegionLinkGroupManager * self,
  int                      group_id)
{
  g_return_val_if_fail (
    self->num_groups >= 0
      && group_id < self->num_groups,
    NULL);

  RegionLinkGroup * group = self->groups[group_id];
  g_return_val_if_fail (
    IS_REGION_LINK_GROUP (group)
      && group->group_idx == group_id,
    NULL);
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
    self->num_groups > 0 && self->groups_size > 0);

  /* only allow removing empty groups */
  RegionLinkGroup * group =
    region_link_group_manager_get_group (
      self, group_id);
  g_return_if_fail (group && group->num_ids == 0);

  for (int j = self->num_groups - 2; j >= group_id;
       j--)
    {
      /* move the next group to the current group
       * slot */
      self->groups[j] = self->groups[j + 1];
      self->groups[j]->group_idx = j;
    }
  self->num_groups--;

  object_free_w_func_and_null (
    region_link_group_free, group);

  g_return_if_fail (self->num_groups >= 0);
}

void
region_link_group_manager_print (
  RegionLinkGroupManager * self)
{
  char * str = yaml_serialize (
    self, &region_link_group_manager_schema);
  g_message ("%s", str);
  g_free (str);
}

RegionLinkGroupManager *
region_link_group_manager_new (void)
{
  RegionLinkGroupManager * self =
    object_new (RegionLinkGroupManager);
  self->schema_version =
    REGION_LINK_GROUP_MANAGER_SCHEMA_VERSION;

  return self;
}

RegionLinkGroupManager *
region_link_group_manager_clone (
  RegionLinkGroupManager * src)
{
  RegionLinkGroupManager * self =
    object_new (RegionLinkGroupManager);
  self->schema_version =
    REGION_LINK_GROUP_MANAGER_SCHEMA_VERSION;

  self->groups = object_new_n (
    (size_t) src->num_groups, RegionIdentifier);
  for (int i = 0; i < src->num_groups; i++)
    {
      self->groups[i] =
        region_link_group_clone (src->groups[i]);
    }
  self->num_groups = src->num_groups;

  return self;
}

void
region_link_group_manager_free (
  RegionLinkGroupManager * self)
{
  for (int i = 0; i < self->num_groups; i++)
    {
      object_free_w_func_and_null (
        region_link_group_free, self->groups[i]);
    }
  object_zero_and_free (self->groups);

  object_zero_and_free (self);
}
