// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/clap_plugin_format.h"
#include "plugins/vst3_plugin_format.h"
#include "utils/file_path_list.h"
#include "utils/qt.h"

namespace zrythm::test_helpers
{

/**
 * @brief Finds a test plugin descriptor by name.
 */
template <typename FormatType>
std::unique_ptr<plugins::PluginDescriptor>
find_test_plugin_by_name (
  const QString      &search_paths_str,
  const juce::String &target_name)
{
  if (search_paths_str.isEmpty ())
    return nullptr;

  auto        paths = std::make_unique<utils::FilePathList> ();
  QStringList path_list = search_paths_str.split (":::", Qt::SkipEmptyParts);
  for (const auto &path : path_list)
    paths->add_path (std::filesystem::path (utils::to_std_string (path)));

  if (paths->empty ())
    return nullptr;

  FormatType format;
  for (const auto &identifier : format.search_identifiers (*paths, false))
    {
      auto scan_result = format.scan_identifier (identifier);
      if (!scan_result.has_value ())
        continue;

      for (auto &descriptor : *scan_result)
        {
          if (descriptor->name_.to_juce_string () == target_name)
            return std::move (descriptor);
        }
    }
  return nullptr;
}

inline std::unique_ptr<plugins::PluginDescriptor>
find_test_vst3_plugin_by_name (const juce::String &target_name)
{
  return find_test_plugin_by_name<plugins::Vst3PluginFormat> (
    QStringLiteral (TEST_VST3_SEARCH_PATHS), target_name);
}

inline std::unique_ptr<plugins::PluginDescriptor>
find_test_clap_plugin_by_name (const juce::String &target_name)
{
  return find_test_plugin_by_name<plugins::CLAPPluginFormat> (
    QStringLiteral (TEST_CLAP_SEARCH_PATHS), target_name);
}

} // namespace zrythm::test_helpers
