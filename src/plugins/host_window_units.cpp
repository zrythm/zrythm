// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cassert>
#include <cmath>

#include "plugins/host_window_units.h"

#include <QtGlobal>

namespace zrythm::plugins
{

std::pair<int, int>
plugin_view_size_to_host_window_size (int width, int height, float scale_factor)
{
  assert (width > 0 && height > 0);
  assert (scale_factor > 0.F);
#if defined(Q_OS_MACOS)
  return { width, height };
#else
  return {
    static_cast<int> (std::lround (width / scale_factor)),
    static_cast<int> (std::lround (height / scale_factor))
  };
#endif
}

int
host_window_logical_to_physical (int logical, float scale_factor)
{
  assert (logical > 0);
  assert (scale_factor > 0.F);
#if defined(Q_OS_MACOS)
  return logical;
#else
  return std::max (
    1,
    static_cast<int> (std::lround (static_cast<float> (logical) * scale_factor)));
#endif
}

} // namespace zrythm::plugins
