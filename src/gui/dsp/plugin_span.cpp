// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/project.h"

#include "./plugin_span.h"

using namespace zrythm;

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
void
PluginSpanImpl<Range>::paste_to_slot (Plugin::Channel &ch, dsp::PluginSlot slot)
  const
{
  try
    {
      UNDO_MANAGER->perform (new gui::actions::MixerSelectionsPasteAction (
        *this, *PORT_CONNECTIONS_MGR, &ch.get_track (), slot));
    }
  catch (const ZrythmException &e)
    {
      e.handle (QObject::tr ("Failed to paste plugins"));
    }
}

template <utils::UuidIdentifiableObjectPtrVariantRange Range>
bool
PluginSpanImpl<Range>::can_be_pasted (const dsp::PluginSlot &slot) const
{
  if (std::ranges::distance (*this) == 0)
    return false;

  auto slots = std::ranges::to<std::vector> (
    std::views::transform (*this, slot_projection));
  auto [highest_slot_it, lowest_slot_it] = std::ranges::minmax_element (slots);

  if ((*lowest_slot_it).has_slot_index () && slot.has_slot_index ())
    {
      auto delta =
        (*highest_slot_it).get_slot_with_index ().second
        - (*lowest_slot_it).get_slot_with_index ().second;

      return slot.get_slot_with_index ().second + delta < (int) dsp::STRIP_SIZE;
    }
  // non-indexed slots not supported yet
  return false;
}

template class PluginSpanImpl<std::span<const PluginSpan::VariantType>>;
template class PluginSpanImpl<
  utils::UuidIdentifiableObjectSpan<PluginSpan::PluginRegistry>>;
