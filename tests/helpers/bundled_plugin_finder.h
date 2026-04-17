// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/CLAPPluginFormat.h"
#include "utils/file_path_list.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace zrythm::test_helpers
{

/**
 * @brief Finds a bundled plugin description by name, using a JUCE format to
 * scan the given search paths.
 */
template <typename FormatType>
std::unique_ptr<juce::PluginDescription>
find_bundled_plugin_by_name (
  const QString      &search_paths_str,
  const juce::String &target_name)
{
  if (search_paths_str.isEmpty ())
    return nullptr;

  auto        paths = std::make_unique<utils::FilePathList> ();
  QStringList path_list = search_paths_str.split (":::", Qt::SkipEmptyParts);
  for (const auto &path : path_list)
    paths->add_path (std::filesystem::path (path.toStdString ()));

  if (paths->empty ())
    return nullptr;

  FormatType format;
  auto       juce_search_paths = paths->get_as_juce_file_search_path ();
  auto       identifiers =
    format.searchPathsForPlugins (juce_search_paths, false, false);
  for (const auto &id : identifiers)
    {
      juce::OwnedArray<juce::PluginDescription> descriptions;
      format.findAllTypesForFile (descriptions, id);
      for (const auto * desc : descriptions)
        {
          if (desc->name == target_name)
            return std::make_unique<juce::PluginDescription> (*desc);
        }
    }
  return nullptr;
}

inline std::unique_ptr<juce::PluginDescription>
find_bundled_vst3_plugin_by_name (const juce::String &target_name)
{
  return find_bundled_plugin_by_name<juce::VST3PluginFormat> (
    QStringLiteral (BUNDLED_VST3_SEARCH_PATHS), target_name);
}

inline std::unique_ptr<juce::PluginDescription>
find_bundled_clap_plugin_by_name (const juce::String &target_name)
{
  return find_bundled_plugin_by_name<plugins::CLAPPluginFormat> (
    QStringLiteral (BUNDLED_CLAP_SEARCH_PATHS), target_name);
}

} // namespace zrythm::test_helpers
