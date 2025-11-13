// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <ranges>

namespace zrythm::ranges
{

/**
 * @brief Returns whether all elements in the range are equal, optionally using
 * a projection.
 */
template <std::ranges::input_range R, typename Proj = std::identity>
bool
all_equal (R &&r, Proj proj = {})
{
  auto begin = std::ranges::begin (r);
  auto end = std::ranges::end (r);

  // Empty or single-element ranges are considered all equal
  if (begin == end)
    return true;

  // Returns end() if no non-equal adjacent elements exist
  return std::ranges::adjacent_find (r, std::ranges::not_equal_to{}, proj) == end;
}
}
