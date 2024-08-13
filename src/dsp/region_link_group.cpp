// SPDX-FileCopyrightText: © 2020-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/region.h"
#include "dsp/region_link_group.h"
#include "dsp/region_link_group_manager.h"
#include "project.h"
#include "zrythm.h"

bool
RegionLinkGroup::contains_region (const Region &region) const
{
  for (const auto &id : ids_)
    {
      if (id == region.id_)
        return true;
    }
  return false;
}

void
RegionLinkGroup::add_region (Region &region)
{
  if (contains_region (region))
    return;

  z_return_if_fail (region.id_.idx_ >= 0);
  z_return_if_fail (IS_REGION_LINK_GROUP (this));

  region.id_.link_group_ = group_idx_;
  ids_.push_back (region.id_);
}

void
RegionLinkGroup::remove_region (
  Region &region,
  bool    autoremove_last_region_and_group,
  bool    update_identifier)
{
  z_return_if_fail (
    !ids_.empty ()
    && group_idx_ < (int) REGION_LINK_GROUP_MANAGER.groups_.size ());

  z_debug (
    "removing region '%s' from link group %d (num ids: %zu)",
    region.get_name (), group_idx_, ids_.size ());
  auto it = std::find (ids_.begin (), ids_.end (), region.id_);
  z_return_if_fail (it != ids_.end ());

  ids_.erase (it);

  z_debug ("num ids after deletion: {}", ids_.size ());

  if (autoremove_last_region_and_group)
    {
      /* if only one region left in group, remove it */
      if (ids_.size () == 1)
        {
          auto last_region = Region::find (ids_[0]);
          remove_region (*last_region, true, update_identifier);
        }
      /* if no regions left, remove the group */
      else if (ids_.empty ())
        {
          z_warn_if_fail (REGION_LINK_GROUP_MANAGER.groups_.size () > 0);
          REGION_LINK_GROUP_MANAGER.remove_group (group_idx_);
        }
    }

  region.id_.link_group_ = -1;

  std::visit (
    [&] (auto &&r) {
      if (update_identifier)
        r->update_identifier ();
    },
    convert_to_variant<RegionPtrVariant> (&region));
}

void
RegionLinkGroup::update (const Region &main_region)
{
  for (const auto &id : ids_)
    {
      auto region = Region::find (id);
      z_return_if_fail (region);

      if (id == main_region.id_)
        continue;

      z_debug ("updating {} ({})", region->id_.idx_, region->get_name ());

      std::visit (
        [&] (auto &&r) {
          using T = base_type<decltype (r)>;
          if constexpr (RegionWithChildren<T>)
            {
              /* delete and readd all children */
              r->remove_all_children ();
              auto main_region_casted = dynamic_cast<const T *> (&main_region);
              r->copy_children (*main_region_casted);
            }
        },
        convert_to_variant<RegionPtrVariant> (region.get ()));
    }
}

bool
RegionLinkGroup::validate () const
{
  for (const auto &id : ids_)
    {
      auto region = Region::find (id);
      z_return_val_if_fail (region, false);
      RegionLinkGroup * link_group = region->get_link_group ();
      z_return_val_if_fail (link_group == this, false);
    }

  return true;
}