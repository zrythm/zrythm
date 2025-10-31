// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"

#include "./plugin_span.h"

using namespace zrythm;

bool
PluginSpan::can_be_pasted (const plugins::PluginSlot &slot) const
{
  if (std::ranges::distance (*this) == 0)
    return false;

// TODO
#if 0
  auto slots = std::ranges::to<std::vector> (
    std::views::transform (*this, slot_projection));
  auto [highest_slot_it, lowest_slot_it] = std::ranges::minmax_element (slots);

  if ((*lowest_slot_it).has_slot_index () && slot.has_slot_index ())
    {
      auto delta =
        (*highest_slot_it).get_slot_with_index ().second
        - (*lowest_slot_it).get_slot_with_index ().second;

      return slot.get_slot_with_index ().second + delta
             < (int) structure::tracks::Channel::STRIP_SIZE;
    }
#endif
  // non-indexed slots not supported yet
  return false;
}
