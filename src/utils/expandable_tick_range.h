// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>

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
        assert (range->second >= range->first);
        start_ = range->first;
        end_ = range->second;
        is_full_content_ = false;
      }
  }

  /**
   * @brief Returns whether the range is the full content (ie, there is no
   * range).
   */
  bool is_full_content () const { return is_full_content_; }

  /**
   * @brief Expands to the given range.
   */
  void expand (std::pair<double, double> range_to_add)
  {
    // full content takes precedence
    if (is_full_content ())
      return;

    assert (range_to_add.second >= range_to_add.first);
    start_ = std::min (start_, range_to_add.first);
    end_ = std::max (end_, range_to_add.second);
  }

  void expand_to_full ()
  {
    is_full_content_ = true;
    start_ = 0;
    end_ = 0;
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
  double end_{};
  bool   is_full_content_{ true };
};

auto
format_as (const ExpandableTickRange &range) -> std::string;
}

Q_DECLARE_METATYPE (zrythm::utils::ExpandableTickRange)
