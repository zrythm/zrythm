// SPDX-FileCopyrightText: © 2020, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/region_link_group_manager.h"
#include "utils/logger.h"
#include "utils/views.h"

namespace zrythm::structure::arrangement
{
int
RegionLinkGroupManager::add_group ()
{
  groups_.emplace_back (groups_.size ());

  return groups_.size () - 1;
}

RegionLinkGroup *
RegionLinkGroupManager::get_group (int group_id)
{
  if (group_id >= (int) groups_.size ())
    {
      add_group ();
    }

  RegionLinkGroup * group = &groups_[group_id];
  z_return_val_if_fail (group->group_idx_ == group_id, nullptr);
  return group;
}

void
RegionLinkGroupManager::remove_group (int group_id)
{
  /* only allow removing empty groups */
  RegionLinkGroup * group = get_group (group_id);
  z_return_if_fail (group && group->ids_.empty ());

  for (int j = group_id; j < (int) groups_.size () - 1; j++)
    {
      groups_[j] = std::move (groups_[j + 1]);
      groups_[j].group_idx_ = j;
    }
  groups_.pop_back ();
}

void
from_json (const nlohmann::json &j, RegionLinkGroupManager &mgr)
{
  for (
    const auto &[index, group_json] :
    utils::views::enumerate (j.at (RegionLinkGroupManager::kGroupsKey)))
    {
      auto group = std::make_unique<RegionLinkGroup> (index);
      from_json (group_json, *group);
      mgr.groups_.push_back (std::move (*group.release ()));
    }
}
}
