// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_scan_manager.h"
#include "utils/gtest_wrapper.h"

#include <QSignalSpy>
#include <QTest>
#include <QtConcurrent>

#include "helpers/scoped_juce_message_thread.h"
#include "helpers/scoped_qcoreapplication.h"

#include <gmock/gmock.h>

using namespace zrythm::test_helpers;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

namespace zrythm::plugins
{

class TestPathsProvider
{
public:
  std::unique_ptr<utils::FilePathList> operator() (Protocol::ProtocolType prot)
  {
    auto list = std::make_unique<utils::FilePathList> ();
    switch (prot)
      {
      case Protocol::ProtocolType::LV2:
        list->add_path (fs::path ("test/path/lv2"));
        break;
      case Protocol::ProtocolType::VST:
        list->add_path (fs::path ("test/path/vst"));
        break;
      default:
        break;
      }
    return list;
  }
};

class MockPluginList : public juce::KnownPluginList
{
public:
  MOCK_METHOD (
    bool,
    scanAndAddFile,
    (const juce::String &,
     bool,
     juce::OwnedArray<juce::PluginDescription> &,
     juce::AudioPluginFormat &),
    ());
};

class MockAudioFormat : public juce::VST3PluginFormat
{
public:
  MOCK_METHOD (juce::String, getName, (), (const override));
  MOCK_METHOD (
    juce::StringArray,
    searchPathsForPlugins,
    (const juce::FileSearchPath &, bool, bool),
    (override));
};

class MockScanner : public juce::KnownPluginList::CustomScanner
{
public:
  MOCK_METHOD (
    bool,
    findPluginTypesFor,
    (juce::AudioPluginFormat & format,
     juce::OwnedArray<juce::PluginDescription> &result,
     const juce::String                        &fileOrIdentifier),
    (override));
};

class PluginScannerTest
    : public ::testing::TestWithParam<bool>,
      private ScopedQCoreApplication,
      private ScopedJuceApplication
{
protected:
  void SetUp () override
  {
    // set up scanner dependencies
    {
      auto customScannerUniquePtr = std::make_unique<MockScanner> ();
      customScanner_ = customScannerUniquePtr.get ();
      pluginList_ = std::make_shared<NiceMock<MockPluginList>> ();
      pluginList_->setCustomScanner (std::move (customScannerUniquePtr));
    }
    {
      formatManager_ = std::make_shared<juce::AudioPluginFormatManager> ();
      formatManager_->addFormat (new MockAudioFormat ());
      mockFormat_ =
        dynamic_cast<MockAudioFormat *> (formatManager_->getFormat (0));
      // this needs to be an actual format supported by
      // Protocol::from_juce_format_name()
      EXPECT_CALL (*mockFormat_, getName ()).WillRepeatedly (Return ("VST3"));
    }
  }

  void mock_single_plugin ()
  {
    // Set up mock expectations
    constexpr auto test_descriptor_generator = [] () {
      auto * descr = new juce::PluginDescription ();
      descr->name = "Test Plugin";
      return descr;
    };

    EXPECT_CALL (*mockFormat_, searchPathsForPlugins (_, _, _))
      .WillRepeatedly (Return (juce::StringArray{ "test-plugin" }));

    EXPECT_CALL (*pluginList_, scanAndAddFile (_, _, _, _))
      .WillRepeatedly (
        [test_descriptor_generator] (
          const juce::String &, bool,
          juce::OwnedArray<juce::PluginDescription> &outTypes,
          juce::AudioPluginFormat &) {
          outTypes.add (test_descriptor_generator ());
          z_debug ("scanAndAddFile called");
          return true;
        });

    EXPECT_CALL (*customScanner_, findPluginTypesFor (_, _, _))
      .WillRepeatedly (
        [test_descriptor_generator] (
          const juce::AudioPluginFormat &,
          juce::OwnedArray<juce::PluginDescription> &outTypes,
          const juce::String &) {
          outTypes.add (test_descriptor_generator ());
          z_debug ("findPluginTypesFor called");
          return true;
        });
  }

  void TearDown () override { }

  // non-owning pointer (owned by pluginList_)
  MockScanner * customScanner_{};

  // non-owning pointer (owned by formatManager_)
  MockAudioFormat * mockFormat_{};

  std::shared_ptr<NiceMock<MockPluginList>>       pluginList_;
  std::shared_ptr<juce::AudioPluginFormatManager> formatManager_;
  TestPathsProvider                               pathsProvider_;
};

TEST_P (PluginScannerTest, SinglePluginScan)
{
  PluginScanManager scanner (
    pluginList_, formatManager_, pathsProvider_, nullptr);

  QSignalSpy finishedSpy (&scanner, &PluginScanManager::scanningFinished);
  QSignalSpy pluginChangedSpy (
    &scanner, &PluginScanManager::currentlyScanningPluginChanged);

  const bool test_with_plugin = GetParam ();
  if (test_with_plugin)
    {
      mock_single_plugin ();
    }

  scanner.beginScan ();

  if (test_with_plugin)
    {
      pluginChangedSpy.wait (1000);
      EXPECT_FALSE (scanner.getCurrentlyScanningPlugin ().isEmpty ());
      EXPECT_EQ (
        utils::Utf8String::from_qstring (scanner.getCurrentlyScanningPlugin ())
          .str (),
        "test-plugin");
    }

  finishedSpy.wait (1000);

  EXPECT_EQ (finishedSpy.count (), 1);
  if (test_with_plugin)
    {
      EXPECT_EQ (pluginList_->getNumTypes (), 1);
      EXPECT_EQ (
        pluginList_->getTypes ().getFirst ().name, juce::String ("Test Plugin"));
      EXPECT_GT (pluginChangedSpy.count (), 0);
    }
  else
    {
      EXPECT_EQ (pluginList_->getNumTypes (), 0);
    }
}

INSTANTIATE_TEST_SUITE_P (
  PluginScanTests,
  PluginScannerTest,
  ::testing::Values (false, true));
}

/*
 * TODO: Test Suite Improvements Needed:
 *
 * 1. Test Coverage:
 *    - Add tests for edge cases (invalid paths, empty paths, unsupported
 * formats)
 *    - Test scan cancellation if supported
 *    - Verify plugin_paths_provider interaction with all protocol types
 *
 * 2. Behavioral Verification:
 *    - Explicitly test scan_for_plugins() protocol iteration logic
 *    - Add thread-safety verification for currentlyScanningPlugin_
 *
 * 3. Code Structure:
 *    - Refactor mock_single_plugin() into reusable helper
 *    - Add negative tests (scan failures, false returns)
 *
 * 4. Documentation:
 *    - Clarify test descriptions with expected behavior
 *    - Add comments explaining mock roles
 */
