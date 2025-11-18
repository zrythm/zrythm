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
  PluginCategory         category = PluginCategory::None,
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
  auto instrument = create_test_descriptor (PluginCategory::Instrument);
  EXPECT_TRUE (instrument->isInstrument ());
  EXPECT_FALSE (instrument->isEffect ());
  EXPECT_FALSE (instrument->isModulator ());
  EXPECT_FALSE (instrument->isMidiModifier ());

  auto effect = create_test_descriptor (PluginCategory::Delay);
  EXPECT_FALSE (effect->isInstrument ());
  EXPECT_TRUE (effect->isEffect ());
  EXPECT_FALSE (effect->isModulator ());
  EXPECT_FALSE (effect->isMidiModifier ());

  auto modulator = create_test_descriptor (PluginCategory::MODULATOR);
  modulator->num_cv_outs_ = 1;
  EXPECT_FALSE (modulator->isInstrument ());
  EXPECT_TRUE (modulator->isEffect ());
  EXPECT_TRUE (modulator->isModulator ());
  EXPECT_FALSE (modulator->isMidiModifier ());

  auto midi_mod = create_test_descriptor (PluginCategory::MIDI);
  EXPECT_FALSE (midi_mod->isInstrument ());
  EXPECT_FALSE (midi_mod->isEffect ());
  EXPECT_FALSE (midi_mod->isModulator ());
  EXPECT_TRUE (midi_mod->isMidiModifier ());
}

TEST (PluginDescriptorTest, CategoryConversion)
{
  EXPECT_EQ (
    PluginDescriptor::string_to_category (u8"Delay"), PluginCategory::Delay);
  EXPECT_EQ (
    PluginDescriptor::string_to_category (u8"Equalizer"), PluginCategory::EQ);

  EXPECT_EQ (
    PluginDescriptor::category_to_string (PluginCategory::REVERB), u8"Reverb");
}

TEST (PluginDescriptorTest, CloneAndEquality)
{
  auto original = create_test_descriptor (PluginCategory::COMPRESSOR);
  auto clone = utils::clone_unique (*original);

  EXPECT_TRUE (original->is_same_plugin (*clone));

  clone->protocol_ = Protocol::ProtocolType::AudioUnit;
  EXPECT_FALSE (original->is_same_plugin (*clone));
}

TEST (PluginDescriptorTest, Serialization)
{
  auto original = create_test_descriptor (PluginCategory::Delay);
  original->path_or_id_ = utils::Utf8String (u8"http://example.org/plugin");
  original->unique_id_ = 12345;

  nlohmann::json j = *original;
  auto           deserialized = std::make_unique<PluginDescriptor> ();
  from_json (j, *deserialized);

  EXPECT_TRUE (original->is_same_plugin (*deserialized));
}
