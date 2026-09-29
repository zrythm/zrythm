// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

// Helpers shared by the plugin format implementations (CLAP, VST3, LV2).

#pragma once

#include <algorithm>
#include <cstdint>

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

namespace zrythm::plugins
{

/**
 * @brief Stable hash of a plugin identifier (LV2 URI, CLAP ID, VST3 TUID).
 *
 * Used for PluginDescription::uniqueId.
 */
inline auto
get_hash_for_range (auto &&range) -> int
{
  return static_cast<int> (std::ranges::fold_left (
    range, uint32_t{ 0 }, [] (uint32_t acc, auto &&item) {
      return (acc * 31) + static_cast<uint32_t> (item);
    }));
}

/**
 * @brief Modification time used for stale-scan detection.
 *
 * A plugin bundle (.lv2, .vst3) is a directory whose own mtime doesn't
 * change when files inside are replaced, so bundles use the newest mtime
 * of their contents.
 */
juce::Time
effective_modification_time (const juce::File &plugin_file);

} // namespace zrythm::plugins
