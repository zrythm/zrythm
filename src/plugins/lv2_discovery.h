// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>
#include <vector>

#include "utils/utf8_string.h"

namespace zrythm::plugins
{

class Lv2World;

/**
 * @brief Metadata of a single LV2 plugin, extracted from a bundle's ttl.
 *
 * Extraction is metadata-only: no plugin (or UI) binary is ever loaded.
 */
struct Lv2PluginInfo
{
  /** Plugin URI (canonical identifier). */
  utils::Utf8String uri_;

  utils::Utf8String name_;
  utils::Utf8String author_;

  /** "minor.micro" from the lv2:minorVersion/lv2:microVersion triples. */
  utils::Utf8String version_;

  /** Tail of the declared plugin class URI, e.g. "InstrumentPlugin". */
  utils::Utf8String category_str_;

  bool is_instrument_ = false;

  int num_audio_ins_ = 0;
  int num_audio_outs_ = 0;
  int num_midi_ins_ = 0;
  int num_midi_outs_ = 0;
  int num_ctrl_ins_ = 0;
  int num_ctrl_outs_ = 0;
  int num_cv_ins_ = 0;
  int num_cv_outs_ = 0;

  bool has_custom_ui_ = false;
};

/**
 * @brief Loads the bundle at @p bundle_dir into @p world and returns the
 * extracted metadata of all plugins it contains.
 *
 * Bundles are additive in lilv: previously loaded bundles stay in the
 * world, so only plugins whose bundle URI matches @p bundle_dir are
 * returned.
 *
 * @return Extracted plugin metadata; empty if the bundle contains no
 * plugins or cannot be parsed.
 *
 * @note Main thread only.
 */
std::vector<Lv2PluginInfo>
get_plugins_in_bundle (Lv2World &world, const std::filesystem::path &bundle_dir);

} // namespace zrythm::plugins
