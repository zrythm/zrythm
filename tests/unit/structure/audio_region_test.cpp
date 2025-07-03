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
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0);
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
    region->regionMixin ()->bounds ()->length ()->setSamples (500);
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
  EXPECT_NE (region->regionMixin (), nullptr);
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

// Test fill_stereo_ports
TEST_F (AudioRegionTest, FillStereoPorts)
{
  // Setup output buffers
  constexpr nframes_t nframes = 10;
  std::vector<float>  left_buffer (nframes, 0.0f);
  std::vector<float>  right_buffer (nframes, 0.0f);

  // Create process time info
  EngineProcessTimeInfo time_nfo;
  // Start at region position + builtin fade frames to test normal operation
  time_nfo.g_start_frame_ = 1000 + AudioRegion::BUILTIN_FADE_FRAMES;
  time_nfo.g_start_frame_w_offset_ = 1000 + AudioRegion::BUILTIN_FADE_FRAMES;
  time_nfo.local_offset_ = 0;
  time_nfo.nframes_ = nframes;

  region->prepare_to_play (1024, 44100.0);

  // Call fill_stereo_ports
  region->fill_stereo_ports (time_nfo, { left_buffer, right_buffer });

  // Verify output
  for (nframes_t i = 0; i < nframes; i++)
    {
      // Check left channel (0.5 * gain 1.0 = 0.5)
      EXPECT_FLOAT_EQ (left_buffer[i], 0.5f);

      // Check right channel (-0.5 * gain 1.0 = -0.5)
      EXPECT_FLOAT_EQ (right_buffer[i], -0.5f);
    }

  // Test with gain
  region->setGain (0.5f);
  std::ranges::fill (left_buffer, 0.0f);
  std::ranges::fill (right_buffer, 0.0f);

  region->fill_stereo_ports (time_nfo, { left_buffer, right_buffer });

  for (nframes_t i = 0; i < nframes; i++)
    {
      EXPECT_FLOAT_EQ (left_buffer[i], 0.25f);   // 0.5 * 0.5
      EXPECT_FLOAT_EQ (right_buffer[i], -0.25f); // -0.5 * 0.5
    }

  // Test built-in fade in
  region->setGain (1.0f);
  std::ranges::fill (left_buffer, 0.0f);
  std::ranges::fill (right_buffer, 0.0f);

  // Set position to beginning of region to enable built-in fade in
  time_nfo.g_start_frame_ = 1000;
  time_nfo.g_start_frame_w_offset_ = 1000;

  region->fill_stereo_ports (time_nfo, { left_buffer, right_buffer });

  // Samples should be faded (0.5 * builtin fade factor)
  for (nframes_t i = 0; i < nframes; i++)
    {
      const float expected_fade =
        0.5f * (static_cast<float> (i) / AudioRegion::BUILTIN_FADE_FRAMES);
      EXPECT_FLOAT_EQ (left_buffer[i], expected_fade);
      EXPECT_FLOAT_EQ (right_buffer[i], -expected_fade);
    }

  // Test built-in fade out
  std::ranges::fill (left_buffer, 0.0f);
  std::ranges::fill (right_buffer, 0.0f);

  // Set position to near end of region to enable built-in fade in
  const auto end_pos_frames =
    region->regionMixin ()->bounds ()->get_end_position_samples (true);
  time_nfo.g_start_frame_ = end_pos_frames - nframes;
  time_nfo.g_start_frame_w_offset_ = end_pos_frames - nframes;

  region->fill_stereo_ports (time_nfo, { left_buffer, right_buffer });

  // Samples should be faded (0.5 * builtin fade factor)
  for (nframes_t i = 0; i < nframes; i++)
    {
      const float expected_fade =
        0.5f
        * (static_cast<float> (nframes - i) / AudioRegion::BUILTIN_FADE_FRAMES);
      EXPECT_FLOAT_EQ (left_buffer[i], expected_fade);
      EXPECT_FLOAT_EQ (right_buffer[i], -expected_fade);
    }

  // TODO: Test custom fade range
  region->fadeRange ()->startOffset ()->setSamples (100);
  region->fadeRange ()->endOffset ()->setSamples (100);
}

// Test serialization/deserialization
TEST_F (AudioRegionTest, Serialization)
{
  // Set initial state
  region->position ()->setSamples (1000);
  region->setGain (0.8f);
  region->setMusicalMode (AudioRegion::MusicalMode::On);
  region->regionMixin ()->bounds ()->length ()->setSamples (5000);

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
  EXPECT_EQ (new_region->regionMixin ()->bounds ()->length ()->samples (), 5000);
}

// Test copying
TEST_F (AudioRegionTest, Copying)
{
  // Set initial state
  region->position ()->setSamples (2000);
  region->setGain (1.2f);
  region->setMusicalMode (AudioRegion::MusicalMode::Off);
  region->regionMixin ()->bounds ()->length ()->setSamples (3000);

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
  EXPECT_EQ (target->regionMixin ()->bounds ()->length ()->samples (), 3000);
}

} // namespace zrythm::structure::arrangement
