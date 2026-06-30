// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/time_warp_map.h"

#include <au/math.hh>

namespace zrythm::dsp
{

bool
TimeWarpMap::is_valid () const noexcept
{
  if (source_length <= units::samples (0) || output_length <= units::samples (0))
    return false;
  if (anchors.size () < 2)
    return false;

  if (
    anchors.front ().source_frame != units::samples (0)
    || anchors.front ().output_frame != units::samples (0))
    return false;
  if (
    anchors.back ().source_frame != source_length
    || anchors.back ().output_frame != output_length)
    return false;

  // Sorted ascending by source_frame, strictly increasing in output_frame.
  for (size_t i = 1; i < anchors.size (); ++i)
    {
      if (anchors[i].source_frame <= anchors[i - 1].source_frame)
        return false;
      if (anchors[i].output_frame <= anchors[i - 1].output_frame)
        return false;
    }
  return true;
}

bool
is_sample_space_identity (
  std::span<const WarpAnchor> anchors,
  units::sample_t             tolerance)
{
  return std::ranges::all_of (anchors, [tolerance] (const WarpAnchor &a) {
    return abs (a.source_frame - a.output_frame) <= tolerance;
  });
}

} // namespace zrythm::dsp
