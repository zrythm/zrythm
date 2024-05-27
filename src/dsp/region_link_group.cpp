// clang-format off
// SPDX-FileCopyrightText: © 2020-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include "dsp/region.h"
#include "dsp/region_link_group.h"
#include "dsp/region_link_group_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/mem.h"
#include "utils/objects.h"

void
region_link_group_init_loaded (RegionLinkGroup * self)
{
  self->magic = REGION_LINK_GROUP_MAGIC;
  self->ids_size = (size_t) self->num_ids;
}

RegionLinkGroup *
region_link_group_new (int idx)
{
  RegionLinkGroup * self = object_new (RegionLinkGroup);

  self->num_ids = 0;
  self->ids_size = 0;
  self->ids = NULL;
  self->group_idx = idx;
  self->magic = REGION_LINK_GROUP_MAGIC;

  return self;
}

bool
region_link_group_contains_region (RegionLinkGroup * self, Region * region)
{
  for (int i = 0; i < self->num_ids; i++)
    {
      if (region_identifier_is_equal (&self->ids[i], &region->id))
        return true;
    }
  return false;
}

void
region_link_group_add_region (RegionLinkGroup * self, Region * region)
{
  if (region_link_group_contains_region (self, region))
    return;

  g_return_if_fail (region->id.idx >= 0);
  g_return_if_fail (IS_REGION_LINK_GROUP (self));

  array_double_size_if_full (
    self->ids, self->num_ids, self->ids_size, RegionIdentifier);
  region->id.link_group = self->group_idx;
  region_identifier_copy (&self->ids[self->num_ids++], &region->id);
}

void
region_link_group_remove_region (
  RegionLinkGroup * self,
  Region *          region,
  bool              autoremove_last_region_and_group,
  bool              update_identifier)
{
  g_return_if_fail (
    IS_REGION_LINK_GROUP (self) && IS_REGION (region) && self->num_ids > 0
    && self->group_idx < REGION_LINK_GROUP_MANAGER->num_groups);

  g_message (
    "removing region '%s' from link group %d (num ids: %d)", region->name,
    self->group_idx, self->num_ids);
  bool found = false;
  for (int i = 0; i < self->num_ids; i++)
    {
      if (region_identifier_is_equal (&self->ids[i], &region->id))
        {
          found = true;
          self->num_ids--;
          for (int j = i; j < self->num_ids; j++)
            {
              region_identifier_copy (&self->ids[j], &self->ids[j + 1]);
            }
          break;
        }
    }
  g_return_if_fail (found);

  g_debug ("num ids after deletion: %d", self->num_ids);

  if (autoremove_last_region_and_group)
    {
      /* if only one region left in group, remove it */
      if (self->num_ids == 1)
        {
          Region * last_region = region_find (&self->ids[0]);
          region_link_group_remove_region (
            self, last_region, true, update_identifier);
        }
      /* if no regions left, remove the group */
      else if (self->num_ids == 0)
        {
          g_warn_if_fail (REGION_LINK_GROUP_MANAGER->num_groups > 0);
          region_link_group_manager_remove_group (
            REGION_LINK_GROUP_MANAGER, self->group_idx);
        }
    }

  region->id.link_group = -1;

  if (update_identifier)
    region_update_identifier (region);
}

/**
 * Updates all other regions in the link group.
 *
 * @param region The region where the change
 *   happened.
 */
void
region_link_group_update (RegionLinkGroup * self, Region * main_region)
{
  for (int i = 0; i < self->num_ids; i++)
    {
      Region * region = region_find (&self->ids[i]);
      g_return_if_fail (IS_REGION_AND_NONNULL (region));

      if (region_identifier_is_equal (&self->ids[i], &main_region->id))
        continue;

      g_message (
        "[%s] updating %d (%d %s)", __func__, i, region->id.idx, region->name);

      /* delete and readd all children */
      region_remove_all_children (region);
      region_copy_children (region, main_region);
    }
}

bool
region_link_group_validate (RegionLinkGroup * self)
{
  for (int i = 0; i < self->num_ids; i++)
    {
      Region * region = region_find (&self->ids[i]);
      g_return_val_if_fail (IS_REGION_AND_NONNULL (region), false);
      RegionLinkGroup * link_group = region_get_link_group (region);
      g_return_val_if_fail (link_group == self, false);
    }

  return true;
}

void
region_link_group_print (RegionLinkGroup * self)
{
  /* TODO */
}

#if 0
/**
 * Moves the regions from \ref src to \ref dest.
 */
void
region_link_group_move (
  RegionLinkGroup * dest,
  RegionLinkGroup * src)
{
  dest->num_ids = 0;
  /*dest->group_idx = src->group_idx;*/
  for (int i = 0; src->num_ids; i++)
    {
      Region * region =
        region_find (&src->ids[i]);
      region_set_link_group (
        region, dest->group_idx, true);
      region_identifier_copy (
        &dest->ids[i], &region->id);
      dest->num_ids++;
    }
}
#endif

RegionLinkGroup *
region_link_group_clone (const RegionLinkGroup * src)
{
  RegionLinkGroup * self = object_new (RegionLinkGroup);
  self->magic = REGION_LINK_GROUP_MAGIC;

  self->group_idx = src->group_idx;
  self->ids = object_new_n ((size_t) src->num_ids, RegionIdentifier);
  for (int i = 0; i < src->num_ids; i++)
    {
      region_identifier_copy (&self->ids[i], &src->ids[i]);
    }
  self->num_ids = src->num_ids;

  return self;
}

void
region_link_group_free (RegionLinkGroup * self)
{
  object_zero_and_free (self->ids);

  object_zero_and_free (self);
}
