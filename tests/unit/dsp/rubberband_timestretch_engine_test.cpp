// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>
#include <cstdint>

#include "dsp/rubberband_timestretch_engine.h"
#include "dsp/time_warp_map.h"
#include "dsp/timestretch_engine.h"
#include "utils/audio.h"
#include "utils/units.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

namespace
{
constexpr double kSampleRateHz = 44100.0;

/// A mono→stereo sine buffer at a fixed frequency.
utils::audio::AudioBuffer
make_sine (int channels, int64_t frames, double freq)
{
  utils::audio::AudioBuffer buf (channels, static_cast<int> (frames));
  for (int c = 0; c < channels; ++c)
    for (int64_t i = 0; i < frames; ++i)
      buf.setSample (
        c, static_cast<int> (i),
        static_cast<float> (std::sin (
          2.0 * std::numbers::pi * freq * static_cast<double> (i)
          / kSampleRateHz)));
  return buf;
}

TimeWarpMap
make_constant_warp (int64_t source, int64_t output)
{
  TimeWarpMap m;
  m.source_length = units::samples (source);
  m.output_length = units::samples (output);
  m.anchors.push_back ({ units::samples (0), units::samples (0) });
  m.anchors.push_back ({ units::samples (source), units::samples (output) });
  return m;
}

/// A non-linear (quadratic-ish) warp emulating a tempo ramp.
TimeWarpMap
make_ramp_warp (int64_t source)
{
  const int64_t output = source * 3 / 2;
  TimeWarpMap   m;
  m.source_length = units::samples (source);
  m.output_length = units::samples (output);
  constexpr int steps = 20;
  for (int i = 0; i <= steps; ++i)
    {
      const double f = static_cast<double> (i) / steps;
      const auto   sf =
        static_cast<int64_t> (std::llround (f * static_cast<double> (source)));
      // output(t) grows as t^1.5 — monotonic, non-linear, like a tempo ramp.
      const auto of = static_cast<int64_t> (
        std::llround (static_cast<double> (output) * std::pow (f, 1.5)));
      m.anchors.push_back ({ units::samples (sf), units::samples (of) });
    }
  return m;
}

double
rms (const utils::audio::AudioBuffer &buf)
{
  double sum = 0.0;
  for (int c = 0; c < buf.getNumChannels (); ++c)
    for (int i = 0; i < buf.getNumSamples (); ++i)
      {
        const double s = buf.getSample (c, i);
        sum += s * s;
      }
  return std::sqrt (
    sum / static_cast<double> (buf.getNumChannels () * buf.getNumSamples ()));
}
} // namespace

TEST (TimeStretchEngineFactoryTest, ListsAndCreatesRubberBand)
{
  const auto engines = available_timestretch_engines ();
  ASSERT_FALSE (engines.empty ());
  EXPECT_EQ (engines.front ().id, TimeStretchEngineId::RubberBand);

  auto engine = create_timestretch_engine (
    TimeStretchEngineId::RubberBand, units::sample_rate (44100));
  ASSERT_NE (engine, nullptr);
  EXPECT_EQ (engine->id (), "rubberband");
  EXPECT_TRUE (engine->supports (StretchOptions::PitchMode::Preserve));
  EXPECT_FALSE (engine->supports (StretchOptions::PitchMode::Repitch));
}

TEST (RubberBandTimeStretchEngineTest, IdentityPreservesLengthAndEnergy)
{
  constexpr int64_t kFrames = 4410; // 0.1 s
  const auto        input = make_sine (/*channels=*/2, kFrames, /*freq=*/440.0);
  const auto        warp = make_constant_warp (kFrames, kFrames);
  ASSERT_TRUE (warp.is_valid ());

  auto engine = create_default_timestretch_engine (
    StretchOptions{}, units::sample_rate (44100));
  const auto out = engine->stretch (input, warp, StretchOptions{});

  // Exact length contract.
  EXPECT_EQ (out.getNumSamples (), static_cast<int> (kFrames));
  EXPECT_EQ (out.getNumChannels (), 2);
  // Energy is roughly preserved at ratio 1 (phase-vocoder introduces small
  // differences, so compare RMS, not sample-by-sample).
  EXPECT_NEAR (rms (out), rms (input), 0.05);
}

TEST (RubberBandTimeStretchEngineTest, ConstantRatioTwoDoublesLength)
{
  constexpr int64_t kFrames = 4410;
  const auto        input = make_sine (2, kFrames, 440.0);
  const auto        warp = make_constant_warp (kFrames, 2 * kFrames);
  ASSERT_TRUE (warp.is_valid ());

  auto engine = create_timestretch_engine (
    TimeStretchEngineId::RubberBand, units::sample_rate (44100));
  const auto out = engine->stretch (input, warp, StretchOptions{});

  EXPECT_EQ (out.getNumSamples (), static_cast<int> (2 * kFrames));
  // Output should carry energy (not silent).
  EXPECT_GT (rms (out), 0.05);
}

TEST (RubberBandTimeStretchEngineTest, LinearRampProducesExactLength)
{
  constexpr int64_t kFrames = 8820; // 0.2 s
  const auto        input = make_sine (2, kFrames, 300.0);
  const auto        warp = make_ramp_warp (kFrames);
  ASSERT_TRUE (warp.is_valid ());

  auto engine = create_timestretch_engine (
    TimeStretchEngineId::RubberBand, units::sample_rate (44100));
  const auto out = engine->stretch (input, warp, StretchOptions{});

  EXPECT_EQ (
    out.getNumSamples (),
    static_cast<int> (warp.output_length.in (units::samples)));
  EXPECT_GT (rms (out), 0.05);
}

// The engine reads its channel count from the input buffer, so mono must work
// the same as stereo (ported from the legacy stretcher test).
TEST (RubberBandTimeStretchEngineTest, MonoInputWorks)
{
  constexpr int64_t kFrames = 4410;
  const auto        input = make_sine (/*channels=*/1, kFrames, 440.0);
  const auto        warp = make_constant_warp (kFrames, kFrames);
  ASSERT_TRUE (warp.is_valid ());

  auto engine = create_timestretch_engine (
    TimeStretchEngineId::RubberBand, units::sample_rate (44100));
  const auto out = engine->stretch (input, warp, StretchOptions{});

  EXPECT_EQ (out.getNumChannels (), 1);
  EXPECT_EQ (out.getNumSamples (), static_cast<int> (kFrames));
  EXPECT_NEAR (rms (out), rms (input), 0.05);
}

// Regression: when a region starts before a tempo change, the warp map has an
// identity anchor at the boundary (source_frame == output_frame) followed by a
// non-identity stretch anchor. RubberBand's updateRatioFromMap() computes the
// initial ratio from the first keyframe — an identity keyframe yields ratio
// 1.0, silently disabling stretching for the entire output.
TEST (RubberBandTimeStretchEngineTest, IdentityAnchorBeforeStretchNotSilenced)
{
  constexpr int64_t kFrames = 4410; // 0.1 s
  const auto        input = make_sine (2, kFrames, 440.0);

  // Warp map emulating a region that starts ~23 samples before a tempo change:
  //   (0, 0)         — origin
  //   (23, 23)       — identity: tiny constant-tempo lead-in before the event
  //   (4410, 8820)   — 2× stretch for the remainder
  TimeWarpMap warp;
  warp.source_length = units::samples (kFrames);
  warp.output_length = units::samples (2 * kFrames);
  warp.anchors.push_back ({ units::samples (0), units::samples (0) });
  warp.anchors.push_back ({ units::samples (23), units::samples (23) });
  warp.anchors.push_back (
    { units::samples (kFrames), units::samples (2 * kFrames) });
  ASSERT_TRUE (warp.is_valid ());

  auto engine = create_default_timestretch_engine (
    StretchOptions{}, units::sample_rate (44100));
  const auto out = engine->stretch (input, warp, StretchOptions{});

  EXPECT_EQ (out.getNumSamples (), static_cast<int> (2 * kFrames));

  // The second half of the output must carry energy (stretched audio), not
  // silence. With the bug, RubberBand processes at ratio 1.0 and the second
  // half is filled with zeros.
  const int half = out.getNumSamples () / 2;
  double    sum = 0.0;
  int       count = 0;
  for (int c = 0; c < out.getNumChannels (); ++c)
    for (int i = half; i < out.getNumSamples (); ++i)
      {
        const double s = out.getSample (c, i);
        sum += s * s;
        ++count;
      }
  const double second_half_rms = std::sqrt (sum / count);
  EXPECT_GT (second_half_rms, 0.05)
    << "Second half of output is near-silent — stretching was likely disabled "
       "by an identity keyframe";
}

// A short input (well below a second) exercises RubberBand's offline handling
// of sub-window signals without crashing or truncating (ported from the legacy
// realtime-block test, adapted to the offline/whole-input model).
TEST (RubberBandTimeStretchEngineTest, ShortInputProducesExactLength)
{
  constexpr int64_t kFrames = 512;
  const auto        input = make_sine (2, kFrames, 440.0);
  const auto        warp = make_constant_warp (kFrames, 2 * kFrames);
  ASSERT_TRUE (warp.is_valid ());

  auto engine = create_timestretch_engine (
    TimeStretchEngineId::RubberBand, units::sample_rate (44100));
  const auto out = engine->stretch (input, warp, StretchOptions{});

  EXPECT_EQ (out.getNumSamples (), static_cast<int> (2 * kFrames));
  // Even a short, heavily-stretched signal should not come back silent.
  EXPECT_GT (rms (out), 0.01);
}

TEST (RubberBandTimeStretchEngineTest, NviRejectsInvalidPreconditions)
{
  constexpr int64_t kFrames = 4410;
  const auto        input = make_sine (1, kFrames, 440.0);

  auto engine = create_timestretch_engine (
    TimeStretchEngineId::RubberBand, units::sample_rate (44100));

  // Invalid warp (empty).
  TimeWarpMap bad;
  EXPECT_THROW (
    (void) engine->stretch (input, bad, StretchOptions{}),
    std::invalid_argument);

  // Source length mismatch.
  auto warp = make_constant_warp (kFrames, 2 * kFrames);
  warp.source_length =
    units::samples (kFrames + 1); // now inconsistent with anchors
  EXPECT_THROW (
    (void) engine->stretch (input, warp, StretchOptions{}),
    std::invalid_argument);

  // Unsupported pitch mode.
  const auto     good_warp = make_constant_warp (kFrames, kFrames);
  StretchOptions repitch{ .pitch_mode = StretchOptions::PitchMode::Repitch };
  EXPECT_THROW (
    (void) engine->stretch (input, good_warp, repitch), std::invalid_argument);
}

// Extreme stretch ratios (e.g., 1 BPM project with 120 BPM source = 120x)
// must produce full-length output with energy throughout. Using a full
// 8192-sample block at 20x ratio ensures RubberBand's internal output buffer
// (initially 131072) is exceeded per-block, exercising the buffer-growth path.
TEST (RubberBandTimeStretchEngineTest, ExtremeStretchRatioProducesFullOutput)
{
  constexpr int64_t kFrames = 8192; // one full process block
  constexpr int64_t kRatio = 20;    // 8192*20 = 163840 > 131072 default buffer
  const auto        input = make_sine (2, kFrames, 440.0);
  const auto        warp = make_constant_warp (kFrames, kFrames * kRatio);
  ASSERT_TRUE (warp.is_valid ());

  auto engine = create_default_timestretch_engine (
    StretchOptions{}, units::sample_rate (44100));
  const auto out = engine->stretch (input, warp, StretchOptions{});

  EXPECT_EQ (out.getNumSamples (), static_cast<int> (kFrames * kRatio));

  // The last 10% must carry energy — not truncated or silent.
  const int check_start = out.getNumSamples () * 9 / 10;
  double    sum = 0.0;
  int       count = 0;
  for (int c = 0; c < out.getNumChannels (); ++c)
    for (int i = check_start; i < out.getNumSamples (); ++i)
      {
        sum += std::pow (out.getSample (c, i), 2.0);
        ++count;
      }
  const double tail_rms = std::sqrt (sum / count);
  EXPECT_GT (tail_rms, 0.01) << "Output tail is near-silent at extreme ratio";
}

} // namespace zrythm::dsp
