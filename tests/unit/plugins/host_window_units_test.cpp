// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/host_window_units.h"

#include <gtest/gtest.h>

namespace zrythm::plugins
{

TEST (HostWindowUnitsTest, ViewSizeToWindowSize)
{
#if defined(Q_OS_MACOS)
  // Passthrough: plugin view sizes are logical on macOS
  EXPECT_EQ (
    plugin_view_size_to_host_window_size (640, 480, 2.f),
    (std::pair{ 640, 480 }));
#else
  EXPECT_EQ (
    plugin_view_size_to_host_window_size (640, 480, 1.f),
    (std::pair{ 640, 480 }));
  EXPECT_EQ (
    plugin_view_size_to_host_window_size (640, 480, 2.f),
    (std::pair{ 320, 240 }));
  EXPECT_EQ (
    plugin_view_size_to_host_window_size (400, 300, 1.25f),
    (std::pair{ 320, 240 }));
#endif
}

TEST (HostWindowUnitsTest, LogicalToPhysical)
{
#if defined(Q_OS_MACOS)
  // Passthrough: plugin view sizes are logical on macOS
  EXPECT_EQ (host_window_logical_to_physical (320, 1.f), 320);
  EXPECT_EQ (host_window_logical_to_physical (320, 2.f), 320);
#else
  EXPECT_EQ (host_window_logical_to_physical (320, 1.f), 320);
  EXPECT_EQ (host_window_logical_to_physical (320, 2.f), 640);
  EXPECT_EQ (host_window_logical_to_physical (321, 1.25f), 401);
#endif
}

} // namespace zrythm::plugins
