// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_REGION_LINK_GROUP_H__
#define __AUDIO_REGION_LINK_GROUP_H__

#include "dsp/region_identifier.h"

class Region;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define REGION_LINK_GROUP_MAGIC 1222013
#define IS_REGION_LINK_GROUP(x) \
  (((RegionLinkGroup *) (x))->magic_ == REGION_LINK_GROUP_MAGIC)

/**
 * A group of linked regions.
 */
class RegionLinkGroup final : public ISerializable<RegionLinkGroup>
{
public:
  RegionLinkGroup () = default;
  RegionLinkGroup (int idx) : group_idx_ (idx) { }
  void add_region (Region &region);

  /**
   * Remove the region from the link group.
   *
   * @param autoremove_last_region_and_group Automatically remove the last
   * region left in the group, and the group itself when empty.
   */
  void remove_region (
    Region &region,
    bool    autoremove_last_region_and_group,
    bool    update_identifier);

  bool contains_region (const Region &region) const;

  /**
   * Updates all other regions in the link group.
   *
   * @param region The region where the change happened.
   */
  void update (const Region &region);

  bool validate () const;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Identifiers for regions in this link group. */
  std::vector<RegionIdentifier> ids_;

  int magic_ = REGION_LINK_GROUP_MAGIC;

  /** Group index. */
  int group_idx_ = -1;
};

template <> struct fmt::formatter<RegionLinkGroup>
{
  constexpr auto parse (format_parse_context &ctx) { return ctx.begin (); }

  template <typename FormatContext>
  auto format (const RegionLinkGroup &rlg, FormatContext &ctx)
  {
    return format_to (
      ctx.out (), "RegionLinkGroup {{ group_idx: {}, ids: [{}] }}",
      rlg.group_idx_, fmt::join (rlg.ids_, ", "));
  }
};

/**
 * @}
 */

#endif
