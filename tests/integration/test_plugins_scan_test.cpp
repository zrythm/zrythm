// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <filesystem>
#include <string_view>

#include "plugins/clap_plugin_format.h"
#include "plugins/lv2_plugin_format.h"
#include "plugins/lv2_world.h"
#include "plugins/out_of_process_scanner.h"
#include "plugins/plugin_catalog.h"
#include "plugins/plugin_format_utils.h"
#include "plugins/plugin_scan_format.h"
#include "plugins/vst3_plugin_format.h"
#include "utils/file_path_list.h"
#include "utils/icloneable.h"
#include "utils/io_utils.h"
#include "utils/logger.h"
#include "utils/qt.h"
#include "utils/utf8_string.h"

#include <QFile>
#include <QStringList>

#include "helpers/scoped_juce_qapplication.h"

#include <gtest/gtest.h>

using namespace zrythm::test_helpers;
using namespace std::string_view_literals;

namespace zrythm::plugins
{

/**
 * @brief Integration test for test plugin scanning (VST3, CLAP and LV2).
 *
 * Verifies that the test fixtures are discoverable, scan into complete
 * Zrythm descriptors, and that the port counts reported by the descriptors
 * classify plugins correctly.
 */
class TestPluginsScanTest : public ::testing::Test, private ScopedJuceQApplication
{
protected:
  // Expected manufacturer name for all test plugins
  static constexpr std::string_view EXPECTED_MANUFACTURER = "Zrythm";

  void SetUp () override
  {
    scan_formats_ = std::make_shared<PluginScanFormatManager> (
      std::make_shared<Lv2World> (Lv2PluginFormat::get_spec_bundles_dir ()));
  }

  void TearDown () override { scan_formats_.reset (); }

  /**
   * @brief Parses a compile-definition search path string (:::-separated)
   * into a FilePathList.
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

  static std::unique_ptr<utils::FilePathList> vst3_search_paths ()
  {
    return parse_search_paths (QStringLiteral (TEST_VST3_SEARCH_PATHS));
  }

  static std::unique_ptr<utils::FilePathList> clap_search_paths ()
  {
    return parse_search_paths (QStringLiteral (TEST_CLAP_SEARCH_PATHS));
  }

  static std::unique_ptr<utils::FilePathList> lv2_search_paths ()
  {
    return parse_search_paths (QStringLiteral (TEST_LV2_SEARCH_PATHS));
  }

  [[nodiscard]] PluginScanFormat *
  find_format (std::string_view format_name) const
  {
    return scan_formats_->find_format (
      utils::Utf8String::from_utf8_encoded_string (std::string{ format_name }));
  }

  /**
   * @brief Scans every identifier found directly below the search paths.
   *
   * @return All descriptors found; identifiers that fail to scan are
   * reported as failures and skipped.
   */
  static std::vector<std::unique_ptr<PluginDescriptor>>
  scan_all (PluginScanFormat &format, const utils::FilePathList &paths)
  {
    std::vector<std::unique_ptr<PluginDescriptor>> descriptors;
    for (const auto &identifier : format.search_identifiers (paths, false))
      {
        auto result = format.scan_identifier (identifier);
        if (!result.has_value ())
          {
            ADD_FAILURE ()
              << "Scan failed for " << identifier.view () << ": "
              << result.error ().view ();
            continue;
          }
        descriptors.insert (
          descriptors.end (), std::make_move_iterator (result->begin ()),
          std::make_move_iterator (result->end ()));
      }
    return descriptors;
  }

  [[nodiscard]] static const PluginDescriptor * find_by_name (
    const std::vector<std::unique_ptr<PluginDescriptor>> &descriptors,
    std::string_view                                      name)
  {
    const auto it = std::ranges::find_if (
      descriptors, [name] (const std::unique_ptr<PluginDescriptor> &descriptor) {
        return descriptor->name_.view () == name;
      });
    return it == descriptors.end () ? nullptr : it->get ();
  }

  /** Returns the identifier whose path ends with @p filename. */
  [[nodiscard]] static utils::Utf8String find_identifier (
    const PluginScanFormat    &format,
    const utils::FilePathList &paths,
    std::string_view           filename)
  {
    for (const auto &identifier : format.search_identifiers (paths, false))
      {
        if (identifier.view ().ends_with (filename))
          return identifier;
      }
    return utils::Utf8String{};
  }

  static void set_up_scanner_path ()
  {
    const auto scanner_path = QString::fromUtf8 (TEST_PLUGIN_SCANNER_PATH);
    ASSERT_TRUE (QFile::exists (scanner_path)) << scanner_path.toStdString ();
    qputenv ("ZRYTHM_PLUGIN_SCANNER_PATH", scanner_path.toUtf8 ());
  }

  std::shared_ptr<PluginScanFormatManager> scan_formats_;
};

// ============================================================================
// VST3 tests
// ============================================================================

TEST_F (TestPluginsScanTest, ScanAllTestVst3Plugins)
{
  auto * vst3_format = find_format ("VST3");
  ASSERT_NE (vst3_format, nullptr) << "VST3 format not available";

  const auto identifiers =
    vst3_format->search_identifiers (*vst3_search_paths (), false);
  EXPECT_EQ (identifiers.size (), TEST_VST3_PLUGINS_COUNT)
    << "Expected " << TEST_VST3_PLUGINS_COUNT
    << " test VST3 plugins, but found " << identifiers.size ();

  const auto descriptors = scan_all (*vst3_format, *vst3_search_paths ());
  EXPECT_EQ (descriptors.size (), TEST_VST3_PLUGINS_COUNT);
  for (const auto &descriptor : descriptors)
    {
      EXPECT_EQ (descriptor->author_.view (), EXPECTED_MANUFACTURER)
        << descriptor->name_.view ();
      EXPECT_FALSE (descriptor->version_.empty ());
      EXPECT_EQ (descriptor->protocol_, Protocol::ProtocolType::VST3);
      EXPECT_GT (descriptor->last_file_mod_time_ms_, 0);
    }
}

// Plugin types are non-exclusive and derive from the declared ports: the
// synth consumes and emits events, so it is both an instrument and a MIDI
// modifier; the gain fixture processes audio only
TEST_F (TestPluginsScanTest, Vst3PortCountsClassifyPlugins)
{
  auto * vst3_format = find_format ("VST3");
  ASSERT_NE (vst3_format, nullptr);

  const auto descriptors = scan_all (*vst3_format, *vst3_search_paths ());

  const auto * synth = find_by_name (descriptors, "Test Synth");
  ASSERT_NE (synth, nullptr);
  EXPECT_EQ (synth->num_audio_ins_, 0);
  EXPECT_EQ (synth->num_audio_outs_, 2);
  EXPECT_EQ (synth->num_midi_ins_, 1);
  EXPECT_EQ (synth->num_midi_outs_, 1);
  EXPECT_TRUE (synth->isInstrument ());
  EXPECT_TRUE (synth->isMidiModifier ());
  EXPECT_FALSE (synth->isModulator ());

  const auto * gain = find_by_name (descriptors, "Test Gain");
  ASSERT_NE (gain, nullptr);
  EXPECT_EQ (gain->num_audio_ins_, 2);
  EXPECT_EQ (gain->num_audio_outs_, 2);
  EXPECT_EQ (gain->num_midi_ins_, 1);
  EXPECT_EQ (gain->num_midi_outs_, 0);
  EXPECT_TRUE (gain->isEffect ());
  EXPECT_FALSE (gain->isInstrument ());
  EXPECT_FALSE (gain->isMidiModifier ());
  EXPECT_FALSE (gain->isModulator ());
}

// ============================================================================
// CLAP tests
// ============================================================================

TEST_F (TestPluginsScanTest, ScanAllTestClapPlugins)
{
  auto * clap_format = find_format ("CLAP");
  ASSERT_NE (clap_format, nullptr) << "CLAP format not available";

  const auto identifiers =
    clap_format->search_identifiers (*clap_search_paths (), false);
  ASSERT_FALSE (identifiers.empty ())
    << "No CLAP search paths configured - check TEST_CLAP_SEARCH_PATHS";

  // every fixture file scans, including the synth bundle that holds both
  // note dialect variants (2 plugins)
  const auto descriptors = scan_all (*clap_format, *clap_search_paths ());
  EXPECT_EQ (descriptors.size (), TEST_CLAP_PLUGINS_COUNT)
    << "Expected " << TEST_CLAP_PLUGINS_COUNT
    << " test CLAP plugins, but found " << descriptors.size ();
  for (const auto &descriptor : descriptors)
    {
      EXPECT_FALSE (descriptor->name_.empty ());
      EXPECT_EQ (descriptor->author_.view (), EXPECTED_MANUFACTURER)
        << descriptor->name_.view ();
      EXPECT_FALSE (descriptor->version_.empty ());
    }
}

TEST_F (TestPluginsScanTest, ClapPortCountsClassifyPlugins)
{
  auto * clap_format = find_format ("CLAP");
  ASSERT_NE (clap_format, nullptr);

  const auto descriptors = scan_all (*clap_format, *clap_search_paths ());

  const auto * synth = find_by_name (descriptors, "Test Synth");
  ASSERT_NE (synth, nullptr);
  EXPECT_EQ (synth->num_audio_ins_, 0);
  EXPECT_EQ (synth->num_audio_outs_, 2);
  EXPECT_EQ (synth->num_midi_ins_, 1);
  EXPECT_EQ (synth->num_midi_outs_, 1);
  EXPECT_TRUE (synth->isInstrument ());
  EXPECT_TRUE (synth->isMidiModifier ());
  EXPECT_FALSE (synth->isModulator ());

  const auto * gain = find_by_name (descriptors, "Test Gain");
  ASSERT_NE (gain, nullptr);
  EXPECT_EQ (gain->num_audio_ins_, 2);
  EXPECT_EQ (gain->num_audio_outs_, 2);
  EXPECT_EQ (gain->num_midi_ins_, 0);
  EXPECT_EQ (gain->num_midi_outs_, 0);
  EXPECT_TRUE (gain->isEffect ());
  EXPECT_FALSE (gain->isInstrument ());
  EXPECT_FALSE (gain->isMidiModifier ());
  EXPECT_FALSE (gain->isModulator ());
}

// ============================================================================
// LV2 tests
// ============================================================================

TEST_F (TestPluginsScanTest, ScanAllTestLv2Plugins)
{
  auto * lv2_format = find_format ("LV2");
  ASSERT_NE (lv2_format, nullptr) << "LV2 format not available";

  const auto identifiers =
    lv2_format->search_identifiers (*lv2_search_paths (), false);
  EXPECT_EQ (identifiers.size (), TEST_LV2_BUNDLES_COUNT)
    << "Expected " << TEST_LV2_BUNDLES_COUNT << " test LV2 bundles, but found "
    << identifiers.size ();

  int bundles_with_plugins = 0;
  int total_plugins_found = 0;
  for (const auto &identifier : identifiers)
    {
      auto result = lv2_format->scan_identifier (identifier);
      ASSERT_TRUE (result.has_value ()) << identifier.view ();

      // a bundle containing only presets yields no plugins - that is a
      // valid scan result, not a failure
      if (result->empty ())
        continue;

      bundles_with_plugins++;
      total_plugins_found += static_cast<int> (result->size ());
      for (const auto &descriptor : *result)
        {
          EXPECT_EQ (descriptor->protocol_, Protocol::ProtocolType::LV2);
          EXPECT_TRUE (identifier.to_qstring ().endsWith (".lv2"))
            << identifier.view ();
          EXPECT_NE (descriptor->unique_id_, 0);
          EXPECT_FALSE (descriptor->name_.empty ());
        }
    }

  // every plugin bundle must scan, including the multi-plugin plumbing
  // bundle and the sigabrt bundle (scanning is metadata-only, so plugins
  // that abort on instantiation still scan); the preset-only bundle
  // contributes no plugins
  EXPECT_EQ (bundles_with_plugins, TEST_LV2_BUNDLES_COUNT - 1);
  EXPECT_EQ (total_plugins_found, TEST_LV2_PLUGINS_COUNT)
    << "Expected " << TEST_LV2_PLUGINS_COUNT << " test LV2 plugins, but found "
    << total_plugins_found;
}

// LV2 declares its port counts statically: MIDI ports are atom ports that
// announce midi:MidiEvent support, and CV ports exist only in LV2
TEST_F (TestPluginsScanTest, Lv2PortCountsComeFromDeclaredPorts)
{
  auto * lv2_format = find_format ("LV2");
  ASSERT_NE (lv2_format, nullptr);

  const auto instrument_identifier =
    find_identifier (*lv2_format, *lv2_search_paths (), "test-instrument.lv2");
  ASSERT_FALSE (instrument_identifier.empty ());
  auto instrument_result = lv2_format->scan_identifier (instrument_identifier);
  ASSERT_TRUE (instrument_result.has_value ());
  ASSERT_EQ (instrument_result->size (), 1);
  const auto &instrument = *instrument_result->front ();
  EXPECT_EQ (instrument.name_.view (), "Test Instrument");
  EXPECT_EQ (instrument.num_midi_ins_, 1);
  EXPECT_EQ (instrument.num_audio_outs_, 1);
  EXPECT_EQ (instrument.num_ctrl_ins_, 1);
  EXPECT_EQ (instrument.num_cv_ins_, 0);
  EXPECT_EQ (instrument.num_cv_outs_, 1);
  EXPECT_TRUE (instrument.isInstrument ());
  EXPECT_TRUE (instrument.isModulator ());

  const auto amp_identifier =
    find_identifier (*lv2_format, *lv2_search_paths (), "eg-amp.lv2");
  ASSERT_FALSE (amp_identifier.empty ());
  auto amp_result = lv2_format->scan_identifier (amp_identifier);
  ASSERT_TRUE (amp_result.has_value ());
  // the amp and the float-window amp
  ASSERT_EQ (amp_result->size (), 2);
  const auto amp_unique_id =
    get_hash_for_range (std::string_view{ "http://lv2plug.in/plugins/eg-amp" });
  const PluginDescriptor * amp = nullptr;
  for (const auto &descriptor : *amp_result)
    {
      if (descriptor->unique_id_ == amp_unique_id)
        amp = descriptor.get ();
    }
  ASSERT_NE (amp, nullptr);
  EXPECT_EQ (amp->num_audio_ins_, 1);
  EXPECT_EQ (amp->num_audio_outs_, 1);
  // the message atom port announces atom:String, not MIDI, so it does not
  // count as a MIDI input
  EXPECT_EQ (amp->num_midi_ins_, 0);
  EXPECT_EQ (amp->num_midi_outs_, 0);
  EXPECT_EQ (amp->num_cv_outs_, 0);
  EXPECT_TRUE (amp->isEffect ());
  EXPECT_FALSE (amp->isInstrument ());
  EXPECT_FALSE (amp->isModulator ());
  EXPECT_FALSE (amp->isMidiModifier ());
}

// A descriptor must record enough information to detect bundle changes:
// rescanning is only required when the bundle contents changed on disk,
// and entries for bundles that disappeared must be reportable as gone
TEST_F (TestPluginsScanTest, Lv2RescanOnlyWhenBundleChanged)
{
  Lv2PluginFormat format{
    std::make_shared<Lv2World> (Lv2PluginFormat::get_spec_bundles_dir ())
  };

  const auto bundle =
    juce::File (TEST_LV2_SEARCH_PATHS).getChildFile ("eg-amp.lv2");
  auto result = format.scan_identifier (
    utils::Utf8String::from_juce_string (bundle.getFullPathName ()));
  ASSERT_TRUE (result.has_value ());
  // the amp and the float-window amp
  ASSERT_EQ (result->size (), 2);
  const auto &descriptor = *result->front ();

  EXPECT_FALSE (format.plugin_needs_rescanning (descriptor));

  const auto ttl = bundle.getChildFile ("amp.ttl");
  const auto original_time = ttl.getLastModificationTime ();
  ASSERT_TRUE (ttl.setLastModificationTime (
    juce::Time::getCurrentTime () + juce::RelativeTime (2.0)));
  EXPECT_TRUE (format.plugin_needs_rescanning (descriptor));
  ASSERT_TRUE (ttl.setLastModificationTime (original_time));

  EXPECT_TRUE (format.plugin_exists (descriptor));
  auto missing = utils::clone_unique (descriptor);
  missing->path_or_id_ =
    utils::Utf8String::from_juce_string (
      bundle.getParentDirectory ()
        .getChildFile ("nonexistent.lv2")
        .getFullPathName ())
      .to_path ();
  EXPECT_FALSE (format.plugin_exists (*missing));
}

// ============================================================================
// Subprocess and catalog tests
// ============================================================================

// A scan through the scanner subprocess must always be answered with a
// definitive result, even when the scanned identifier contains no plugins
// (e.g. an LV2 bundle that only holds presets) - otherwise the coordinator
// times out and the identifier gets blacklisted
TEST_F (TestPluginsScanTest, SubprocessScanAnswersForIdentifierWithoutPlugins)
{
  set_up_scanner_path ();
  discovery::OutOfProcessPluginScanner scanner;

  const auto bundle =
    juce::File (TEST_LV2_SEARCH_PATHS)
      .getChildFile ("test-instrument.preset.lv2")
      .getFullPathName ();
  auto result = scanner.scan (
    utils::Utf8String::from_utf8_encoded_string ("LV2"),
    utils::Utf8String::from_juce_string (bundle), [] { return false; });
  ASSERT_TRUE (result.has_value ()) << result.error ().view ();
  EXPECT_TRUE (result->empty ());
}

// The scanner subprocess communicates results as versioned JSON documents;
// scanning a real VST3 bundle through it must yield the same complete
// descriptor an in-process scan produces
TEST_F (TestPluginsScanTest, SubprocessScanReturnsCompleteVst3Descriptor)
{
  set_up_scanner_path ();
  auto * vst3_format = find_format ("VST3");
  ASSERT_NE (vst3_format, nullptr);
  discovery::OutOfProcessPluginScanner scanner;

  const auto identifier =
    find_identifier (*vst3_format, *vst3_search_paths (), "Test Gain.vst3");
  ASSERT_FALSE (identifier.empty ());

  auto result = scanner.scan (vst3_format->format_name (), identifier, [] {
    return false;
  });
  ASSERT_TRUE (result.has_value ()) << result.error ().view ();
  ASSERT_EQ (result->size (), 1);
  const auto &descriptor = *result->front ();
  EXPECT_EQ (descriptor.name_.view (), "Test Gain");
  EXPECT_EQ (descriptor.protocol_, Protocol::ProtocolType::VST3);
  EXPECT_EQ (descriptor.author_.view (), EXPECTED_MANUFACTURER);
  EXPECT_EQ (descriptor.num_audio_ins_, 2);
  EXPECT_EQ (descriptor.num_audio_outs_, 2);
  EXPECT_EQ (descriptor.num_midi_ins_, 1);
  EXPECT_EQ (descriptor.num_midi_outs_, 0);
  EXPECT_GT (descriptor.last_file_mod_time_ms_, 0);
}

// A format the subprocess does not register is an environment condition
// that applies to every identifier of the format: the scan answers with no
// plugins and the identifier is not recorded as failed
TEST_F (TestPluginsScanTest, SubprocessScanOfUnregisteredFormatReportsNoPlugins)
{
  set_up_scanner_path ();
  discovery::OutOfProcessPluginScanner scanner;

  auto result = scanner.scan (
    utils::Utf8String::from_utf8_encoded_string ("UnregisteredFormat"),
    utils::Utf8String::from_utf8_encoded_string ("/nonexistent/plugin.vst3"),
    [] { return false; });
  ASSERT_TRUE (result.has_value ()) << result.error ().view ();
  EXPECT_TRUE (result->empty ());
}

// A cancelled scan aborts without waiting for the subprocess response or the
// scan timeout
TEST_F (TestPluginsScanTest, CancelledSubprocessScanReturnsNoPlugins)
{
  set_up_scanner_path ();
  discovery::OutOfProcessPluginScanner scanner;

  auto result = scanner.scan (
    utils::Utf8String::from_utf8_encoded_string ("VST3"),
    utils::Utf8String::from_utf8_encoded_string ("/nonexistent/plugin.vst3"),
    [] { return true; });
  ASSERT_TRUE (result.has_value ()) << result.error ().view ();
  EXPECT_TRUE (result->empty ());
}

// The known-plugins cache must round-trip enough information that a
// reloaded catalog reports identifiers as up to date without rescanning
TEST_F (TestPluginsScanTest, CatalogReloadPreservesDescriptorsAndScanState)
{
  auto * vst3_format = find_format ("VST3");
  ASSERT_NE (vst3_format, nullptr);

  const auto identifier =
    find_identifier (*vst3_format, *vst3_search_paths (), "Test Gain.vst3");
  ASSERT_FALSE (identifier.empty ());
  auto result = vst3_format->scan_identifier (identifier);
  ASSERT_TRUE (result.has_value ());
  ASSERT_EQ (result->size (), 1);

  discovery::PluginCatalog catalog;
  catalog.add_descriptor (std::move (result->front ()));

  discovery::PluginCatalog reloaded;
  reloaded.deserialize (catalog.serialize ());
  ASSERT_EQ (reloaded.descriptor_count (), 1);
  EXPECT_EQ (reloaded.descriptors ().front ()->name_.view (), "Test Gain");
  EXPECT_EQ (
    reloaded.scan_state (
      Protocol::ProtocolType::VST3, identifier,
      [vst3_format] (const PluginDescriptor &descriptor) {
        return vst3_format->plugin_needs_rescanning (descriptor);
      }),
    discovery::PluginCatalog::ScanState::UpToDate);
}

// Scan identities survive across sessions by exact string comparison, so
// search results must come back canonical even when a search directory is
// a symlink
TEST_F (TestPluginsScanTest, Lv2SearchResultsAreCanonical)
{
  auto * lv2_format = find_format ("LV2");
  ASSERT_NE (lv2_format, nullptr);

  const auto temp_dir =
    utils::io::make_tmp_dir ("zrythm_lv2_symlinked_search_XXXXXX");
  const auto linked_search_path =
    utils::Utf8String::from_qstring (temp_dir->path ()).to_path ()
    / "linked-lv2";
  std::error_code ec;
  std::filesystem::create_directory_symlink (
    std::filesystem::path (TEST_LV2_SEARCH_PATHS), linked_search_path, ec);
  ASSERT_FALSE (ec) << ec.message ();

  utils::FilePathList paths;
  paths.add_path (linked_search_path);
  const auto results = lv2_format->search_identifiers (paths, false);
  ASSERT_EQ (results.size (), TEST_LV2_BUNDLES_COUNT);
  for (const auto &result : results)
    {
      const auto result_path = result.to_path ();
      EXPECT_EQ (result_path, std::filesystem::weakly_canonical (result_path))
        << result.view ();
    }
}

} // namespace zrythm::plugins
