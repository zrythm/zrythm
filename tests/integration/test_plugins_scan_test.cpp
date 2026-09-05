// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <filesystem>
#include <string_view>

#include "plugins/CLAPPluginFormat.h"
#include "plugins/lv2_plugin_format.h"
#include "plugins/lv2_world.h"
#include "plugins/out_of_process_scanner.h"
#include "plugins/plugin_format_utils.h"
#include "plugins/plugin_scan_manager.h"
#include "plugins/vst3_plugin_format.h"
#include "utils/file_path_list.h"
#include "utils/io_utils.h"
#include "utils/logger.h"
#include "utils/qt.h"
#include "utils/utf8_string.h"

#include <QFile>
#include <QSignalSpy>
#include <QStringList>

#include "helpers/scoped_juce_qapplication.h"

#include <gtest/gtest.h>

using namespace zrythm::test_helpers;

namespace zrythm::plugins
{

/**
 * @brief Integration test for test plugin scanning (VST3 and CLAP).
 *
 * This test verifies that:
 * 1. All test plugins can be found in the build directory
 * 2. The plugin scanner can successfully scan each plugin
 * 3. Each plugin produces a valid PluginDescription
 * 4. Plugin metadata (vendor, category, version) is correctly set
 */
class TestPluginsScanTest : public ::testing::Test, private ScopedJuceQApplication
{
protected:
  // Expected manufacturer name for all test plugins
  static constexpr std::string_view EXPECTED_MANUFACTURER = "Zrythm";
  void                              SetUp () override
  {
    format_manager_ = std::make_shared<juce::AudioPluginFormatManager> ();
    format_manager_->addFormat (std::make_unique<Vst3PluginFormat> ());
    format_manager_->addFormat (std::make_unique<CLAPPluginFormat> ());
    format_manager_->addFormat (
      std::make_unique<Lv2PluginFormat> (
        std::make_shared<Lv2World> (Lv2PluginFormat::get_spec_bundles_dir ())));

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
        paths->add_path (std::filesystem::path (utils::to_std_string (path)));
      }
    return paths;
  }

  auto get_vst3_search_paths ()
  {
    return parse_search_paths (QStringLiteral (TEST_VST3_SEARCH_PATHS));
  }

  auto get_clap_search_paths ()
  {
    return parse_search_paths (QStringLiteral (TEST_CLAP_SEARCH_PATHS));
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

TEST_F (TestPluginsScanTest, ScanAllTestVst3Plugins)
{
  auto search_paths = get_vst3_search_paths ();
  ASSERT_FALSE (search_paths->empty ())
    << "No VST3 search paths configured - check TEST_VST3_SEARCH_PATHS";

  auto * vst3_format = find_format ("VST3");
  ASSERT_NE (vst3_format, nullptr) << "VST3 format not available";

  auto juce_search_paths = search_paths->get_as_juce_file_search_path ();
  auto plugin_identifiers =
    vst3_format->searchPathsForPlugins (juce_search_paths, false, false);

  EXPECT_EQ (plugin_identifiers.size (), TEST_VST3_PLUGINS_COUNT)
    << "Expected " << TEST_VST3_PLUGINS_COUNT
    << " test VST3 plugins, but found " << plugin_identifiers.size ();

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

  EXPECT_EQ (successful_scans, TEST_VST3_PLUGINS_COUNT);
}

// ============================================================================
// CLAP tests
// ============================================================================

TEST_F (TestPluginsScanTest, ScanAllTestClapPlugins)
{
  auto search_paths = get_clap_search_paths ();
  ASSERT_FALSE (search_paths->empty ())
    << "No CLAP search paths configured - check TEST_CLAP_SEARCH_PATHS";

  auto * clap_format = find_format ("CLAP");
  ASSERT_NE (clap_format, nullptr) << "CLAP format not available";

  auto juce_search_paths = search_paths->get_as_juce_file_search_path ();
  auto plugin_identifiers =
    clap_format->searchPathsForPlugins (juce_search_paths, false, false);

  int successful_scans = 0;
  int total_plugins_found = 0;
  for (const auto &identifier : plugin_identifiers)
    {
      juce::OwnedArray<juce::PluginDescription> descriptions;
      clap_format->findAllTypesForFile (descriptions, identifier);

      if (!descriptions.isEmpty ())
        {
          successful_scans++;
          total_plugins_found += descriptions.size ();
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

  // every file must scan, and the plugins inside them (including multiple
  // plugins per file) must all be enumerated
  EXPECT_EQ (successful_scans, plugin_identifiers.size ());
  EXPECT_EQ (total_plugins_found, TEST_CLAP_PLUGINS_COUNT)
    << "Expected " << TEST_CLAP_PLUGINS_COUNT
    << " test CLAP plugins, but found " << total_plugins_found;
}

// ============================================================================
// LV2 tests
// ============================================================================

TEST_F (TestPluginsScanTest, ScanAllTestLv2Plugins)
{
  auto search_paths =
    parse_search_paths (QStringLiteral (TEST_LV2_SEARCH_PATHS));
  ASSERT_FALSE (search_paths->empty ())
    << "No LV2 search paths configured - check TEST_LV2_SEARCH_PATHS";

  auto * lv2_format = find_format ("LV2");
  ASSERT_NE (lv2_format, nullptr) << "LV2 format not available";

  auto juce_search_paths = search_paths->get_as_juce_file_search_path ();
  auto plugin_identifiers =
    lv2_format->searchPathsForPlugins (juce_search_paths, false, false);

  EXPECT_EQ (plugin_identifiers.size (), TEST_LV2_BUNDLES_COUNT)
    << "Expected " << TEST_LV2_BUNDLES_COUNT << " test LV2 bundles, but found "
    << plugin_identifiers.size ();

  int bundles_with_plugins = 0;
  int total_plugins_found = 0;
  for (const auto &identifier : plugin_identifiers)
    {
      juce::OwnedArray<juce::PluginDescription> descriptions;
      lv2_format->findAllTypesForFile (descriptions, identifier);

      // a bundle containing only presets yields no plugins - that is a
      // valid scan result, not a failure
      if (descriptions.isEmpty ())
        continue;

      bundles_with_plugins++;
      total_plugins_found += descriptions.size ();
      for (const auto * desc : descriptions)
        {
          // descriptions are keyed by the bundle path; the plugin URI is
          // folded into uniqueId
          EXPECT_EQ (desc->pluginFormatName, "LV2");
          EXPECT_TRUE (desc->fileOrIdentifier.endsWith (".lv2"))
            << desc->fileOrIdentifier.toStdString ();
          EXPECT_NE (desc->uniqueId, 0);
          EXPECT_FALSE (desc->name.isEmpty ());
        }
    }

  // every plugin bundle must scan, including the multi-plugin plumbing bundle
  // and the sigabrt bundle (scanning is metadata-only, so plugins that abort
  // on instantiation still scan); the preset-only bundle contributes no
  // plugins
  EXPECT_EQ (bundles_with_plugins, TEST_LV2_BUNDLES_COUNT - 1);
  EXPECT_EQ (total_plugins_found, TEST_LV2_PLUGINS_COUNT)
    << "Expected " << TEST_LV2_PLUGINS_COUNT << " test LV2 plugins, but found "
    << total_plugins_found;
}

// A description must record enough information to detect bundle changes:
// rescanning is only required when the bundle contents changed on disk, and
// entries for bundles that disappeared must be reportable as gone
TEST_F (TestPluginsScanTest, Lv2RescanOnlyWhenBundleChanged)
{
  Lv2PluginFormat format{
    std::make_shared<Lv2World> (Lv2PluginFormat::get_spec_bundles_dir ())
  };

  const auto bundle =
    juce::File (TEST_LV2_SEARCH_PATHS).getChildFile ("eg-amp.lv2");
  juce::OwnedArray<juce::PluginDescription> descriptions;
  format.findAllTypesForFile (descriptions, bundle.getFullPathName ());
  ASSERT_EQ (descriptions.size (), 1);
  const auto &desc = *descriptions.getFirst ();

  EXPECT_FALSE (format.pluginNeedsRescanning (desc));

  const auto ttl = bundle.getChildFile ("amp.ttl");
  const auto original_time = ttl.getLastModificationTime ();
  ASSERT_TRUE (ttl.setLastModificationTime (
    juce::Time::getCurrentTime () + juce::RelativeTime (2.0)));
  EXPECT_TRUE (format.pluginNeedsRescanning (desc));
  ASSERT_TRUE (ttl.setLastModificationTime (original_time));

  EXPECT_TRUE (format.doesPluginStillExist (desc));
  auto missing_desc = desc;
  missing_desc.fileOrIdentifier =
    bundle.getParentDirectory ()
      .getChildFile ("nonexistent.lv2")
      .getFullPathName ();
  EXPECT_FALSE (format.doesPluginStillExist (missing_desc));
}

TEST_F (TestPluginsScanTest, Lv2DescriptionsExposeDeclaredMetadata)
{
  Lv2PluginFormat format{
    std::make_shared<Lv2World> (Lv2PluginFormat::get_spec_bundles_dir ())
  };

  // the instrument declares an author, a class, static port counts and a UI
  juce::OwnedArray<juce::PluginDescription> descriptions;
  const auto                                instrument_bundle =
    juce::File (TEST_LV2_SEARCH_PATHS)
      .getChildFile ("test-instrument.lv2")
      .getFullPathName ();
  format.findAllTypesForFile (descriptions, instrument_bundle);
  ASSERT_EQ (descriptions.size (), 1);
  const auto * desc = descriptions.getFirst ();
  EXPECT_EQ (desc->fileOrIdentifier, instrument_bundle);
  EXPECT_EQ (
    desc->uniqueId,
    get_hash_for_range (
      std::string_view{ "https://lv2.zrythm.org/test-instrument" }));
  EXPECT_EQ (desc->name, "Test Instrument");
  EXPECT_EQ (desc->manufacturerName, "Zrythm");
  EXPECT_EQ (desc->version, "1.0");
  EXPECT_EQ (desc->category, "InstrumentPlugin");
  EXPECT_TRUE (desc->isInstrument);
  EXPECT_EQ (desc->numInputChannels, 0);
  EXPECT_EQ (desc->numOutputChannels, 1);

  // the amp declares its audio port counts statically
  descriptions.clearQuick (true);
  const auto amp_bundle =
    juce::File (TEST_LV2_SEARCH_PATHS)
      .getChildFile ("eg-amp.lv2")
      .getFullPathName ();
  format.findAllTypesForFile (descriptions, amp_bundle);
  ASSERT_EQ (descriptions.size (), 1);
  desc = descriptions.getFirst ();
  EXPECT_EQ (desc->fileOrIdentifier, amp_bundle);
  EXPECT_EQ (
    desc->uniqueId,
    get_hash_for_range (std::string_view{ "http://lv2plug.in/plugins/eg-amp" }));
  EXPECT_EQ (desc->category, "AmplifierPlugin");
  EXPECT_FALSE (desc->isInstrument);
  EXPECT_EQ (desc->numInputChannels, 1);
  EXPECT_EQ (desc->numOutputChannels, 1);
}

// A scan through the scanner subprocess must always be answered with a
// definitive result, even when the scanned identifier contains no plugins
// (e.g. an LV2 bundle that only holds presets) - otherwise the coordinator
// times out and the identifier gets blacklisted.
TEST_F (TestPluginsScanTest, SubprocessScanAnswersForPresetOnlyLv2Bundle)
{
  const auto scanner_path =
    QCoreApplication::applicationDirPath () + QStringLiteral ("/plugin-scanner");
  ASSERT_TRUE (QFile::exists (scanner_path)) << scanner_path.toStdString ();
  qputenv ("ZRYTHM_PLUGIN_SCANNER_PATH", scanner_path.toUtf8 ());

  auto * lv2_format = find_format ("LV2");
  ASSERT_NE (lv2_format, nullptr) << "LV2 format not available";

  discovery::OutOfProcessPluginScanner      scanner;
  juce::OwnedArray<juce::PluginDescription> results;
  const auto                                bundle =
    juce::File (TEST_LV2_SEARCH_PATHS)
      .getChildFile ("test-instrument.preset.lv2")
      .getFullPathName ();
  EXPECT_TRUE (scanner.findPluginTypesFor (*lv2_format, results, bundle));
  EXPECT_TRUE (results.isEmpty ());
}

// Once a bundle's plugins are in a KnownPluginList, an unchanged bundle must
// not be re-scanned: the second scan returns the known entries directly
// instead of querying the format again
TEST_F (TestPluginsScanTest, KnownPluginListSkipsUnchangedLv2Bundles)
{
  auto * lv2_format = find_format ("LV2");
  ASSERT_NE (lv2_format, nullptr) << "LV2 format not available";

  juce::KnownPluginList known_plugins;
  const auto            bundle =
    juce::File (TEST_LV2_SEARCH_PATHS)
      .getChildFile ("eg-amp.lv2")
      .getFullPathName ();

  juce::OwnedArray<juce::PluginDescription> types;
  EXPECT_TRUE (known_plugins.scanAndAddFile (bundle, true, types, *lv2_format));
  EXPECT_EQ (types.size (), 1);
  EXPECT_EQ (known_plugins.getNumTypes (), 1);

  types.clearQuick (true);
  EXPECT_FALSE (known_plugins.scanAndAddFile (bundle, true, types, *lv2_format));
  EXPECT_EQ (types.size (), 1);
  EXPECT_EQ (known_plugins.getNumTypes (), 1);
}

// KnownPluginList matches stored identities by exact string comparison
// (rescan skipping and blacklisting), so search results must come back
// canonical even when a search directory is a symlink
TEST_F (TestPluginsScanTest, Lv2SearchResultsAreCanonical)
{
  auto * lv2_format = find_format ("LV2");
  ASSERT_NE (lv2_format, nullptr) << "LV2 format not available";

  const auto temp_dir =
    utils::io::make_tmp_dir ("zrythm_lv2_symlinked_search_XXXXXX");
  const auto linked_search_path =
    utils::Utf8String::from_qstring (temp_dir->path ()).to_path ()
    / "linked-lv2";
  std::error_code ec;
  std::filesystem::create_directory_symlink (
    TEST_LV2_SEARCH_PATHS, linked_search_path, ec);
  ASSERT_FALSE (ec) << ec.message ();

  const auto results = lv2_format->searchPathsForPlugins (
    juce::FileSearchPath{
      utils::Utf8String::from_path (linked_search_path).to_juce_string () },
    false, false);
  ASSERT_EQ (results.size (), TEST_LV2_BUNDLES_COUNT);
  for (const auto &result : results)
    {
      const auto result_path =
        utils::Utf8String::from_juce_string (result).to_path ();
      EXPECT_EQ (result_path, std::filesystem::weakly_canonical (result_path))
        << result;
    }
}

} // namespace zrythm::plugins
