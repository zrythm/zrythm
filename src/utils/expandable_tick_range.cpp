// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/expandable_tick_range.h"
#include "utils/format.h"

namespace zrythm::utils
{
auto
format_as (const ExpandableTickRange &range) -> std::string
{
  if (range.is_full_content ())
    {
      return "AffectedTickRange: (full content)";
    }
  const auto tick_range = range.range ().value ();
  return fmt::format ("AffectedTickRange: {}", tick_range);
}
} // namespace zrythm::utils
