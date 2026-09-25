// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "plugins/plugin.h"

#include <QString>

#include <clap/clap.h>

namespace zrythm::plugins::clap_preset_discovery
{

/**
 * @brief Location of a preset, as defined by the preset-discovery
 * provider that reported it.
 */
struct PresetLocation
{
  /** @ref clap_preset_discovery_location_kind (FILE or PLUGIN). */
  uint32_t kind = CLAP_PRESET_DISCOVERY_LOCATION_FILE;

  /** Filesystem path; empty for PLUGIN kind. */
  std::string location;

  /** Provider-defined key addressing a preset inside a container; empty
   * for whole-file presets. */
  std::string load_key;
};

/** Encodes a location into the opaque string used as a PresetEntry id. */
QString
encode_preset_id (const PresetLocation &location);

/**
 * @brief Decodes a PresetEntry id produced by encode_preset_id().
 *
 * @return std::nullopt when the string is not a valid encoded id.
 */
std::optional<PresetLocation>
decode_preset_id (const QString &id);

/**
 * @brief Collects the presets a plugin library exposes through the CLAP
 * preset-discovery factory.
 *
 * Runs the library's preset-discovery providers: each provider declares
 * its filetypes and locations during init(), FILE-kind locations are
 * crawled (matching the declared file extensions) and their metadata
 * read, and a single metadata read covers PLUGIN-kind (embedded)
 * presets. Presets that declare compatible plugin ids are filtered
 * against @p plugin_id (the hosted plugin's CLAP id); presets declaring
 * no plugin id are kept. Entries are sorted by name, using the declared
 * location name as their group, and capped at a fixed total.
 *
 * Main thread only. @p entry must belong to a loaded library that
 * outlives the call. Returns an empty list when the library has no
 * preset-discovery factory.
 */
std::vector<Plugin::PresetEntry>
collect_presets (const clap_plugin_entry &entry, std::string_view plugin_id);

/**
 * @brief collect_presets with a per-process cache keyed by the library
 * path and plugin id.
 *
 * Crawling a library's preset locations can visit thousands of files,
 * and every hosted instance of the same plugin would otherwise re-crawl
 * them; the cached result is reused for the lifetime of the process
 * (the same trade the LV2 world's resource cache makes). Main thread
 * only.
 */
std::vector<Plugin::PresetEntry>
collect_presets_cached (
  const std::filesystem::path &library_path,
  const clap_plugin_entry     &entry,
  std::string_view             plugin_id);

/** Discards all cached crawl results; the next
 * collect_presets_cached() call re-crawls. */
void
clear_preset_cache ();

} // namespace zrythm::plugins::clap_preset_discovery
