// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_descriptor_list.h"
#include "utils/gtest_wrapper.h"

#include <QSignalSpy>
#include <QTest>
#include <QtConcurrent>
#include <QtSystemDetection>

#include "helpers/scoped_juce_qapplication.h"
#include "helpers/scoped_qcoreapplication.h"

#include <gmock/gmock.h>

using namespace zrythm::test_helpers;
using namespace std::chrono_literals;

namespace zrythm::plugins::discovery
{

class PluginDescriptorListTest : public ::testing::Test, private ScopedJuceQApplication
{
protected:
  // On Mac there are some issues...
#ifdef Q_OS_MACOS
  static constexpr auto DEBOUNCER_DELAY = 1500ms;
#else
  // Debouncer delay in the implementation (150ms)
  static constexpr auto DEBOUNCER_DELAY = 150ms;
#endif

  // Conservative wait time - use 1.5x multiplier like playback cache scheduler
  static constexpr auto CONSERVATIVE_WAIT = DEBOUNCER_DELAY * 3 / 2;

protected:
  void SetUp () override
  {
    plugin_list_ = std::make_shared<juce::KnownPluginList> ();
    descriptor_list_ = std::make_unique<PluginDescriptorList> (plugin_list_);
  }

  void TearDown () override
  {
    descriptor_list_.reset ();
    plugin_list_.reset ();
  }

  // Helper to create a test juce plugin description
  juce::PluginDescription create_test_juce_description (
    const juce::String &name = "Test Plugin",
    const juce::String &manufacturer = "Test Manufacturer",
    const juce::String &format = "VST3",
    int                 uniqueId = 12345)
  {
    juce::PluginDescription desc;
    desc.name = name;
    desc.manufacturerName = manufacturer;
    desc.pluginFormatName = format;
    desc.uniqueId = uniqueId;
    desc.numInputChannels = 2;
    desc.numOutputChannels = 2;
    desc.isInstrument = false;
    desc.fileOrIdentifier = "/path/to/test/plugin";
    return desc;
  }

  // Helper to create multiple test plugin descriptions
  juce::Array<juce::PluginDescription> create_test_plugin_array (int count)
  {
    juce::Array<juce::PluginDescription> array;
    for (int i = 0; i < count; ++i)
      {
        array.add (create_test_juce_description (
          juce::String ("Test Plugin ") + juce::String (i), "Test Manufacturer",
          "VST3", 12345 + i));
      }
    return array;
  }

  // Helper to add plugins to the real KnownPluginList
  void add_plugins_to_list (const juce::Array<juce::PluginDescription> &plugins)
  {
    for (const auto &plugin : plugins)
      {
        plugin_list_->addType (plugin);
      }
  }

  std::shared_ptr<juce::KnownPluginList> plugin_list_;
  std::unique_ptr<PluginDescriptorList>  descriptor_list_;
};

// Test basic construction
TEST_F (PluginDescriptorListTest, Construction)
{
  // Already created in SetUp(), just verify it exists
  EXPECT_NE (descriptor_list_, nullptr);
  EXPECT_EQ (descriptor_list_->rowCount (), 0);
  EXPECT_EQ (descriptor_list_->columnCount (), 1);
}

// Test role names
TEST_F (PluginDescriptorListTest, RoleNames)
{
  auto roleNames = descriptor_list_->roleNames ();

  EXPECT_TRUE (roleNames.contains (PluginDescriptorList::DescriptorRole));
  EXPECT_TRUE (roleNames.contains (PluginDescriptorList::DescriptorNameRole));
  EXPECT_EQ (roleNames[PluginDescriptorList::DescriptorRole], "descriptor");
  EXPECT_EQ (roleNames[PluginDescriptorList::DescriptorNameRole], "name");
}

// Test empty model behavior
TEST_F (PluginDescriptorListTest, EmptyModel)
{
  // Use the descriptor_list_ created in SetUp()
  EXPECT_EQ (descriptor_list_->rowCount (), 0);
  EXPECT_EQ (descriptor_list_->columnCount (), 1);

  // Test data access with empty model
  auto invalid_index = descriptor_list_->index (0, 0);
  EXPECT_FALSE (invalid_index.isValid ());

  auto data = descriptor_list_->data (invalid_index);
  EXPECT_FALSE (data.isValid ());
}

// Test model with single plugin
TEST_F (PluginDescriptorListTest, SinglePlugin)
{
  auto test_array = create_test_plugin_array (1);
  add_plugins_to_list (test_array);

  // Wait for debounced update to complete
  QTest::qWait (CONSERVATIVE_WAIT);

  EXPECT_EQ (descriptor_list_->rowCount (), 1);

  auto valid_index = descriptor_list_->index (0, 0);
  EXPECT_TRUE (valid_index.isValid ());

  // Test name role
  auto name_data = descriptor_list_->data (
    valid_index, PluginDescriptorList::DescriptorNameRole);
  EXPECT_TRUE (name_data.isValid ());
  EXPECT_EQ (name_data.toString (), QString ("Test Plugin 0"));

  // Test display role (should return name)
  auto display_data = descriptor_list_->data (valid_index, Qt::DisplayRole);
  EXPECT_TRUE (display_data.isValid ());
  EXPECT_EQ (display_data.toString (), QString ("Test Plugin 0"));

  // Test descriptor role
  auto descriptor_data =
    descriptor_list_->data (valid_index, PluginDescriptorList::DescriptorRole);
  EXPECT_TRUE (descriptor_data.isValid ());
  auto * descriptor = qobject_cast<plugins::PluginDescriptor *> (
    descriptor_data.value<QObject *> ());
  EXPECT_NE (descriptor, nullptr);
  EXPECT_EQ (descriptor->getName (), QString ("Test Plugin 0"));
}

// Test model with multiple plugins
TEST_F (PluginDescriptorListTest, MultiplePlugins)
{
  constexpr int plugin_count = 5;
  auto          test_array = create_test_plugin_array (plugin_count);
  add_plugins_to_list (test_array);

  // Wait for debounced update to complete
  QTest::qWait (CONSERVATIVE_WAIT);

  EXPECT_EQ (descriptor_list_->rowCount (), plugin_count);

  // Test all indices are valid
  for (int i = 0; i < plugin_count; ++i)
    {
      auto index = descriptor_list_->index (i, 0);
      EXPECT_TRUE (index.isValid ());

      auto name_data = descriptor_list_->data (
        index, PluginDescriptorList::DescriptorNameRole);
      EXPECT_TRUE (name_data.isValid ());
      EXPECT_TRUE (name_data.toString ().startsWith (QString ("Test Plugin")));
    }

  // Test invalid indices
  auto invalid_index = descriptor_list_->index (plugin_count, 0);
  EXPECT_FALSE (invalid_index.isValid ());

  auto negative_index = descriptor_list_->index (-1, 0);
  EXPECT_FALSE (negative_index.isValid ());
}

// Test index method bounds checking
TEST_F (PluginDescriptorListTest, IndexBoundsChecking)
{
  constexpr int plugin_count = 3;
  auto          test_array = create_test_plugin_array (plugin_count);
  add_plugins_to_list (test_array);

  // Wait for debounced update to complete
  QTest::qWait (CONSERVATIVE_WAIT);

  // Valid indices
  for (int i = 0; i < plugin_count; ++i)
    {
      auto index = descriptor_list_->index (i, 0);
      EXPECT_TRUE (index.isValid ());
      EXPECT_EQ (index.row (), i);
      EXPECT_EQ (index.column (), 0);
    }

  // Invalid indices
  auto out_of_bounds_index = descriptor_list_->index (plugin_count, 0);
  EXPECT_FALSE (out_of_bounds_index.isValid ());

  auto negative_row_index = descriptor_list_->index (-1, 0);
  EXPECT_FALSE (negative_row_index.isValid ());

  // Non-zero column should still be invalid (we only have 1 column)
  auto wrong_col_index = descriptor_list_->index (0, 1);
  EXPECT_FALSE (wrong_col_index.isValid ());
}

// Test parent method (should always return invalid index for flat list)
TEST_F (PluginDescriptorListTest, ParentMethod)
{
  auto parent_index = descriptor_list_->parent (QModelIndex ());
  EXPECT_FALSE (parent_index.isValid ());

  // Even with a valid child index, parent should be invalid
  auto test_array = create_test_plugin_array (1);
  add_plugins_to_list (test_array);

  // Wait for debounced update to complete
  QTest::qWait (CONSERVATIVE_WAIT);

  auto child_index = descriptor_list_->index (0, 0);
  auto parent_of_child = descriptor_list_->parent (child_index);
  EXPECT_FALSE (parent_of_child.isValid ());
}

// Test reset_model method
TEST_F (PluginDescriptorListTest, ResetModel)
{
  auto test_array = create_test_plugin_array (3);
  add_plugins_to_list (test_array);

  // Wait for debounced update to complete
  QTest::qWait (CONSERVATIVE_WAIT);

  EXPECT_EQ (descriptor_list_->rowCount (), 3);

  // Reset should not crash and should maintain same row count
  // (since underlying KnownPluginList hasn't changed)
  EXPECT_NO_THROW (descriptor_list_->reset_model ());
  EXPECT_EQ (descriptor_list_->rowCount (), 3);
}

// Test data method with invalid roles
TEST_F (PluginDescriptorListTest, DataWithInvalidRoles)
{
  auto test_array = create_test_plugin_array (1);
  add_plugins_to_list (test_array);

  // Wait for debounced update to complete
  QTest::qWait (CONSERVATIVE_WAIT);

  auto valid_index = descriptor_list_->index (0, 0);
  EXPECT_TRUE (valid_index.isValid ());

  // Test with invalid role
  auto invalid_role_data =
    descriptor_list_->data (valid_index, Qt::UserRole + 999);
  EXPECT_FALSE (invalid_role_data.isValid ());

  // Test with invalid index
  auto invalid_index = descriptor_list_->index (999, 0);
  EXPECT_FALSE (invalid_index.isValid ());

  auto invalid_index_data =
    descriptor_list_->data (invalid_index, Qt::DisplayRole);
  EXPECT_FALSE (invalid_index_data.isValid ());
}

// Test that descriptor objects are properly created from juce descriptions
TEST_F (PluginDescriptorListTest, DescriptorCreation)
{
  juce::Array<juce::PluginDescription> test_array;
  auto                                 desc = create_test_juce_description (
    "Specific Test Plugin", "Specific Manufacturer", "LV2", 54321);
  test_array.add (desc);
  add_plugins_to_list (test_array);

  // Wait for debounced update to complete
  QTest::qWait (CONSERVATIVE_WAIT);

  EXPECT_EQ (descriptor_list_->rowCount (), 1);

  auto index = descriptor_list_->index (0, 0);
  auto descriptor_data =
    descriptor_list_->data (index, PluginDescriptorList::DescriptorRole);
  EXPECT_TRUE (descriptor_data.isValid ());

  auto * descriptor = qobject_cast<plugins::PluginDescriptor *> (
    descriptor_data.value<QObject *> ());
  EXPECT_NE (descriptor, nullptr);
  EXPECT_EQ (descriptor->getName (), QString ("Specific Test Plugin"));
  EXPECT_EQ (
    descriptor->author_.to_qstring (), QString ("Specific Manufacturer"));
}

// Test column count is always 1
TEST_F (PluginDescriptorListTest, ColumnCount)
{
  // Should always be 1 regardless of plugin count
  EXPECT_EQ (descriptor_list_->columnCount (), 1);

  // Even with plugins
  auto test_array = create_test_plugin_array (5);
  add_plugins_to_list (test_array);

  // Wait for debounced update to complete
  QTest::qWait (CONSERVATIVE_WAIT);

  EXPECT_EQ (descriptor_list_->columnCount (), 1);
}

// Test with different plugin formats
TEST_F (PluginDescriptorListTest, DifferentPluginFormats)
{
  juce::Array<juce::PluginDescription> test_array;

  // Add plugins with different formats
  auto vst3_desc = create_test_juce_description (
    "VST3 Plugin", "VST3 Manufacturer", "VST3", 1001);
  auto lv2_desc = create_test_juce_description (
    "LV2 Plugin", "LV2 Manufacturer", "LV2", 1002);
  auto clap_desc = create_test_juce_description (
    "CLAP Plugin", "CLAP Manufacturer", "CLAP", 1003);

  test_array.add (vst3_desc);
  test_array.add (lv2_desc);
  test_array.add (clap_desc);
  add_plugins_to_list (test_array);

  // Wait for debounced update to complete
  QTest::qWait (CONSERVATIVE_WAIT);

  EXPECT_EQ (descriptor_list_->rowCount (), 3);

  // Verify each plugin's format is correctly translated
  bool found_vst3{};
  bool found_lv2{};
  bool found_clap{};
  for (int i = 0; i < 3; ++i)
    {
      auto index = descriptor_list_->index (i, 0);
      auto descriptor_data =
        descriptor_list_->data (index, PluginDescriptorList::DescriptorRole);
      auto * descriptor = qobject_cast<plugins::PluginDescriptor *> (
        descriptor_data.value<QObject *> ());
      EXPECT_NE (descriptor, nullptr);

      auto format = descriptor->getFormat ();
      if (format == QString ("VST3"))
        {
          found_vst3 = true;
        }
      else if (format == QString ("CLAP"))
        {
          found_clap = true;
        }
      else if (format == QString ("LV2"))
        {
          found_lv2 = true;
        }
    }
  EXPECT_TRUE (found_vst3);
  EXPECT_TRUE (found_clap);
  EXPECT_TRUE (found_lv2);
}

// Test instrument plugins
TEST_F (PluginDescriptorListTest, InstrumentPlugins)
{
  juce::Array<juce::PluginDescription> test_array;

  auto instrument_desc = create_test_juce_description (
    "Instrument Plugin", "Instrument Manufacturer", "VST3", 2001);
  instrument_desc.isInstrument = true;
  instrument_desc.numInputChannels =
    0; // Instruments typically have no audio inputs
  instrument_desc.numOutputChannels = 2;

  test_array.add (instrument_desc);
  add_plugins_to_list (test_array);

  // Wait for debounced update to complete
  QTest::qWait (CONSERVATIVE_WAIT);

  auto index = descriptor_list_->index (0, 0);
  auto descriptor_data =
    descriptor_list_->data (index, PluginDescriptorList::DescriptorRole);
  auto * descriptor = qobject_cast<plugins::PluginDescriptor *> (
    descriptor_data.value<QObject *> ());
  EXPECT_NE (descriptor, nullptr);

  // Should be recognized as an instrument
  EXPECT_TRUE (descriptor->isInstrument ());
}

// Test debounced update functionality
TEST_F (PluginDescriptorListTest, DebouncedUpdate)
{
  // Initially empty
  EXPECT_EQ (descriptor_list_->rowCount (), 0);

  // Add plugins rapidly
  auto test_array = create_test_plugin_array (3);
  add_plugins_to_list (test_array);

  // Should still be empty immediately (debounced)
  EXPECT_EQ (descriptor_list_->rowCount (), 0);

  // Wait for debounced update to complete
  QTest::qWait (CONSERVATIVE_WAIT);

  // Now should have plugins
  EXPECT_EQ (descriptor_list_->rowCount (), 3);
}

// Test change listener triggers update
TEST_F (PluginDescriptorListTest, ChangeListenerTriggersUpdate)
{
  auto test_array = create_test_plugin_array (2);
  add_plugins_to_list (test_array);

  // Wait for initial update to complete
  QTest::qWait (CONSERVATIVE_WAIT);

  EXPECT_EQ (descriptor_list_->rowCount (), 2);

  // Add more plugins after construction (similar plugins get merged)
  auto additional_array = create_test_plugin_array (3);
  add_plugins_to_list (additional_array);

  // Should trigger debounced update
  QTest::qWait (CONSERVATIVE_WAIT);

  EXPECT_EQ (descriptor_list_->rowCount (), 3);
}

} // namespace zrythm::plugins::discovery
