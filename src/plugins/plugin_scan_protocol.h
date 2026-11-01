// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <expected>
#include <functional>
#include <memory>
#include <vector>

#include "plugins/plugin_descriptor.h"
#include "utils/utf8_string.h"

namespace zrythm::plugins::discovery
{

using PluginScanResult = std::
  expected<std::vector<std::unique_ptr<PluginDescriptor>>, utils::Utf8String>;

/**
 * @brief Blocking scanner interface for scanning one plugin identifier at a
 * time.
 *
 * A returned error fails the single scanned identifier. Environment failures
 * that affect every identifier (for example an unlaunchable scanner) surface
 * as exceptions and abort the whole scan.
 */
class PluginScanner
{
public:
  virtual ~PluginScanner () = default;

  /**
   * @brief Scans one plugin identifier.
   *
   * @param cancelled Polled while the scan is in progress; implementations
   * abort and report no plugins once it returns true.
   * @return The descriptors found (possibly none), or a scan error for the
   * identifier.
   */
  [[nodiscard]] virtual PluginScanResult scan (
    const utils::Utf8String      &format_name,
    const utils::Utf8String      &identifier,
    const std::function<bool ()> &cancelled) = 0;
};

/** Serializes a scan response for transfer from the scanner subprocess. */
[[nodiscard]] utils::Utf8String
serialize_plugin_scan_response (const PluginScanResult &result);

/**
 * @brief Parses a scan response sent by the scanner subprocess.
 *
 * @return The response's descriptors or scan error, or a transport error when
 * the payload is not a valid scan-response document.
 */
[[nodiscard]] PluginScanResult
deserialize_plugin_scan_response (std::string_view payload);

} // namespace zrythm::plugins::discovery
