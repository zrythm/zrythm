// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <memory>

#include "dsp/tempo_map.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/arranger_object_all.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/in_memory_settings_backend.h"
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
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    parent = std::make_unique<MockQObject> ();
    app_settings = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());
    app_settings->set_musicalMode (true);

    // Create a mock audio source
    auto sample_buffer = std::make_unique<utils::audio::AudioBuffer> (2, 1024);

    // Fill with test data: left channel = 0.5, right channel = -0.5
    for (int i = 0; i < sample_buffer->getNumSamples (); i++)
      {
        sample_buffer->setSample (0, i, 0.5f);
        sample_buffer->setSample (1, i, -0.5f);
      }

    auto source_ref = utils::create_object<dsp::FileAudioSource> (
      registry, *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), units::bpm (120.0), u8"DummySource");
    auto audio_source_object_ref = utils::create_object<AudioSourceObject> (
      registry, *tempo_map_wrapper, registry, source_ref);

    region = std::make_unique<AudioRegion> (
      *tempo_map_wrapper, registry, *app_settings, parent.get ());

    region->set_source (audio_source_object_ref);

    // Set region position and length
    region->position ()->setSamples (1000);
    region->bounds ()->length ()->setSamples (500);
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  std::unique_ptr<MockQObject>          parent;
  std::unique_ptr<utils::AppSettings>   app_settings;
  utils::ObjectRegistry                 registry;
  std::unique_ptr<AudioRegion>          region;
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

// Test sourceBpm property
TEST_F (AudioRegionTest, SourceBpmProperty)
{
  // Source was created with 120 BPM in SetUp()
  EXPECT_DOUBLE_EQ (region->sourceBpm (), 120.0);
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
  app_settings->set_musicalMode (true);
  EXPECT_TRUE (region->effectivelyInMusicalMode ());

  app_settings->set_musicalMode (false);
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
  // Switch the length to Absolute time to verify the format (not just the
  // default Musical) survives serialization.
  region->bounds ()->length ()->setMode (
    dsp::AtomicPosition::TimeFormat::Absolute);

  // Serialize
  nlohmann::json j;
  to_json (j, *region);

  // Create new region
  auto new_region = std::make_unique<AudioRegion> (
    *tempo_map_wrapper, registry, *app_settings, parent.get ());
  from_json (j, *new_region);

  // Verify state
  EXPECT_EQ (new_region->position ()->samples (), 1000);
  EXPECT_FLOAT_EQ (new_region->gain (), 0.8f);
  EXPECT_EQ (new_region->musicalMode (), AudioRegion::MusicalMode::On);
  EXPECT_EQ (new_region->bounds ()->length ()->samples (), 5000);
  EXPECT_EQ (
    new_region->bounds ()->length ()->mode (),
    dsp::AtomicPosition::TimeFormat::Absolute);
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
    *tempo_map_wrapper, registry, *app_settings, parent.get ());

  // Copy
  init_from (*target, *region, utils::ObjectCloneType::Snapshot);

  // Verify state
  EXPECT_EQ (target->position ()->samples (), 2000);
  EXPECT_FLOAT_EQ (target->gain (), 1.2f);
  EXPECT_EQ (target->musicalMode (), AudioRegion::MusicalMode::Off);
  EXPECT_EQ (target->bounds ()->length ()->samples (), 3000);
}

// ---------------------------------------------------------------------------
// Musical-mode length format (source-BPM-based, absolute sync)
// ---------------------------------------------------------------------------

class AudioRegionMusicalModeLengthTest : public ::testing::Test
{
protected:
  static constexpr double  kSr = 44100.0;
  static constexpr int64_t kFrames = 44100; // 1 second of audio

  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (kSr));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    parent = std::make_unique<MockQObject> ();
    app_settings = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());
    app_settings->set_musicalMode (true);
  }

  /// Builds a full-clip AudioRegion whose clip has the given source BPM. The
  /// length is whatever init_length_from_clip() sets (not overridden), so the
  /// musical-mode format behaviour is observable.
  std::unique_ptr<AudioRegion> make_region (units::bpm_t source_bpm)
  {
    utils::audio::AudioBuffer buf (2, static_cast<int> (kFrames));
    auto source_ref = utils::create_object<dsp::FileAudioSource> (
      registry, buf, utils::audio::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), source_bpm, u8"clip");
    auto audio_source_object_ref = utils::create_object<AudioSourceObject> (
      registry, *tempo_map_wrapper, registry, source_ref);
    auto r = std::make_unique<AudioRegion> (
      *tempo_map_wrapper, registry, *app_settings, parent.get ());
    r->set_source (audio_source_object_ref); // triggers init_length_from_clip
    return r;
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  std::unique_ptr<MockQObject>          parent;
  std::unique_ptr<utils::AppSettings>   app_settings;
  utils::ObjectRegistry                 registry;
};

// Musical mode on + known source BPM => length in musical ticks derived from
// the SOURCE bpm. 1 s @ 100 BPM => 1600 ticks (NOT 1920, which would be 1 s at
// the project tempo 120 — confirms source-BPM derivation).
TEST_F (AudioRegionMusicalModeLengthTest, MusicalOnUsesSourceBpmLength)
{
  app_settings->set_musicalMode (true);
  auto         region = make_region (units::bpm (100.0));
  const auto * len = region->bounds ()->length ();
  EXPECT_EQ (len->mode (), dsp::AtomicPosition::TimeFormat::Musical);
  EXPECT_NEAR (len->ticks (), 1600.0, 1.0);
}

// On -> Off -> On returns to the original musical length with no drift, even
// though the project tempo (120) != source BPM (100). All positions are always
// Musical now; toggling only reconfigures the ContentTimeWarp.
TEST_F (AudioRegionMusicalModeLengthTest, ToggleRoundTripHasNoDrift)
{
  app_settings->set_musicalMode (true);
  auto region = make_region (units::bpm (100.0));
  region->setMusicalMode (AudioRegion::MusicalMode::On);
  const double ticks_before = region->bounds ()->length ()->ticks ();
  ASSERT_NEAR (ticks_before, 1600.0, 1.0);

  region->setMusicalMode (AudioRegion::MusicalMode::Off);
  EXPECT_EQ (
    region->bounds ()->length ()->mode (),
    dsp::AtomicPosition::TimeFormat::Musical);

  region->setMusicalMode (AudioRegion::MusicalMode::On);
  EXPECT_NEAR (region->bounds ()->length ()->ticks (), ticks_before, 1.0);
}

// Unknown source BPM (0) => falls back to project tempo (120) for tick
// computation. Length is still Musical, ContentTimeWarp is Project mode.
TEST_F (AudioRegionMusicalModeLengthTest, UnknownSourceBpmFallsBackToProjectTempo)
{
  app_settings->set_musicalMode (true);
  auto region = make_region (units::bpm (0.0));
  EXPECT_EQ (
    region->bounds ()->length ()->mode (),
    dsp::AtomicPosition::TimeFormat::Musical);
  // 1 s @ 120 BPM => 1920 ticks
  EXPECT_NEAR (region->bounds ()->length ()->ticks (), 1920.0, 1.0);
}

// Full-clip: loop_end tracks the length. All positions are Musical ticks
// derived from source BPM — no format switching on musical-mode toggle.
TEST_F (AudioRegionMusicalModeLengthTest, FullClipLoopEndAlwaysMusical)
{
  app_settings->set_musicalMode (true);
  auto   region = make_region (units::bpm (100.0));
  auto * loop_end = region->loopRange ()->loopEndPosition ();
  EXPECT_EQ (loop_end->mode (), dsp::AtomicPosition::TimeFormat::Musical);
  EXPECT_NEAR (loop_end->ticks (), 1600.0, 1.0);

  region->setMusicalMode (AudioRegion::MusicalMode::Off);
  EXPECT_EQ (loop_end->mode (), dsp::AtomicPosition::TimeFormat::Musical);
  EXPECT_NEAR (loop_end->ticks (), 1600.0, 1.0);
}

// A custom (detached) loop range is preserved across a musical-mode toggle:
// all positions are always Musical ticks, so no conversion happens.
TEST_F (AudioRegionMusicalModeLengthTest, DetachedLoopPointsPreservedOnToggle)
{
  app_settings->set_musicalMode (true);
  auto region = make_region (units::bpm (100.0));

  // Detach loop points from the bounds and set a custom loop.
  region->loopRange ()->setTrackBounds (false);
  region->loopRange ()->loopStartPosition ()->setTicks (400.0);
  region->loopRange ()->loopEndPosition ()->setTicks (1200.0);

  // Toggle to non-musical: ticks unchanged, format unchanged.
  region->setMusicalMode (AudioRegion::MusicalMode::Off);
  EXPECT_NEAR (region->loopRange ()->loopStartPosition ()->ticks (), 400.0, 1.0);
  EXPECT_NEAR (region->loopRange ()->loopEndPosition ()->ticks (), 1200.0, 1.0);

  // Toggle back: still unchanged.
  region->setMusicalMode (AudioRegion::MusicalMode::On);
  EXPECT_NEAR (region->loopRange ()->loopStartPosition ()->ticks (), 400.0, 1.0);
  EXPECT_NEAR (region->loopRange ()->loopEndPosition ()->ticks (), 1200.0, 1.0);
}

} // namespace zrythm::structure::arrangement
