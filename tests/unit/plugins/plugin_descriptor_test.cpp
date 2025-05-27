// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_descriptor.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::plugins;

namespace
{

// Helper function to create and return a unique_ptr to a basic descriptor
std::unique_ptr<PluginDescriptor>
create_test_descriptor (
  ZPluginCategory        category = ZPluginCategory::NONE,
  Protocol::ProtocolType protocol = Protocol::ProtocolType::Internal)
{
  auto desc = std::make_unique<PluginDescriptor> ();
  desc->name_ = u8"Test Plugin";
  desc->author_ = u8"Test Author";
  desc->category_ = category;
  desc->protocol_ = protocol;
  desc->num_audio_ins_ = 2;
  desc->num_audio_outs_ = 2;
  desc->num_midi_ins_ = 1;
  desc->num_midi_outs_ = 1;
  return desc;
}

} // namespace

TEST (PluginDescriptorTest, TypeDetection)
{
  auto instrument = create_test_descriptor (ZPluginCategory::INSTRUMENT);
  EXPECT_TRUE (instrument->is_instrument ());
  EXPECT_FALSE (instrument->is_effect ());
  EXPECT_FALSE (instrument->is_modulator ());
  EXPECT_FALSE (instrument->is_midi_modifier ());

  auto effect = create_test_descriptor (ZPluginCategory::DELAY);
  EXPECT_FALSE (effect->is_instrument ());
  EXPECT_TRUE (effect->is_effect ());
  EXPECT_FALSE (effect->is_modulator ());
  EXPECT_FALSE (effect->is_midi_modifier ());

  auto modulator = create_test_descriptor (ZPluginCategory::MODULATOR);
  modulator->num_cv_outs_ = 1;
  EXPECT_FALSE (modulator->is_instrument ());
  EXPECT_TRUE (modulator->is_effect ());
  EXPECT_TRUE (modulator->is_modulator ());
  EXPECT_FALSE (modulator->is_midi_modifier ());

  auto midi_mod = create_test_descriptor (ZPluginCategory::MIDI);
  EXPECT_FALSE (midi_mod->is_instrument ());
  EXPECT_FALSE (midi_mod->is_effect ());
  EXPECT_FALSE (midi_mod->is_modulator ());
  EXPECT_TRUE (midi_mod->is_midi_modifier ());
}

TEST (PluginDescriptorTest, CategoryConversion)
{
  EXPECT_EQ (
    PluginDescriptor::string_to_category (u8"Delay"), ZPluginCategory::DELAY);
  EXPECT_EQ (
    PluginDescriptor::string_to_category (u8"Equalizer"), ZPluginCategory::EQ);

  EXPECT_EQ (
    PluginDescriptor::category_to_string (ZPluginCategory::REVERB), u8"Reverb");
}

TEST (PluginDescriptorTest, CloneAndEquality)
{
  auto original = create_test_descriptor (ZPluginCategory::COMPRESSOR);
  auto clone = original->clone_unique ();

  EXPECT_TRUE (original->is_same_plugin (*clone));

  clone->protocol_ = Protocol::ProtocolType::AudioUnit;
  EXPECT_FALSE (original->is_same_plugin (*clone));
}

TEST (PluginDescriptorTest, Serialization)
{
  auto original = create_test_descriptor (ZPluginCategory::DELAY);
  original->path_or_id_ = utils::Utf8String (u8"http://example.org/plugin");
  original->unique_id_ = 12345;

  nlohmann::json j = *original;
  auto           deserialized = std::make_unique<PluginDescriptor> ();
  from_json (j, *deserialized);

  EXPECT_TRUE (original->is_same_plugin (*deserialized));
}
