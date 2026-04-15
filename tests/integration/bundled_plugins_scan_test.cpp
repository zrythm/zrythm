// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <string_view>

#include "plugins/CLAPPluginFormat.h"
#include "plugins/plugin_scan_manager.h"
#include "utils/file_path_list.h"

#include <QSignalSpy>
#include <QStringList>

#include "helpers/scoped_juce_qapplication.h"

#include <gtest/gtest.h>

using namespace zrythm::test_helpers;

namespace zrythm::plugins
{

/**
 * @brief Integration test for bundled plugin scanning (VST3 and CLAP).
 *
 * This test verifies that:
 * 1. All bundled plugins can be found in the build directory
 * 2. The plugin scanner can successfully scan each plugin
 * 3. Each plugin produces a valid PluginDescription
 * 4. Plugin metadata (vendor, category, version) is correctly set
 */
class BundledPluginsScanTest : public ::testing::Test, private ScopedJuceQApplication
{
protected:
  // Expected manufacturer name for all bundled plugins
  static constexpr std::string_view EXPECTED_MANUFACTURER = "Zrythm";
  void                              SetUp () override
  {
    format_manager_ = std::make_shared<juce::AudioPluginFormatManager> ();
    format_manager_->addFormat (std::make_unique<juce::VST3PluginFormat> ());
    format_manager_->addFormat (std::make_unique<CLAPPluginFormat> ());

    known_plugins_ = std::make_shared<juce::KnownPluginList> ();
  }

  void TearDown () override
  {
    known_plugins_.reset ();
    format_manager_.reset ();
  }

  /**
   * @brief Parses a compile-definition search path string (:::-separated) into
   * a FilePathList.
   */
  static std::unique_ptr<utils::FilePathList>
  parse_search_paths (const QString &paths_str)
  {
    auto        paths = std::make_unique<utils::FilePathList> ();
    QStringList path_list = paths_str.split (":::", Qt::SkipEmptyParts);
    for (const auto &path : path_list)
      {
        paths->add_path (std::filesystem::path (path.toStdString ()));
      }
    return paths;
  }

  auto get_vst3_search_paths ()
  {
    return parse_search_paths (QStringLiteral (BUNDLED_VST3_SEARCH_PATHS));
  }

  auto get_clap_search_paths ()
  {
    return parse_search_paths (QStringLiteral (BUNDLED_CLAP_SEARCH_PATHS));
  }

  /**
   * @brief Finds a format by name in the format manager.
   */
  juce::AudioPluginFormat * find_format (const juce::String &format_name) const
  {
    for (auto * format : format_manager_->getFormats ())
      {
        if (format->getName () == format_name)
          return format;
      }
    return nullptr;
  }

  std::shared_ptr<juce::AudioPluginFormatManager> format_manager_;
  std::shared_ptr<juce::KnownPluginList>          known_plugins_;
};

// ============================================================================
// VST3 tests
// ============================================================================

TEST_F (BundledPluginsScanTest, ScanAllBundledVst3Plugins)
{
  auto search_paths = get_vst3_search_paths ();
  ASSERT_FALSE (search_paths->empty ())
    << "No VST3 search paths configured - check BUNDLED_VST3_SEARCH_PATHS";

  auto * vst3_format = find_format ("VST3");
  ASSERT_NE (vst3_format, nullptr) << "VST3 format not available";

  auto juce_search_paths = search_paths->get_as_juce_file_search_path ();
  auto plugin_identifiers =
    vst3_format->searchPathsForPlugins (juce_search_paths, false, false);

  EXPECT_EQ (plugin_identifiers.size (), BUNDLED_PLUGINS_COUNT)
    << "Expected " << BUNDLED_PLUGINS_COUNT
    << " bundled VST3 plugins, but found " << plugin_identifiers.size ();

  int successful_scans = 0;
  for (const auto &identifier : plugin_identifiers)
    {
      juce::OwnedArray<juce::PluginDescription> descriptions;
      vst3_format->findAllTypesForFile (descriptions, identifier);

      if (!descriptions.isEmpty ())
        {
          successful_scans++;
          for (const auto * desc : descriptions)
            {
              EXPECT_FALSE (desc->name.isEmpty ());
              EXPECT_EQ (
                desc->manufacturerName.toStdString (),
                std::string (EXPECTED_MANUFACTURER));
              EXPECT_FALSE (desc->version.isEmpty ());
              EXPECT_FALSE (desc->category.isEmpty ());
            }
        }
    }

  EXPECT_EQ (successful_scans, BUNDLED_PLUGINS_COUNT);
}

TEST_F (BundledPluginsScanTest, InstantiateBundledVst3Plugins)
{
  auto search_paths = get_vst3_search_paths ();
  ASSERT_FALSE (search_paths->empty ());

  auto * vst3_format = find_format ("VST3");
  ASSERT_NE (vst3_format, nullptr);

  auto juce_search_paths = search_paths->get_as_juce_file_search_path ();
  auto plugin_identifiers =
    vst3_format->searchPathsForPlugins (juce_search_paths, false, false);

  int successful_instantiations = 0;
  for (const auto &identifier : plugin_identifiers)
    {
      juce::OwnedArray<juce::PluginDescription> descriptions;
      vst3_format->findAllTypesForFile (descriptions, identifier);

      for (const auto * desc : descriptions)
        {
          juce::String error;
          auto         instance =
            format_manager_->createPluginInstance (*desc, 44100.0, 512, error);

          if (instance)
            {
              successful_instantiations++;
              EXPECT_GE (instance->getTotalNumInputChannels (), 0u);
              EXPECT_GT (instance->getTotalNumOutputChannels (), 0u);
            }
        }
    }

  EXPECT_EQ (successful_instantiations, BUNDLED_PLUGINS_COUNT)
    << "Not all bundled VST3 plugins could be instantiated";
}

// ============================================================================
// CLAP tests
// ============================================================================

TEST_F (BundledPluginsScanTest, ScanAllBundledClapPlugins)
{
  auto search_paths = get_clap_search_paths ();
  ASSERT_FALSE (search_paths->empty ())
    << "No CLAP search paths configured - check BUNDLED_CLAP_SEARCH_PATHS";

  auto * clap_format = find_format ("CLAP");
  ASSERT_NE (clap_format, nullptr) << "CLAP format not available";

  auto juce_search_paths = search_paths->get_as_juce_file_search_path ();
  auto plugin_identifiers =
    clap_format->searchPathsForPlugins (juce_search_paths, false, false);

  EXPECT_EQ (plugin_identifiers.size (), BUNDLED_PLUGINS_COUNT)
    << "Expected " << BUNDLED_PLUGINS_COUNT
    << " bundled CLAP plugins, but found " << plugin_identifiers.size ();

  int successful_scans = 0;
  for (const auto &identifier : plugin_identifiers)
    {
      juce::OwnedArray<juce::PluginDescription> descriptions;
      clap_format->findAllTypesForFile (descriptions, identifier);

      if (!descriptions.isEmpty ())
        {
          successful_scans++;
          for (const auto * desc : descriptions)
            {
              z_info (
                "  Found CLAP plugin: {} (id: {})", desc->name.toStdString (),
                desc->fileOrIdentifier.toStdString ());

              EXPECT_FALSE (desc->name.isEmpty ());
              EXPECT_EQ (
                desc->manufacturerName.toStdString (),
                std::string (EXPECTED_MANUFACTURER));
              EXPECT_FALSE (desc->version.isEmpty ());
            }
        }
      else
        {
          z_warning (
            "Failed to scan CLAP plugin: {}", identifier.toStdString ());
        }
    }

  EXPECT_EQ (successful_scans, BUNDLED_PLUGINS_COUNT);
}

} // namespace zrythm::plugins
