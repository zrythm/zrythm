// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <fmt/format.h>
#include <juce_core/juce_core.h>

// Formatter for juce::String
template <>
struct fmt::formatter<juce::String> : fmt::formatter<std::string_view>
{
  auto format (const juce::String &s, fmt::format_context &ctx) const
    -> format_context::iterator
  {
    return fmt::formatter<std::string_view>::format (
      std::string_view (s.toUTF8 ()), ctx);
  }
};
static_assert (fmt::formattable<juce::String>);

// Formatter for juce::File
namespace juce
{
inline auto
format_as (const File &file)
{
  return file.getFullPathName ();
}
}
static_assert (fmt::formattable<juce::File>);
