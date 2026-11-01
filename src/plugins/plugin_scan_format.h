// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <expected>
#include <memory>
#include <vector>

#include "plugins/plugin_descriptor.h"
#include "utils/file_path_list.h"
#include "utils/utf8_string.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

namespace zrythm::plugins
{

class Lv2World;

/**
 * @brief A plugin format that can discover plugins and scan them into
 * complete Zrythm descriptors.
 */
class PluginScanFormat
{
public:
  virtual ~PluginScanFormat () = default;

  [[nodiscard]] virtual Protocol::ProtocolType protocol () const = 0;
  [[nodiscard]] virtual utils::Utf8String      format_name () const = 0;

  /** Returns candidate plugin identifiers below the given search paths. */
  [[nodiscard]] virtual std::vector<utils::Utf8String>
  search_identifiers (const utils::FilePathList &search_paths, bool recursive)
    const = 0;

  /**
   * @brief Scans one identifier.
   *
   * @return The plugins found in the identifier (possibly none), or an error
   * when the identifier belongs to this format but cannot be scanned.
   */
  [[nodiscard]] virtual std::
    expected<std::vector<std::unique_ptr<PluginDescriptor>>, utils::Utf8String>
    scan_identifier (const utils::Utf8String &identifier) = 0;

  /** Returns whether the plugin file changed since @p descriptor was scanned. */
  [[nodiscard]] virtual bool
  plugin_needs_rescanning (const PluginDescriptor &descriptor) const = 0;

  /** Returns whether the plugin file/identifier still exists. */
  [[nodiscard]] virtual bool
  plugin_exists (const PluginDescriptor &descriptor) const = 0;

  /**
   * Returns whether scanning this identifier must happen on the message
   * thread.
   */
  [[nodiscard]] virtual bool
  scan_uses_message_thread (const utils::Utf8String &identifier) const = 0;
};

/**
 * @brief Registry of the plugin formats available to the scanner.
 *
 * Native VST3 and CLAP formats are always registered, and LV2 is registered
 * when an LV2 world is available. The remaining JUCE-hosted formats are
 * wrapped by an adapter; their JUCE descriptions never leave the subprocess.
 */
class PluginScanFormatManager
{
public:
  explicit PluginScanFormatManager (std::shared_ptr<Lv2World> lv2_world);
  ~PluginScanFormatManager ();

  PluginScanFormatManager (const PluginScanFormatManager &) = delete;
  PluginScanFormatManager &operator= (const PluginScanFormatManager &) = delete;
  PluginScanFormatManager (PluginScanFormatManager &&) = delete;
  PluginScanFormatManager &operator= (PluginScanFormatManager &&) = delete;

  /** Returns all registered formats in registration order. */
  [[nodiscard]] std::vector<PluginScanFormat *> formats () const;

  /** Returns the format with @p format_name, or null. */
  [[nodiscard]] PluginScanFormat *
  find_format (const utils::Utf8String &format_name) const;

  /**
   * @brief Returns the JUCE format manager used for JucePlugin instantiation.
   *
   * Formats with a native scan implementation are skipped when scan formats
   * are registered from this manager.
   */
  [[nodiscard]] std::shared_ptr<juce::AudioPluginFormatManager>
  juce_format_manager () const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace zrythm::plugins
