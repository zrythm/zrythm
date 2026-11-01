// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include "plugins/plugin_scan_format.h"

namespace zrythm::plugins
{

class Lv2World;

/**
 * @brief Native LV2 plugin discovery via lilv.
 *
 * Produces complete Zrythm descriptors directly. Discovery is metadata-only:
 * bundle ttl is parsed, but no plugin binary is ever loaded. Hosting is done
 * by Lv2Plugin.
 */
class Lv2PluginFormat final : public PluginScanFormat
{
public:
  /**
   * @param world The world used for bundle parsing, kept alive by the
   * format. Callers share one world per process.
   */
  explicit Lv2PluginFormat (std::shared_ptr<Lv2World> world);
  ~Lv2PluginFormat () override;

  /**
   * @brief Returns the directory the LV2 specification bundles are shipped
   * in.
   */
  static std::filesystem::path get_spec_bundles_dir ();

  [[nodiscard]] Protocol::ProtocolType protocol () const override;
  [[nodiscard]] utils::Utf8String      format_name () const override;

  [[nodiscard]] std::vector<utils::Utf8String>
  search_identifiers (const utils::FilePathList &search_paths, bool recursive)
    const override;

  [[nodiscard]] std::
    expected<std::vector<std::unique_ptr<PluginDescriptor>>, utils::Utf8String>
    scan_identifier (const utils::Utf8String &identifier) override;

  [[nodiscard]] bool
  plugin_needs_rescanning (const PluginDescriptor &descriptor) const override;
  [[nodiscard]] bool
  plugin_exists (const PluginDescriptor &descriptor) const override;
  [[nodiscard]] bool
  scan_uses_message_thread (const utils::Utf8String &identifier) const override;

private:
  [[nodiscard]] bool file_might_contain_this_plugin_type (
    const utils::Utf8String &identifier) const;

  std::shared_ptr<Lv2World> world_;
};

} // namespace zrythm::plugins
