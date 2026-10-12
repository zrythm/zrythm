// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>

#include "dsp/curve.h"
#include "dsp/timebase.h"
#include "structure/arrangement/audio_function.h"
#include "utils/audio.h"
#include "utils/enum_utils.h"
#include "utils/exceptions.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{

namespace
{

utils::audio::AudioBuffer
make_buffer (
  std::initializer_list<float> left,
  std::initializer_list<float> right)
{
  utils::audio::AudioBuffer buf{ 2, static_cast<int> (left.size ()) };
  std::ranges::copy (left, buf.getWritePointer (0));
  std::ranges::copy (right, buf.getWritePointer (1));
  return buf;
}

void
expect_sample (const utils::audio::AudioBuffer &buf, int ch, int i, float value)
{
  EXPECT_FLOAT_EQ (buf.getSample (ch, i), value)
    << "ch " << ch << " frame " << i;
}

units::sample_u64_t
whole (int n)
{
  return units::samples (static_cast<uint64_t> (n));
}

} // namespace

TEST (AudioFunctionKindTest, KindFromIdFindsEveryKind)
{
  const std::array<std::pair<std::string_view, std::string_view>, 6> id_label_pairs = {
    { { AudioFunction::Invert::kId, "Invert Polarity" },
     { AudioFunction::Reverse::kId, "Reverse" },
     { AudioFunction::NormalizePeak::kId, "Normalize Peak" },
     { AudioFunction::NormalizeRms::kId, "Normalize RMS" },
     { AudioFunction::FadeIn::kId, "Fade In" },
     { AudioFunction::FadeOut::kId, "Fade Out" } }
  };
  for (const auto &[id, label] : id_label_pairs)
    {
      const auto kind = AudioFunction::kind_from_id (id);
      ASSERT_TRUE (kind.has_value ()) << id;
      EXPECT_EQ (AudioFunction::label (*kind, false).view (), label);
    }
}

TEST (AudioFunctionKindTest, KindFromIdRejectsUnknownId)
{
  EXPECT_FALSE (AudioFunction::kind_from_id ("no-such-function").has_value ());
}

TEST (AudioFunctionKindTest, FadeParamsRoundTripThroughOptions)
{
  AudioFunction::FadeIn fade;
  const auto            applied = apply_options (
    {
      { "curveAlgorithm",
       ENUM_VALUE_TO_INT (dsp::CurveOptions::Algorithm::Exponent) },
      { "curviness",      0.25                                    }
  },
    fade, AudioFunction::FadeIn::params ());
  EXPECT_TRUE (applied.has_value ());
  EXPECT_EQ (fade.curve.algo_, dsp::CurveOptions::Algorithm::Exponent);
  EXPECT_DOUBLE_EQ (fade.curve.curviness_, 0.25);
}

TEST (AudioFunctionKindTest, FadeParamsRejectOutOfRangeCurviness)
{
  AudioFunction::FadeOut fade;
  const auto             applied = apply_options (
    {
      { "curviness", 2.0 }
  },
    fade, AudioFunction::FadeOut::params ());
  EXPECT_FALSE (applied.has_value ());
}

TEST (AudioFunctionKindTest, FadeParamsAcceptEveryCurveAlgorithm)
{
  for (
    const auto algo : magic_enum::enum_values<dsp::CurveOptions::Algorithm> ())
    {
      AudioFunction::FadeIn fade;
      const auto            applied = apply_options (
        {
          { "curveAlgorithm", ENUM_VALUE_TO_INT (algo) },
          { "curviness",      0.25                     }
      },
        fade, AudioFunction::FadeIn::params ());
      EXPECT_TRUE (applied.has_value ())
        << "algorithm " << ENUM_VALUE_TO_INT (algo);
      EXPECT_EQ (fade.curve.algo_, algo);
      EXPECT_DOUBLE_EQ (fade.curve.curviness_, 0.25);
    }
}

TEST (AudioFunctionKindTest, FadeParamsRejectCurveAlgorithmNotInChoices)
{
  AudioFunction::FadeIn fade;
  const auto            applied = apply_options (
    {
      { "curveAlgorithm", 99 }
  },
    fade, AudioFunction::FadeIn::params ());
  EXPECT_FALSE (applied.has_value ());
}

TEST (AudioFunctionKindTest, FadeParamsRejectHugeCurveAlgorithm)
{
  AudioFunction::FadeIn fade;
  const auto            applied = apply_options (
    {
      { "curveAlgorithm", 1e300 }
  },
    fade, AudioFunction::FadeIn::params ());
  EXPECT_FALSE (applied.has_value ());
}

TEST (AudioFunctionKindTest, NonFadeKindsHaveNoParams)
{
  EXPECT_TRUE (AudioFunction::Invert::params ().empty ());
  EXPECT_TRUE (AudioFunction::Reverse::params ().empty ());
  EXPECT_TRUE (AudioFunction::NormalizePeak::params ().empty ());
  EXPECT_TRUE (AudioFunction::NormalizeRms::params ().empty ());
}

TEST (AudioFunctionEntryTest, EntryJsonRoundTripsAllKinds)
{
  const std::array<AudioFunction::Entry, 6> entries = {
    AudioFunction::Invert{
                          .start = dsp::ContentTick{ units::ticks (1.5) },
                          .end = dsp::ContentTick{ units::ticks (2.5) } },
    AudioFunction::Reverse{
                          .start = dsp::ContentTick{ units::ticks (0.) },
                          .end = dsp::ContentTick{ units::ticks (10.) } },
    AudioFunction::NormalizePeak{
                          .start = dsp::ContentTick{ units::ticks (0.) },
                          .end = dsp::ContentTick{ units::ticks (4.) },
                          .gain = 2.5 },
    AudioFunction::NormalizeRms{
                          .start = dsp::ContentTick{ units::ticks (0.) },
                          .end = dsp::ContentTick{ units::ticks (4.) },
                          .gain = 0.5 },
    [] {
      AudioFunction::FadeIn fade;
      fade.start = dsp::ContentTick{ units::ticks (1.) };
      fade.end = dsp::ContentTick{ units::ticks (3.) };
      fade.curve.curviness_ = -0.5;
      fade.curve.algo_ = dsp::CurveOptions::Algorithm::Exponent;
      return fade;
                          }
    (),
    [] {
      AudioFunction::FadeOut fade;
      fade.start = dsp::ContentTick{ units::ticks (2.) };
      fade.end = dsp::ContentTick{ units::ticks (6.) };
      fade.curve.curviness_ = 0.75;
      return fade;
                          }
    (),
  };

  for (const auto &entry : entries)
    {
      nlohmann::json       json;
      AudioFunction::Entry parsed;
      AudioFunction::entry_to_json (json, entry);
      AudioFunction::entry_from_json (json, parsed);
      EXPECT_EQ (parsed, entry) << json.dump ();
    }
}

TEST (AudioFunctionEntryTest, EntryJsonRejectsUnknownKind)
{
  nlohmann::json json;
  json["kind"] = "pitch-shift";
  AudioFunction::Entry entry;
  EXPECT_THROW (AudioFunction::entry_from_json (json, entry), ZrythmException);
}

TEST (AudioFunctionBufferTest, InvertFlipsSign)
{
  auto buf = make_buffer ({ 0.5f, -0.25f }, { -1.f, 0.75f });
  AudioFunction::apply_entry_to_buffer (
    buf, AudioFunction::Invert{}, whole (0), whole (2));
  expect_sample (buf, 0, 0, -0.5f);
  expect_sample (buf, 0, 1, 0.25f);
  expect_sample (buf, 1, 0, 1.f);
  expect_sample (buf, 1, 1, -0.75f);
}

TEST (AudioFunctionBufferTest, NormalizeAppliesFrozenGain)
{
  // The gain was frozen when the entry was created; applying the entry
  // never recomputes it
  auto buf = make_buffer ({ 0.25f, -0.5f }, { -0.1f, 0.125f });
  AudioFunction::NormalizePeak entry;
  entry.gain = 2.0;
  AudioFunction::apply_entry_to_buffer (buf, entry, whole (0), whole (2));
  expect_sample (buf, 0, 0, 0.5f);
  expect_sample (buf, 0, 1, -1.f);
  expect_sample (buf, 1, 0, -0.2f);
  expect_sample (buf, 1, 1, 0.25f);
}

TEST (AudioFunctionBufferTest, FadeInLinearRunsFromZeroToOne)
{
  auto buf =
    make_buffer ({ 1.f, 1.f, 1.f, 1.f, 1.f }, { 1.f, 1.f, 1.f, 1.f, 1.f });
  AudioFunction::FadeIn entry;
  entry.curve.curviness_ = 0.0; // linear for every algorithm
  AudioFunction::apply_entry_to_buffer (buf, entry, whole (0), whole (5));
  expect_sample (buf, 0, 0, 0.f);
  expect_sample (buf, 1, 0, 0.f);
  expect_sample (buf, 0, 2, 0.5f);
  expect_sample (buf, 0, 4, 1.f);
  expect_sample (buf, 1, 4, 1.f);
}

TEST (AudioFunctionBufferTest, FadeOutLinearRunsFromOneToZero)
{
  auto buf =
    make_buffer ({ 1.f, 1.f, 1.f, 1.f, 1.f }, { 1.f, 1.f, 1.f, 1.f, 1.f });
  AudioFunction::FadeOut entry;
  entry.curve.curviness_ = 0.0;
  AudioFunction::apply_entry_to_buffer (buf, entry, whole (0), whole (5));
  expect_sample (buf, 0, 0, 1.f);
  expect_sample (buf, 1, 0, 1.f);
  expect_sample (buf, 0, 2, 0.5f);
  expect_sample (buf, 0, 4, 0.f);
  expect_sample (buf, 1, 4, 0.f);
}

TEST (AudioFunctionBufferTest, EntriesEditOnlyTheGivenRange)
{
  constexpr int kStart = 2;
  constexpr int kLength = 4;

  auto fade_buf = make_buffer (
    { 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f },
    { 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f });
  AudioFunction::FadeIn fade;
  fade.curve.curviness_ = 0.0;
  AudioFunction::apply_entry_to_buffer (
    fade_buf, fade, whole (kStart), whole (kLength));
  expect_sample (fade_buf, 0, 0, 1.f);
  expect_sample (fade_buf, 0, 1, 1.f);
  expect_sample (fade_buf, 0, 2, 0.f);
  expect_sample (fade_buf, 0, 6, 1.f);
  expect_sample (fade_buf, 0, 7, 1.f);

  auto reverse_buf = make_buffer (
    { 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f },
    { 0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f });
  AudioFunction::apply_entry_to_buffer (
    reverse_buf, AudioFunction::Reverse{}, whole (kStart), whole (kLength));
  expect_sample (reverse_buf, 0, 0, 0.f);
  expect_sample (reverse_buf, 0, 1, 1.f);
  expect_sample (reverse_buf, 0, 2, 5.f);
  expect_sample (reverse_buf, 0, 3, 4.f);
  expect_sample (reverse_buf, 0, 4, 3.f);
  expect_sample (reverse_buf, 0, 5, 2.f);
  expect_sample (reverse_buf, 0, 6, 6.f);
  expect_sample (reverse_buf, 0, 7, 7.f);
}

TEST (AudioFunctionBufferTest, OutOfBoundsRangeIsClamped)
{
  auto buf = make_buffer ({ 1.f, 1.f, 1.f, 1.f }, { 1.f, 1.f, 1.f, 1.f });
  // Start beyond the buffer end: nothing to edit
  AudioFunction::apply_entry_to_buffer (
    buf, AudioFunction::Invert{}, whole (10), whole (2));
  // Length overrunning the buffer end is clamped to the remaining frames
  AudioFunction::apply_entry_to_buffer (
    buf, AudioFunction::Invert{}, whole (2), whole (100));
  expect_sample (buf, 0, 0, 1.f);
  expect_sample (buf, 0, 1, 1.f);
  expect_sample (buf, 0, 2, -1.f);
  expect_sample (buf, 0, 3, -1.f);
}

class AudioFunctionClipTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    // A clip without an audio source object attached
    clip = std::make_unique<AudioClip> (*tempo_map_wrapper, registry, nullptr);
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  utils::ObjectRegistry                 registry;
  std::unique_ptr<AudioClip>            clip;
};

TEST_F (AudioFunctionClipTest, ContentWindowOfSourcelessClipIsEmpty)
{
  const auto window = AudioFunction::audio_clip_content_window (*clip);
  EXPECT_EQ (window.getNumSamples (), 0);
}

TEST_F (AudioFunctionClipTest, NormalizeGainOfSourcelessClipIsUnity)
{
  const auto gain = AudioFunction::normalize_gain (
    *clip, dsp::ContentTick{ units::ticks (0.) },
    dsp::ContentTick{ units::ticks (1.) }, false);
  EXPECT_DOUBLE_EQ (gain, 1.0);
}

} // namespace zrythm::structure::arrangement
