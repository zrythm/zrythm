// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <filesystem>
#include <fstream>

#include "plugins/lv2_discovery.h"
#include "plugins/lv2_plugin_format.h"
#include "utils/exceptions.h"
#include "utils/io_utils.h"

#include "helpers/scoped_juce_qapplication.h"

#include <gtest/gtest.h>

namespace zrythm::plugins
{

// constructing a juce::AudioPluginFormat requires a JUCE message manager
class Lv2DiscoveryTest
    : public ::testing::Test,
      private test_helpers::ScopedJuceQApplication
{
protected:
  /**
   * @brief Path of a fixture bundle in the build tree.
   */
  static std::filesystem::path bundle_path (const char * bundle_name)
  {
    return std::filesystem::path{ TEST_LV2_SEARCH_PATHS } / bundle_name;
  }

  Lv2World world_{ Lv2PluginFormat::get_spec_bundles_dir () };
};

TEST_F (Lv2DiscoveryTest, ExtractsAmpBundleMetadata)
{
  const auto infos = world_.get_plugins_in_bundle (bundle_path ("eg-amp.lv2"));
  ASSERT_EQ (infos.size (), 1);

  const auto &info = infos.front ();
  EXPECT_EQ (info.uri_, "http://lv2plug.in/plugins/eg-amp");
  EXPECT_NE (info.name_.view ().find ("Amplifier"), std::string_view::npos);
  EXPECT_TRUE (info.author_.view ().empty ()) << info.author_;
  EXPECT_TRUE (info.version_.view ().empty ()) << info.version_;
  EXPECT_EQ (info.category_str_, "AmplifierPlugin");
  EXPECT_FALSE (info.is_instrument_);
  EXPECT_EQ (info.num_audio_ins_, 1);
  EXPECT_EQ (info.num_audio_outs_, 1);
  EXPECT_EQ (info.num_ctrl_ins_, 1);
  EXPECT_EQ (info.num_ctrl_outs_, 0);
  EXPECT_EQ (info.num_midi_ins_, 0);
  EXPECT_EQ (info.num_midi_outs_, 0);
  EXPECT_EQ (info.num_cv_ins_, 0);
  EXPECT_EQ (info.num_cv_outs_, 0);
  EXPECT_FALSE (info.has_custom_ui_);
}

TEST_F (Lv2DiscoveryTest, ExtractsFifthsBundleMetadata)
{
  const auto infos =
    world_.get_plugins_in_bundle (bundle_path ("eg-fifths.lv2"));
  ASSERT_EQ (infos.size (), 1);

  const auto &info = infos.front ();
  EXPECT_EQ (info.uri_, "http://lv2plug.in/plugins/eg-fifths");
  EXPECT_EQ (info.category_str_, "MIDIPlugin");
  EXPECT_FALSE (info.is_instrument_);
  EXPECT_EQ (info.num_audio_ins_, 0);
  EXPECT_EQ (info.num_audio_outs_, 0);
  EXPECT_EQ (info.num_ctrl_ins_, 1);
  EXPECT_EQ (info.num_midi_ins_, 1);
  EXPECT_EQ (info.num_midi_outs_, 1);
  EXPECT_EQ (info.num_cv_ins_, 0);
  EXPECT_EQ (info.num_cv_outs_, 0);
}

TEST_F (Lv2DiscoveryTest, ExtractsInstrumentBundleMetadata)
{
  const auto infos =
    world_.get_plugins_in_bundle (bundle_path ("test-instrument.lv2"));
  ASSERT_EQ (infos.size (), 1);

  const auto &info = infos.front ();
  EXPECT_EQ (info.uri_, "https://lv2.zrythm.org/test-instrument");
  EXPECT_EQ (info.name_, "Test Instrument");
  EXPECT_EQ (info.author_, "Zrythm");
  EXPECT_EQ (info.version_, "1.0");
  EXPECT_EQ (info.category_str_, "InstrumentPlugin");
  EXPECT_TRUE (info.is_instrument_);
  EXPECT_EQ (info.num_audio_ins_, 0);
  EXPECT_EQ (info.num_audio_outs_, 1);
  EXPECT_EQ (info.num_ctrl_ins_, 1);
  EXPECT_EQ (info.num_ctrl_outs_, 0);
  EXPECT_EQ (info.num_midi_ins_, 1);
  EXPECT_EQ (info.num_midi_outs_, 0);
  EXPECT_EQ (info.num_cv_ins_, 0);
  EXPECT_EQ (info.num_cv_outs_, 1);
  // the UI binary is not built, but the declaration must still be extracted
  EXPECT_TRUE (info.has_custom_ui_);
}

TEST_F (Lv2DiscoveryTest, DiscoversMultiplePluginsInOneBundle)
{
  const auto infos = world_.get_plugins_in_bundle (bundle_path ("plumbing.lv2"));
  EXPECT_EQ (infos.size (), 23);
}

TEST_F (Lv2DiscoveryTest, ReturnsEmptyForDirectoryWithoutPlugins)
{
  const auto temp_dir =
    utils::io::make_tmp_dir ("zrythm_lv2_discovery_empty_bundle_XXXXXX");
  const auto infos = world_.get_plugins_in_bundle (
    utils::Utf8String::from_qstring (temp_dir->path ()).to_path ());
  EXPECT_TRUE (infos.empty ());
}

TEST_F (Lv2DiscoveryTest, MissingSpecBundlesAreAFatalError)
{
  const auto temp_dir =
    utils::io::make_tmp_dir ("zrythm_lv2_discovery_missing_specs_XXXXXX");
  // the spec bundles are shipped with the application: a directory without
  // any is a broken installation and must not be silently tolerated
  EXPECT_THROW (
    Lv2World{ utils::Utf8String::from_qstring (temp_dir->path ()).to_path () },
    ZrythmException);
}

TEST_F (Lv2DiscoveryTest, BundlesLoadedIntoOneWorldAreQueriedIndependently)
{
  // loading a bundle must not affect the results of previously loaded ones
  const auto amp_infos =
    world_.get_plugins_in_bundle (bundle_path ("eg-amp.lv2"));
  ASSERT_EQ (amp_infos.size (), 1);

  const auto instrument_infos =
    world_.get_plugins_in_bundle (bundle_path ("test-instrument.lv2"));
  ASSERT_EQ (instrument_infos.size (), 1);
  EXPECT_EQ (
    instrument_infos.front ().uri_, "https://lv2.zrythm.org/test-instrument");
}

// A plugin that only declares the base lv2:Plugin class is uncategorized:
// the discovery must report no category so that the format falls back to
// "Instrument"/"Effect" instead of exposing the meaningless "Plugin"
TEST_F (Lv2DiscoveryTest, UncategorizedPluginHasNoCategory)
{
  const auto temp_dir =
    utils::io::make_tmp_dir ("zrythm_lv2_discovery_uncategorized_XXXXXX");
  const auto bundle_path_ =
    utils::Utf8String::from_qstring (temp_dir->path ()).to_path ()
    / "test-uncategorized.lv2";
  std::filesystem::create_directories (bundle_path_);
  {
    std::ofstream manifest{ bundle_path_ / "manifest.ttl" };
    manifest
      << "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
         "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n"
         "\n"
         "<http://example.org/test-uncategorized>\n"
         "    a lv2:Plugin ;\n"
         "    rdfs:seeAlso <test-uncategorized.ttl> .\n";
    std::ofstream ttl{ bundle_path_ / "test-uncategorized.ttl" };
    ttl
      << "@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .\n"
         "@prefix doap: <http://usefulinc.com/ns/doap#> .\n"
         "\n"
         "<http://example.org/test-uncategorized>\n"
         "    a lv2:Plugin ;\n"
         "    doap:name \"Test Uncategorized\" ;\n"
         "    lv2:port [\n"
         "        a lv2:InputPort , lv2:ControlPort ;\n"
         "        lv2:index 0 ;\n"
         "        lv2:symbol \"gain\" ;\n"
         "        lv2:name \"Gain\" ;\n"
         "        lv2:default 1.0 ;\n"
         "        lv2:minimum 0.0 ;\n"
         "        lv2:maximum 2.0\n"
         "    ] .\n";
  }

  const auto infos = world_.get_plugins_in_bundle (bundle_path_);
  ASSERT_EQ (infos.size (), 1);
  EXPECT_TRUE (infos.front ().category_str_.view ().empty ())
    << infos.front ().category_str_;
  EXPECT_FALSE (infos.front ().is_instrument_);

  Lv2PluginFormat                           format;
  juce::OwnedArray<juce::PluginDescription> results;
  format.findAllTypesForFile (
    results, utils::Utf8String::from_path (bundle_path_).to_juce_string ());
  ASSERT_EQ (results.size (), 1);
  EXPECT_EQ (results[0]->category, "Effect");
}

} // namespace zrythm::plugins
