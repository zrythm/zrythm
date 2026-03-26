// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstddef>
#include <ranges>
#include <utility>

namespace zrythm::utils::views
{

// Range adaptor closure for enumerate - uses zip with iota
struct EnumerateAdaptor : std::ranges::range_adaptor_closure<EnumerateAdaptor>
{
  template <std::ranges::viewable_range Range> auto operator() (Range &&r) const
  {
    return std::views::zip (
      std::views::iota (std::size_t{ 0 }), std::forward<Range> (r));
  }
};

// The enumerate function is a range adaptor closure
inline constexpr EnumerateAdaptor enumerate{};

} // namespace zrythm::utils::views
