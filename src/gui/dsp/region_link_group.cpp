// SPDX-FileCopyrightText: © 2020-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/region.h"
#include "gui/dsp/region_link_group.h"
#include "gui/dsp/region_link_group_manager.h"

bool
RegionLinkGroup::contains_region (const Region &region) const
{
  return std::ranges::contains (ids_, region.get_uuid ());
}

void
RegionLinkGroup::add_region (Region &region)
{
  if (contains_region (region))
    return;

  region.link_group_ = group_idx_;
  ids_.push_back (region.get_uuid ());
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
    "removing region '{}' from link group {} (num ids: {})", region.get_name (),
    group_idx_, ids_.size ());
  auto it = std::ranges::find (ids_, region.get_uuid ());
  z_return_if_fail (it != ids_.end ());

  ids_.erase (it);

  z_debug ("num ids after deletion: {}", ids_.size ());

  if (autoremove_last_region_and_group)
    {
      /* if only one region left in group, remove it */
      if (ids_.size () == 1)
        {
          auto last_region_var =
            PROJECT->find_arranger_object_by_id (ids_.front ());
          z_return_if_fail (last_region_var.has_value ());
          std::visit (
            [&] (auto &&last_region) {
              if constexpr (
                std::derived_from<base_type<decltype (last_region)>, Region>)
                {
                  remove_region (*last_region, true, update_identifier);
                }
            },
            last_region_var.value ());
        }
      /* if no regions left, remove the group */
      else if (ids_.empty ())
        {
          z_warn_if_fail (REGION_LINK_GROUP_MANAGER.groups_.size () > 0);
          REGION_LINK_GROUP_MANAGER.remove_group (group_idx_);
        }
    }

  region.link_group_.reset ();

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
      if (id == main_region.get_uuid ())
        continue;

      auto region_var = PROJECT->find_arranger_object_by_id (ids_.front ());
      z_return_if_fail (region_var.has_value ());
      std::visit (
        [&] (auto &&region) {
          using RegionT = base_type<decltype (region)>;
          if constexpr (std::derived_from<RegionT, Region>)
            {
              z_debug (
                "updating '{}' ({})", region->get_name (), region->get_uuid ());
              if constexpr (RegionWithChildren<RegionT>)
                {
                  /* delete and readd all children */
                  region->remove_all_children ();
                  auto main_region_casted =
                    dynamic_cast<const RegionT *> (&main_region);
                  region->copy_children (*main_region_casted);
                }
            }
        },
        region_var.value ());
    }
}

bool
RegionLinkGroup::validate () const
{
// TODO
#if 0
  for (const auto &id : ids_)
    {
      auto region_var = PROJECT->find_arranger_object_by_id (ids_.front ());
      z_return_val_if_fail (region_var.has_value (), false);
      return std::visit (
        [&] (auto &&region) {
          if constexpr (std::derived_from<base_type<decltype (region)>, Region>)
          {
            RegionLinkGroup * link_group = region->get_link_group ();
            z_return_val_if_fail (link_group == this, false);
            return true;
          }
        },
        region_var.value ());
    }
#endif

  return true;
}
