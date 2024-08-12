// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_REGION_LINK_GROUP_MANAGER_H__
#define __AUDIO_REGION_LINK_GROUP_MANAGER_H__

#include "dsp/region_link_group.h"

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

template <> struct fmt::formatter<RegionLinkGroupManager>
{
  constexpr auto parse (format_parse_context &ctx) { return ctx.begin (); }

  template <typename FormatContext>
  auto format (const RegionLinkGroupManager &rlgm, FormatContext &ctx)
  {
    return format_to (
      ctx.out (), "RegionLinkGroupManager {{ groups: [{}] }}",
      fmt::join (rlgm.groups_, ", "));
  }
};

/**
 * @}
 */

#endif
