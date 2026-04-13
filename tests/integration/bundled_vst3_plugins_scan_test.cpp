// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <string_view>

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
 * @brief Integration test for bundled VST3 plugin scanning.
 *
 * This test verifies that:
 * 1. All bundled VST3 plugins can be found in the build directory
 * 2. The plugin scanner can successfully scan each plugin
 * 3. Each plugin produces a valid PluginDescription
 * 4. Plugin metadata (vendor, category, version) is correctly set
 */
class BundledVST3PluginsScanTest
    : public ::testing::Test,
      private ScopedJuceQApplication
{
protected:
  // Expected manufacturer name for all bundled plugins
  static constexpr std::string_view EXPECTED_MANUFACTURER = "Zrythm";
  void                              SetUp () override
  {
    // Create format manager and add only VST3 format
    // (avoid picking up system plugins like AudioUnit on macOS)
    format_manager_ = std::make_shared<juce::AudioPluginFormatManager> ();
    format_manager_->addFormat (std::make_unique<juce::VST3PluginFormat> ());

    // Create known plugin list
    known_plugins_ = std::make_shared<juce::KnownPluginList> ();
  }

  void TearDown () override
  {
    known_plugins_.reset ();
    format_manager_.reset ();
  }

  /**
   * @brief Get the VST3 search paths from the compile definition.
   */
  auto get_vst3_search_paths ()
  {
    std::unique_ptr<utils::FilePathList> paths =
      std::make_unique<utils::FilePathList> ();

    // Parse the semicolon-separated paths from the compile definition
    QString paths_str = QStringLiteral (BUNDLED_VST3_SEARCH_PATHS);
    EXPECT_FALSE (paths_str.isEmpty ());
    QStringList path_list = paths_str.split (":::", Qt::SkipEmptyParts);

    for (const auto &path : path_list)
      {
        paths->add_path (std::filesystem::path (path.toStdString ()));
      }

    return paths;
  }

  std::shared_ptr<juce::AudioPluginFormatManager> format_manager_;
  std::shared_ptr<juce::KnownPluginList>          known_plugins_;
};

/**
 * @brief Test that all bundled VST3 plugins can be scanned.
 *
 * This test uses the actual VST3PluginFormat to scan the bundled plugins
 * built during the build process.
 */
TEST_F (BundledVST3PluginsScanTest, ScanAllBundledPlugins)
{
  // Get the VST3 search paths
  auto search_paths = get_vst3_search_paths ();
  ASSERT_FALSE (search_paths->empty ())
    << "No VST3 search paths configured - check BUNDLED_VST3_SEARCH_PATHS";

  // Find VST3 format
  juce::VST3PluginFormat * vst3_format = nullptr;
  for (auto * format : format_manager_->getFormats ())
    {
      if (format->getName () == "VST3")
        {
          vst3_format = dynamic_cast<juce::VST3PluginFormat *> (format);
          break;
        }
    }
  ASSERT_NE (vst3_format, nullptr) << "VST3 format not available";

  // Convert search paths to JUCE format
  auto juce_search_paths = search_paths->get_as_juce_file_search_path ();

  z_info ("VST3 search paths: {}", juce_search_paths.toString ().toStdString ());

  // Search for plugin files
  auto plugin_identifiers =
    vst3_format->searchPathsForPlugins (juce_search_paths, false, false);

  z_info ("Found {} VST3 plugin identifiers", plugin_identifiers.size ());

  // We expect to find exactly 14 bundled plugins
  EXPECT_EQ (plugin_identifiers.size (), BUNDLED_PLUGINS_COUNT)
    << "Expected to find " << BUNDLED_PLUGINS_COUNT
    << " bundled plugins, but found " << plugin_identifiers.size ();

  // Scan each plugin and verify it produces valid descriptions
  int successful_scans = 0;
  for (const auto &identifier : plugin_identifiers)
    {
      z_info ("Scanning plugin: {}", identifier.toStdString ());

      juce::OwnedArray<juce::PluginDescription> descriptions;
      vst3_format->findAllTypesForFile (descriptions, identifier);

      if (!descriptions.isEmpty ())
        {
          successful_scans++;
          for (const auto * desc : descriptions)
            {
              z_info (
                "  Found plugin: {} (category: {}, manufacturer: {})",
                desc->name.toStdString (), desc->category.toStdString (),
                desc->manufacturerName.toStdString ());

              // Verify basic plugin info
              EXPECT_FALSE (desc->name.isEmpty ()) << "Plugin name is empty";
              EXPECT_FALSE (desc->fileOrIdentifier.isEmpty ())
                << "Plugin identifier is empty";

              // Verify plugin metadata
              EXPECT_EQ (
                desc->manufacturerName.toStdString (),
                std::string (EXPECTED_MANUFACTURER))
                << "Plugin '" << desc->name.toStdString ()
                << "' has incorrect manufacturer";
              EXPECT_FALSE (desc->version.isEmpty ())
                << "Plugin '" << desc->name.toStdString ()
                << "' version is empty";
              EXPECT_FALSE (desc->category.isEmpty ())
                << "Plugin '" << desc->name.toStdString ()
                << "' category is empty";
            }
        }
      else
        {
          z_warning ("Failed to scan plugin: {}", identifier.toStdString ());
        }
    }

  // All plugins should scan successfully
  EXPECT_EQ (successful_scans, BUNDLED_PLUGINS_COUNT)
    << "Not all bundled plugins scanned successfully";
}

/**
 * @brief Test that the PluginScanManager can scan bundled VST3 plugins.
 *
 * This test uses the PluginScanManager class to verify the full scanning
 * workflow works correctly.
 */
TEST_F (BundledVST3PluginsScanTest, PluginScanManagerIntegration)
{
  auto paths_provider = [this] (Protocol::ProtocolType protocol)
    -> std::unique_ptr<utils::FilePathList> {
    auto paths = std::make_unique<utils::FilePathList> ();
    if (protocol == Protocol::ProtocolType::VST3)
      {
        // Copy paths from our search paths
        auto search_paths = get_vst3_search_paths ();
        for (const auto &path : search_paths->getPaths ())
          {
            paths->add_path (std::filesystem::path (path.toStdString ()));
          }
      }
    return paths;
  };

  PluginScanManager scanner (
    known_plugins_, format_manager_, paths_provider, nullptr);

  QSignalSpy finished_spy (&scanner, &PluginScanManager::scanningFinished);

  scanner.beginScan ();

  // Wait for scanning to complete (with timeout)
  bool finished = finished_spy.wait (30000); // 30 second timeout
  ASSERT_TRUE (finished) << "Plugin scanning did not complete within timeout";

  // Check that we found plugins
  auto types = known_plugins_->getTypes ();
  z_info ("Found {} plugin types", types.size ());

  EXPECT_EQ (types.size (), BUNDLED_PLUGINS_COUNT)
    << "Expected " << BUNDLED_PLUGINS_COUNT << " plugins, found "
    << types.size ();

  // Verify each plugin has required information
  for (const auto &desc : types)
    {
      EXPECT_FALSE (desc.name.isEmpty ()) << "Plugin name is empty";
      EXPECT_FALSE (desc.fileOrIdentifier.isEmpty ())
        << "Plugin identifier is empty";
      EXPECT_EQ (desc.pluginFormatName, juce::String ("VST3"))
        << "Plugin is not VST3 format";

      // Verify plugin metadata
      EXPECT_EQ (
        desc.manufacturerName.toStdString (),
        std::string (EXPECTED_MANUFACTURER))
        << "Plugin '" << desc.name.toStdString ()
        << "' has incorrect manufacturer";
      EXPECT_FALSE (desc.version.isEmpty ())
        << "Plugin '" << desc.name.toStdString () << "' version is empty";
      EXPECT_FALSE (desc.category.isEmpty ())
        << "Plugin '" << desc.name.toStdString () << "' category is empty";

      z_info (
        "Verified plugin: {} (manufacturer: {}, category: {})",
        desc.name.toStdString (), desc.manufacturerName.toStdString (),
        desc.category.toStdString ());
    }
}

/**
 * @brief Test that each bundled plugin can be instantiated.
 *
 * This test verifies that each plugin can be loaded as an AudioPluginInstance.
 * This is a more thorough test than just scanning.
 */
TEST_F (BundledVST3PluginsScanTest, InstantiateBundledPlugins)
{
  auto search_paths = get_vst3_search_paths ();
  ASSERT_FALSE (search_paths->empty ());

  // Find VST3 format
  juce::VST3PluginFormat * vst3_format = nullptr;
  for (auto * format : format_manager_->getFormats ())
    {
      if (format->getName () == "VST3")
        {
          vst3_format = dynamic_cast<juce::VST3PluginFormat *> (format);
          break;
        }
    }
  ASSERT_NE (vst3_format, nullptr);

  auto juce_search_paths = search_paths->get_as_juce_file_search_path ();
  auto plugin_identifiers =
    vst3_format->searchPathsForPlugins (juce_search_paths, false, false);

  int successful_instantiations = 0;

  for (const auto &identifier : plugin_identifiers)
    {
      // Get plugin description
      juce::OwnedArray<juce::PluginDescription> descriptions;
      vst3_format->findAllTypesForFile (descriptions, identifier);

      for (const auto * desc : descriptions)
        {
          z_info ("Attempting to instantiate: {}", desc->name.toStdString ());

          // Try to create an instance
          std::unique_ptr<juce::AudioPluginInstance> instance;
          juce::String                               error;

          // Create instance synchronously
          instance =
            format_manager_->createPluginInstance (*desc, 44100.0, 512, error);

          if (instance)
            {
              successful_instantiations++;
              z_info (
                "Successfully instantiated: {}", desc->name.toStdString ());

              // Basic sanity checks
              EXPECT_GE (instance->getTotalNumInputChannels (), 0u);
              EXPECT_GT (instance->getTotalNumOutputChannels (), 0u);
            }
          else
            {
              z_warning (
                "Failed to instantiate {}: {}", desc->name.toStdString (),
                error.toStdString ());
            }
        }
    }

  EXPECT_EQ (successful_instantiations, BUNDLED_PLUGINS_COUNT)
    << "Not all bundled plugins could be instantiated";
}

} // namespace zrythm::plugins
