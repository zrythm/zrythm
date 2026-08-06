// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

namespace zrythm::plugins
{

/**
 * @brief Converts a plugin view size to a host window size.
 *
 * Plugin view sizes are physical pixels on Windows/GNU/Linux and logical
 * units on macOS (CLAP clap/ext/gui.h, VST3 iplugview.h); host window
 * sizes are logical pixels per the PluginHostWindow contract.
 *
 * @pre Positive dimensions and scale factor.
 */
std::pair<int, int>
plugin_view_size_to_host_window_size (int width, int height, float scale_factor);

/**
 * @brief Converts a logical host window size to plugin view units.
 *
 * Inverse of plugin_view_size_to_host_window_size(): a passthrough on
 * macOS, where plugin view sizes are logical.
 *
 * @pre Positive dimension and scale factor.
 */
int
host_window_logical_to_physical (int logical, float scale_factor);

} // namespace zrythm::plugins
