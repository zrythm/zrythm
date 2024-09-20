// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_REGION_LINK_GROUP_MANAGER_H__
#define __AUDIO_REGION_LINK_GROUP_MANAGER_H__

#include "dsp/region_link_group.h"
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
class RegionLinkGroupManager final : public ISerializable<RegionLinkGroupManager>
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

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Region link groups. */
  std::vector<RegionLinkGroup> groups_;
};

DEFINE_OBJECT_FORMATTER (
  RegionLinkGroupManager,
  [] (const RegionLinkGroupManager &mgr) {
    return fmt::format (
      "RegionLinkGroupManager {{ groups: [{}] }}",
      fmt::join (mgr.groups_, ", "));
  });

/**
 * @}
 */

#endif
