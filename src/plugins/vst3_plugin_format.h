// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin_scan_format.h"

namespace zrythm::plugins
{

/**
 * @brief Native VST3 plugin discovery via the VST3 SDK.
 *
 * Produces complete Zrythm descriptors directly; plugin instantiation for
 * hosting is done by Vst3Plugin.
 */
class Vst3PluginFormat final : public PluginScanFormat
{
public:
  Vst3PluginFormat () = default;
  ~Vst3PluginFormat () override;

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

  void recursive_file_search (
    std::vector<utils::Utf8String> &results,
    const std::filesystem::path    &directory,
    bool                            recursive) const;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Vst3PluginFormat)
};

} // namespace zrythm::plugins
