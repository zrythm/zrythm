// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/traits.h"

#include <fmt/format.h>

/**
 * @brief Used when fmt::format can't be used (when the format string is not a
 * literal)
 */
template <typename... Args>
std::string
format_str (std::string_view format, Args &&... args)
{
  return fmt::vformat (format, fmt::make_format_args (args...));
}

// Generic formatter for smart pointers (std::shared_ptr and std::unique_ptr)
template <SmartPtr Ptr>
struct fmt::formatter<Ptr> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const Ptr &ptr, FormatContext &ctx) const
  {
    if (ptr)
      {
        return fmt::formatter<std::string_view>::format (
          fmt::format ("{}", *ptr), ctx);
      }

    return fmt::formatter<std::string_view>::format ("(null)", ctx);
  }
};

// Formatter for std::pair
template <fmt::formattable T, fmt::formattable U>
struct fmt::formatter<std::pair<T, U>> : fmt::formatter<std::string_view>
{
  auto format (const std::pair<T, U> &pair, format_context &ctx) const
    -> format_context::iterator
  {
    return fmt::formatter<std::string_view>::format (
      fmt::format ("({}, {})", pair.first, pair.second), ctx);
  }
};
static_assert (fmt::formattable<std::pair<int, std::string>>);
