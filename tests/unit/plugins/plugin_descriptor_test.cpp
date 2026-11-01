// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_descriptor.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::plugins
{

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
  return desc;
}

} // namespace

TEST (PluginDescriptorTest, TypeDetection)
{
  auto instrument = create_test_descriptor (PluginCategory::Instrument);
  instrument->num_audio_ins_ = 0;
  instrument->num_midi_ins_ = 1;
  EXPECT_TRUE (instrument->isInstrument ());
  EXPECT_FALSE (instrument->isEffect ());
  EXPECT_FALSE (instrument->isModulator ());
  EXPECT_FALSE (instrument->isMidiModifier ());

  auto effect = create_test_descriptor (PluginCategory::Delay);
  EXPECT_FALSE (effect->isInstrument ());
  EXPECT_TRUE (effect->isEffect ());
  EXPECT_FALSE (effect->isModulator ());
  EXPECT_FALSE (effect->isMidiModifier ());

  auto modulator = create_test_descriptor ();
  modulator->num_audio_ins_ = 0;
  modulator->num_audio_outs_ = 0;
  modulator->num_cv_outs_ = 1;
  EXPECT_FALSE (modulator->isInstrument ());
  EXPECT_FALSE (modulator->isEffect ());
  EXPECT_TRUE (modulator->isModulator ());
  EXPECT_FALSE (modulator->isMidiModifier ());

  auto midi_effect = create_test_descriptor ();
  midi_effect->num_audio_ins_ = 0;
  midi_effect->num_audio_outs_ = 0;
  midi_effect->num_midi_ins_ = 1;
  midi_effect->num_midi_outs_ = 1;
  EXPECT_FALSE (midi_effect->isInstrument ());
  EXPECT_FALSE (midi_effect->isEffect ());
  EXPECT_FALSE (midi_effect->isModulator ());
  EXPECT_TRUE (midi_effect->isMidiModifier ());
}

TEST (PluginDescriptorTest, ModulatorDetectionRequiresCvOutputs)
{
  auto without_cv_outputs = create_test_descriptor (PluginCategory::MODULATOR);
  without_cv_outputs->num_audio_ins_ = 0;
  without_cv_outputs->num_audio_outs_ = 0;
  EXPECT_FALSE (without_cv_outputs->isModulator ());

  auto with_cv_outputs = create_test_descriptor (PluginCategory::Delay);
  with_cv_outputs->num_audio_ins_ = 0;
  with_cv_outputs->num_audio_outs_ = 0;
  with_cv_outputs->num_cv_outs_ = 1;
  EXPECT_TRUE (with_cv_outputs->isModulator ());
}

TEST (PluginDescriptorTest, MidiDetectionRequiresMidiOutputs)
{
  auto without_midi_outputs = create_test_descriptor (PluginCategory::MIDI);
  without_midi_outputs->num_audio_ins_ = 0;
  without_midi_outputs->num_audio_outs_ = 0;
  without_midi_outputs->num_midi_ins_ = 1;
  EXPECT_FALSE (without_midi_outputs->isMidiModifier ());

  auto with_midi_outputs = create_test_descriptor (PluginCategory::Delay);
  with_midi_outputs->num_audio_ins_ = 0;
  with_midi_outputs->num_audio_outs_ = 0;
  with_midi_outputs->num_midi_ins_ = 1;
  with_midi_outputs->num_midi_outs_ = 1;
  EXPECT_TRUE (with_midi_outputs->isMidiModifier ());
}

TEST (PluginDescriptorTest, TypesAreNotExclusive)
{
  auto descriptor = create_test_descriptor (PluginCategory::Instrument);
  descriptor->num_audio_ins_ = 0;
  descriptor->num_midi_ins_ = 1;
  descriptor->num_midi_outs_ = 1;
  descriptor->num_cv_outs_ = 1;
  EXPECT_TRUE (descriptor->isInstrument ());
  EXPECT_TRUE (descriptor->isModulator ());
  EXPECT_TRUE (descriptor->isMidiModifier ());
}

TEST (PluginDescriptorTest, UncategorizedInstrumentRequiresNoAudioInputs)
{
  auto with_audio_inputs = create_test_descriptor ();
  with_audio_inputs->num_midi_ins_ = 1;
  EXPECT_FALSE (with_audio_inputs->isInstrument ());
  EXPECT_TRUE (with_audio_inputs->isEffect ());

  auto without_audio_inputs = create_test_descriptor ();
  without_audio_inputs->num_audio_ins_ = 0;
  without_audio_inputs->num_midi_ins_ = 1;
  EXPECT_TRUE (without_audio_inputs->isInstrument ());
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

TEST (PluginDescriptorTest, DeserializationAcceptsMissingScanTimestamps)
{
  auto original = create_test_descriptor (PluginCategory::Delay);
  original->path_or_id_ = utils::Utf8String (u8"http://example.org/plugin");
  original->unique_id_ = 12345;
  original->last_file_mod_time_ms_ = 1000;
  original->last_info_update_time_ms_ = 2000;

  nlohmann::json j = *original;
  j.erase ("lastFileModTimeMs");
  j.erase ("lastInfoUpdateTimeMs");

  PluginDescriptor deserialized;
  from_json (j, deserialized);
  EXPECT_EQ (deserialized.last_file_mod_time_ms_, 0);
  EXPECT_EQ (deserialized.last_info_update_time_ms_, 0);
  EXPECT_EQ (deserialized.name_, original->name_);
}
}
