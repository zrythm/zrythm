// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <memory>

#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object_all.h"

#include "helpers/mock_qobject.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace zrythm::structure::arrangement;

namespace zrythm::structure::arrangement
{
class AudioRegionTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    parent = std::make_unique<MockQObject> ();
    musical_mode_getter = [&] () { return global_musical_mode_enabled; };

    // Create a mock audio source
    auto sample_buffer = std::make_unique<utils::audio::AudioBuffer> (2, 1024);

    // Fill with test data: left channel = 0.5, right channel = -0.5
    for (int i = 0; i < sample_buffer->getNumSamples (); i++)
      {
        sample_buffer->setSample (0, i, 0.5f);
        sample_buffer->setSample (1, i, -0.5f);
      }

    auto source_ref = file_audio_source_registry.create_object<
      dsp::FileAudioSource> (
      *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32, 44100, 120.f,
      u8"DummySource");
    auto audio_source_object_ref = registry.create_object<AudioSourceObject> (
      *tempo_map, file_audio_source_registry, source_ref);

    region = std::make_unique<AudioRegion> (
      *tempo_map, registry, file_audio_source_registry, musical_mode_getter,
      parent.get ());

    region->set_source (audio_source_object_ref);

    // Set region position and length
    region->position ()->setSamples (1000);
    region->bounds ()->length ()->setSamples (500);
  }

  std::unique_ptr<dsp::TempoMap>       tempo_map;
  std::unique_ptr<MockQObject>         parent;
  bool                                 global_musical_mode_enabled{ true };
  AudioRegion::GlobalMusicalModeGetter musical_mode_getter;
  ArrangerObjectRegistry               registry;
  dsp::FileAudioSourceRegistry         file_audio_source_registry;
  std::unique_ptr<AudioRegion>         region;
};

// Test initial state
TEST_F (AudioRegionTest, InitialState)
{
  EXPECT_EQ (region->type (), ArrangerObject::Type::AudioRegion);
  EXPECT_EQ (region->position ()->samples (), 1000);
  EXPECT_EQ (region->gain (), 1.0f);
  EXPECT_EQ (region->musicalMode (), AudioRegion::MusicalMode::Inherit);
  EXPECT_TRUE (region->effectivelyInMusicalMode ());
  EXPECT_NE (region->bounds (), nullptr);
  EXPECT_NE (region->loopRange (), nullptr);
  EXPECT_NE (region->name (), nullptr);
  EXPECT_NE (region->color (), nullptr);
  EXPECT_NE (region->mute (), nullptr);
  EXPECT_NE (region->fadeRange (), nullptr);
}

// Test musical mode property
TEST_F (AudioRegionTest, MusicalModeProperty)
{
  // Set to Off
  region->setMusicalMode (AudioRegion::MusicalMode::Off);
  EXPECT_EQ (region->musicalMode (), AudioRegion::MusicalMode::Off);
  EXPECT_FALSE (region->effectivelyInMusicalMode ());

  // Set to On
  region->setMusicalMode (AudioRegion::MusicalMode::On);
  EXPECT_EQ (region->musicalMode (), AudioRegion::MusicalMode::On);
  EXPECT_TRUE (region->effectivelyInMusicalMode ());

  // Test signal emission
  testing::MockFunction<void (AudioRegion::MusicalMode)> mockModeChanged;
  QObject::connect (
    region.get (), &AudioRegion::musicalModeChanged, parent.get (),
    mockModeChanged.AsStdFunction ());

  EXPECT_CALL (mockModeChanged, Call (AudioRegion::MusicalMode::Inherit))
    .Times (1);
  region->setMusicalMode (AudioRegion::MusicalMode::Inherit);
}

// Test gain property
TEST_F (AudioRegionTest, GainProperty)
{
  region->setGain (0.5f);
  EXPECT_FLOAT_EQ (region->gain (), 0.5f);

  // Test clamping
  region->setGain (-0.5f);
  EXPECT_FLOAT_EQ (region->gain (), 0.0f);

  region->setGain (2.5f);
  EXPECT_FLOAT_EQ (region->gain (), 2.0f);
}

// Test effectivelyInMusicalMode under different conditions
TEST_F (AudioRegionTest, EffectivelyInMusicalMode)
{
  // Inherit mode with global true
  region->setMusicalMode (AudioRegion::MusicalMode::Inherit);
  global_musical_mode_enabled = true;
  EXPECT_TRUE (region->effectivelyInMusicalMode ());

  // Inherit mode with global false
  global_musical_mode_enabled = false;
  EXPECT_FALSE (region->effectivelyInMusicalMode ());

  // Explicitly on
  region->setMusicalMode (AudioRegion::MusicalMode::On);
  EXPECT_TRUE (region->effectivelyInMusicalMode ());

  // Explicitly off
  region->setMusicalMode (AudioRegion::MusicalMode::Off);
  EXPECT_FALSE (region->effectivelyInMusicalMode ());
}

// Test serialization/deserialization
TEST_F (AudioRegionTest, Serialization)
{
  // Set initial state
  region->position ()->setSamples (1000);
  region->setGain (0.8f);
  region->setMusicalMode (AudioRegion::MusicalMode::On);
  region->bounds ()->length ()->setSamples (5000);

  // Serialize
  nlohmann::json j;
  to_json (j, *region);

  // Create new region
  auto new_region = std::make_unique<AudioRegion> (
    *tempo_map, registry, file_audio_source_registry, musical_mode_getter,
    parent.get ());
  from_json (j, *new_region);

  // Verify state
  EXPECT_EQ (new_region->position ()->samples (), 1000);
  EXPECT_FLOAT_EQ (new_region->gain (), 0.8f);
  EXPECT_EQ (new_region->musicalMode (), AudioRegion::MusicalMode::On);
  EXPECT_EQ (new_region->bounds ()->length ()->samples (), 5000);
}

// Test copying
TEST_F (AudioRegionTest, Copying)
{
  // Set initial state
  region->position ()->setSamples (2000);
  region->setGain (1.2f);
  region->setMusicalMode (AudioRegion::MusicalMode::Off);
  region->bounds ()->length ()->setSamples (3000);

  // Create target
  auto target = std::make_unique<AudioRegion> (
    *tempo_map, registry, file_audio_source_registry, musical_mode_getter,
    parent.get ());

  // Copy
  init_from (*target, *region, utils::ObjectCloneType::Snapshot);

  // Verify state
  EXPECT_EQ (target->position ()->samples (), 2000);
  EXPECT_FLOAT_EQ (target->gain (), 1.2f);
  EXPECT_EQ (target->musicalMode (), AudioRegion::MusicalMode::Off);
  EXPECT_EQ (target->bounds ()->length ()->samples (), 3000);
}

} // namespace zrythm::structure::arrangement
