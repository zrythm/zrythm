// SPDX-FileCopyrightText: © 2020-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/dsp/region_link_group.h"
#include "utils/format.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define REGION_LINK_GROUP_MANAGER (PROJECT->region_link_group_manager_)

/**
 * Manager of region link groups.
 */
class RegionLinkGroupManager final
{
public:
  /**
   * Adds a group and returns its index.
   */
  int add_group ();

  RegionLinkGroup * get_group (int group_id);

  /**
   * Removes the group.
   */
  void remove_group (int group_id);

  bool validate () const;

private:
  static constexpr auto kGroupsKey = "groups"sv;
  friend void to_json (nlohmann::json &j, const RegionLinkGroupManager &mgr)
  {
    j[kGroupsKey] = mgr.groups_;
  }
  friend void from_json (const nlohmann::json &j, RegionLinkGroupManager &mgr);

public:
  /** Region link groups. */
  std::vector<RegionLinkGroup> groups_;
};

DEFINE_OBJECT_FORMATTER (
  RegionLinkGroupManager,
  RegionLinkGroupManager,
  [] (const RegionLinkGroupManager &mgr) {
    return fmt::format (
      "RegionLinkGroupManager {{ groups: [{}] }}",
      fmt::join (mgr.groups_, ", "));
  });

/**
 * @}
 */
