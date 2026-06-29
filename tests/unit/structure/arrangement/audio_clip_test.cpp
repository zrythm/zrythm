// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <memory>

#include "dsp/tempo_map.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "dsp/timebase.h"
#include "dsp/timestretch_engine.h"
#include "structure/arrangement/arranger_object_all.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/mock_qobject.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace zrythm::structure::arrangement;

namespace zrythm::structure::arrangement
{
class AudioClipTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    parent = std::make_unique<MockQObject> ();

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

    clip =
      std::make_unique<AudioClip> (*tempo_map_wrapper, registry, parent.get ());

    clip->set_source (audio_source_object_ref);

    // Set clip position and length
    clip->position ()->setTicks (1000);
    clip->length ()->setTicks (500);
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  std::unique_ptr<MockQObject>          parent;
  utils::ObjectRegistry                 registry;
  std::unique_ptr<AudioClip>            clip;
};

// Test initial state
TEST_F (AudioClipTest, InitialState)
{
  EXPECT_EQ (clip->type (), ArrangerObject::Type::AudioClip);
  EXPECT_EQ (clip->position ()->ticks (), 1000);
  EXPECT_EQ (clip->gain (), 1.0f);
  // Default effective timebase (no source, no override) is Musical.
  EXPECT_EQ (
    clip->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Musical);
  EXPECT_EQ (
    clip->stretchAlgorithm (), dsp::StretchOptions::Algorithm::Polyphonic);
  EXPECT_NE (clip->length (), nullptr);
  EXPECT_NE (clip->loopStartPosition (), nullptr);
  EXPECT_NE (clip->name (), nullptr);
  EXPECT_NE (clip->color (), nullptr);
  EXPECT_NE (clip->mute (), nullptr);
  EXPECT_NE (clip->fadeRange (), nullptr);
}

// Test stretch algorithm property
TEST_F (AudioClipTest, StretchAlgorithmProperty)
{
  // Set to Monophonic
  clip->setStretchAlgorithm (dsp::StretchOptions::Algorithm::Monophonic);
  EXPECT_EQ (
    clip->stretchAlgorithm (), dsp::StretchOptions::Algorithm::Monophonic);

  // Set to Beats
  clip->setStretchAlgorithm (dsp::StretchOptions::Algorithm::Beats);
  EXPECT_EQ (clip->stretchAlgorithm (), dsp::StretchOptions::Algorithm::Beats);

  // Test signal emission
  testing::MockFunction<void (dsp::StretchOptions::Algorithm)>
    mockAlgorithmChanged;
  QObject::connect (
    clip.get (), &AudioClip::stretchAlgorithmChanged, parent.get (),
    mockAlgorithmChanged.AsStdFunction ());

  EXPECT_CALL (
    mockAlgorithmChanged, Call (dsp::StretchOptions::Algorithm::Repitch))
    .Times (1);
  clip->setStretchAlgorithm (dsp::StretchOptions::Algorithm::Repitch);
}

// Test sourceBpm property
TEST_F (AudioClipTest, SourceBpmProperty)
{
  // Source was created with 120 BPM in SetUp()
  EXPECT_DOUBLE_EQ (clip->sourceBpm (), 120.0);
}

// Test gain property
TEST_F (AudioClipTest, GainProperty)
{
  clip->setGain (0.5f);
  EXPECT_FLOAT_EQ (clip->gain (), 0.5f);

  // Test clamping
  clip->setGain (-0.5f);
  EXPECT_FLOAT_EQ (clip->gain (), 0.0f);

  clip->setGain (2.5f);
  EXPECT_FLOAT_EQ (clip->gain (), 2.0f);
}

// Test effective timebase resolution via the timebase policy
TEST_F (AudioClipTest, EffectiveTimebase)
{
  // Default effective timebase (no source, no override) is Musical.
  EXPECT_EQ (
    clip->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Musical);

  // Override to Absolute.
  clip->timebaseProvider ()->setOverride (dsp::Timebase::Absolute);
  EXPECT_EQ (
    clip->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Absolute);
  EXPECT_EQ (
    clip->timebaseProvider ()->overrideValue (), dsp::Timebase::Absolute);

  // Clear override returns to default (Musical).
  clip->timebaseProvider ()->clearOverride ();
  EXPECT_EQ (
    clip->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Musical);
  EXPECT_FALSE (clip->timebaseProvider ()->overrideValue ().has_value ());

  // Explicitly override to Musical.
  clip->timebaseProvider ()->setOverride (dsp::Timebase::Musical);
  EXPECT_EQ (
    clip->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Musical);
}

// Test serialization/deserialization
TEST_F (AudioClipTest, Serialization)
{
  // Set initial state
  clip->position ()->setTicks (1000);
  clip->setGain (0.8f);
  clip->setStretchAlgorithm (dsp::StretchOptions::Algorithm::Beats);
  clip->length ()->setTicks (5000);

  // Serialize
  nlohmann::json j;
  to_json (j, *clip);

  // Create new clip
  auto new_clip =
    std::make_unique<AudioClip> (*tempo_map_wrapper, registry, parent.get ());
  from_json (j, *new_clip);

  // Verify state
  EXPECT_EQ (new_clip->position ()->ticks (), 1000);
  EXPECT_FLOAT_EQ (new_clip->gain (), 0.8f);
  EXPECT_EQ (
    new_clip->stretchAlgorithm (), dsp::StretchOptions::Algorithm::Beats);
  EXPECT_EQ (new_clip->length ()->ticks (), 5000);
}

// Test copying
TEST_F (AudioClipTest, Copying)
{
  // Set initial state
  clip->position ()->setTicks (2000);
  clip->setGain (1.2f);
  clip->setStretchAlgorithm (dsp::StretchOptions::Algorithm::Monophonic);
  clip->length ()->setTicks (3000);

  // Create target
  auto target =
    std::make_unique<AudioClip> (*tempo_map_wrapper, registry, parent.get ());

  // Copy
  init_from (*target, *clip, utils::ObjectCloneType::Snapshot);

  // Verify state
  EXPECT_EQ (target->position ()->ticks (), 2000);
  EXPECT_FLOAT_EQ (target->gain (), 1.2f);
  EXPECT_EQ (
    target->stretchAlgorithm (), dsp::StretchOptions::Algorithm::Monophonic);
  EXPECT_EQ (target->length ()->ticks (), 3000);
}

// ---------------------------------------------------------------------------
// Musical-mode length format (source-BPM-based, absolute sync)
// ---------------------------------------------------------------------------

class AudioClipMusicalModeLengthTest : public ::testing::Test
{
protected:
  static constexpr double  kSr = 44100.0;
  static constexpr int64_t kFrames = 44100; // 1 second of audio

  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (kSr));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    parent = std::make_unique<MockQObject> ();
  }

  /// Builds a full-clip AudioClip whose clip has the given source BPM. The
  /// length is whatever init_length_from_clip() sets (not overridden), so the
  /// musical-mode format behaviour is observable.
  std::unique_ptr<AudioClip> make_clip (units::bpm_t source_bpm)
  {
    utils::audio::AudioBuffer buf (2, static_cast<int> (kFrames));
    auto source_ref = utils::create_object<dsp::FileAudioSource> (
      registry, buf, utils::audio::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), source_bpm, u8"clip");
    auto audio_source_object_ref = utils::create_object<AudioSourceObject> (
      registry, *tempo_map_wrapper, registry, source_ref);
    auto r =
      std::make_unique<AudioClip> (*tempo_map_wrapper, registry, parent.get ());
    r->set_source (audio_source_object_ref); // triggers init_length_from_clip
    return r;
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  std::unique_ptr<MockQObject>          parent;
  utils::ObjectRegistry                 registry;
};

// Known source BPM => length in musical ticks derived from the SOURCE bpm.
// 1 s @ 100 BPM => 1600 ticks (NOT 1920, which would be 1 s at the project
// tempo 120 — confirms source-BPM derivation).
TEST_F (AudioClipMusicalModeLengthTest, MusicalOnUsesSourceBpmLength)
{
  auto         clip = make_clip (units::bpm (100.0));
  const auto * len = clip->length ();
  EXPECT_NEAR (len->ticks (), 1600.0, 1.0);
}

// Toggling the timebase override between Musical and Absolute does not affect
// the clip length. Length is always stored in musical ticks; the timebase
// only reconfigures the ContentTimeWarp.
TEST_F (AudioClipMusicalModeLengthTest, ToggleRoundTripHasNoDrift)
{
  auto         clip = make_clip (units::bpm (100.0));
  const double ticks_before = clip->length ()->ticks ();
  ASSERT_NEAR (ticks_before, 1600.0, 1.0);

  clip->timebaseProvider ()->setOverride (dsp::Timebase::Absolute);
  EXPECT_EQ (
    clip->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Absolute);
  EXPECT_NEAR (clip->length ()->ticks (), ticks_before, 1.0);

  clip->timebaseProvider ()->setOverride (dsp::Timebase::Musical);
  EXPECT_EQ (
    clip->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Musical);
  EXPECT_NEAR (clip->length ()->ticks (), ticks_before, 1.0);
}

// Unknown source BPM (0) => falls back to project tempo (120) for tick
// computation. Length is still Musical, ContentTimeWarp is Project mode.
TEST_F (AudioClipMusicalModeLengthTest, UnknownSourceBpmFallsBackToProjectTempo)
{
  auto clip = make_clip (units::bpm (0.0));
  // 1 s @ 120 BPM => 1920 ticks
  EXPECT_NEAR (clip->length ()->ticks (), 1920.0, 1.0);
}

// Full-clip: loop_end tracks the length. All positions are Musical ticks
// derived from source BPM — no format switching on timebase toggle.
TEST_F (AudioClipMusicalModeLengthTest, FullClipLoopEndAlwaysMusical)
{
  auto   clip = make_clip (units::bpm (100.0));
  auto * loop_end = clip->loopEndPosition ();
  EXPECT_NEAR (loop_end->ticks (), 1600.0, 1.0);

  // Toggling timebase does not change loop-end value.
  clip->timebaseProvider ()->setOverride (dsp::Timebase::Absolute);
  EXPECT_NEAR (loop_end->ticks (), 1600.0, 1.0);
}

// A custom (detached) loop range is preserved across a timebase toggle: all
// positions are always Musical ticks, so no conversion happens.
TEST_F (AudioClipMusicalModeLengthTest, DetachedLoopPointsPreservedOnToggle)
{
  auto clip = make_clip (units::bpm (100.0));

  // Detach loop points from the bounds and set a custom loop.
  clip->setTrackBounds (false);
  clip->loopStartPosition ()->setTicks (400.0);
  clip->loopEndPosition ()->setTicks (1200.0);

  // Toggle to Absolute timebase: ticks unchanged.
  clip->timebaseProvider ()->setOverride (dsp::Timebase::Absolute);
  EXPECT_NEAR (clip->loopStartPosition ()->ticks (), 400.0, 1.0);
  EXPECT_NEAR (clip->loopEndPosition ()->ticks (), 1200.0, 1.0);

  // Toggle back to Musical: still unchanged.
  clip->timebaseProvider ()->setOverride (dsp::Timebase::Musical);
  EXPECT_NEAR (clip->loopStartPosition ()->ticks (), 400.0, 1.0);
  EXPECT_NEAR (clip->loopEndPosition ()->ticks (), 1200.0, 1.0);
}

} // namespace zrythm::structure::arrangement
