// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>

#include <fmt/format.h>

namespace zrythm::utils
{
class ExpandableTickRange
{
public:
  ExpandableTickRange (
    std::optional<std::pair<double, double>> range = std::nullopt)
  {
    if (range.has_value ())
      {
        assert (range->first >= 0);
        assert (range->second >= range->first);
        start_ = range->first;
        end_ = range->second;
      }
  }

  /**
   * @brief Returns whether the range is the full content (ie, there is no
   * range).
   */
  bool is_full_content () const { return end_ < 0.0; }

  /**
   * @brief Expands to the given range.
   */
  void expand (std::pair<double, double> range_to_add)
  {
    // full content takes precedence
    if (is_full_content ())
      return;

    assert (range_to_add.first >= 0);
    assert (range_to_add.second >= range_to_add.first);
    start_ = std::min (start_, range_to_add.first);
    end_ = std::max (end_, range_to_add.second);
  }

  void expand_to_full ()
  {
    start_ = 0;
    end_ = -1;
  }

  void expand (const ExpandableTickRange &range_to_add)
  {
    if (range_to_add.is_full_content ())
      {
        expand_to_full ();
      }
    else
      {
        expand (range_to_add.range ().value ());
      }
  }

  /**
   * @brief Returns the range, or nullopt if the full content is covered.
   */
  auto range () const -> std::optional<std::pair<double, double>>
  {
    if (is_full_content ())
      {
        return std::nullopt;
      }

    return std::make_pair (start_, end_);
  }

private:
  double start_{};
  double end_{ -1 };
};

inline auto
format_as (const ExpandableTickRange &range) -> std::string
{
  if (range.is_full_content ())
    {
      return "AffectedTickRange: (full content)";
    }
  const auto tick_range = range.range ().value ();
  return fmt::format ("AffectedTickRange: {}", tick_range);
}
}

Q_DECLARE_METATYPE (zrythm::utils::ExpandableTickRange)
