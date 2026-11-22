// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>

#include "dsp/tempo_map.h"
#include "dsp/tick_types.h"
#include "frontends/gui/qquick/waveform_canvas_renderer.h"
#include "utils/units.h"

#include <gtest/gtest.h>
#include <juce_audio_basics/juce_audio_basics.h>

namespace zrythm::frontends::gui::qquick
{

namespace
{

/// Creates a mono buffer of @p num_samples filled with @p value.
juce::AudioSampleBuffer
make_mono_buffer (int num_samples, float value = 0.0f)
{
  juce::AudioSampleBuffer buf (1, num_samples);
  for (int i = 0; i < num_samples; ++i)
    buf.setSample (0, i, value);
  return buf;
}

/// Creates a stereo buffer where each channel is filled independently.
juce::AudioSampleBuffer
make_stereo_buffer (int num_samples, float ch0_value, float ch1_value)
{
  juce::AudioSampleBuffer buf (2, num_samples);
  buf.clear ();
  for (int i = 0; i < num_samples; ++i)
    {
      buf.setSample (0, i, ch0_value);
      buf.setSample (1, i, ch1_value);
    }
  return buf;
}

/// Convenience wrapper: computes a linear frame mapping then peaks.
/// Used by tests that don't need non-linear mapping.
std::vector<std::vector<WaveformPeak>>
compute_peaks_linear (
  const juce::AudioSampleBuffer &buf,
  int                            canvas_width,
  qreal                          reference_width,
  qreal                          reference_x,
  int64_t                        loop_wrap_start = 0,
  int64_t                        loop_wrap_length = 0,
  bool                           has_loop = false)
{
  auto frames = compute_linear_frame_mapping (
    canvas_width, reference_width, reference_x, buf.getNumSamples ());
  return compute_waveform_peaks (
    buf, frames, canvas_width, loop_wrap_start, loop_wrap_length, has_loop);
}

} // namespace

// ===========================================================================
// compute_frame_mapping tests
// ===========================================================================

TEST (FrameMappingTest, LinearMappingIsUniform)
{
  // 100 pixels, reference_width=100, total_frames=1000
  // Each pixel should cover exactly 10 frames
  auto frames = compute_linear_frame_mapping (100, 100, 0, 1000);
  ASSERT_EQ (frames.size (), 101u); // canvas_width + 1
  for (int i = 0; i <= 100; ++i)
    EXPECT_EQ (frames[i], i * 10);
}

TEST (FrameMappingTest, LinearMappingWithReferenceWidth)
{
  // Canvas=200px but reference_width=100 → density is 1000 samples / 100px
  // Pixels 0-99 map to frames 0-999, pixels 100-199 map beyond buffer
  auto frames = compute_linear_frame_mapping (200, 100, 0, 1000);
  ASSERT_EQ (frames.size (), 201u);
  EXPECT_EQ (frames[0], 0);
  EXPECT_EQ (frames[100], 1000); // boundary
  EXPECT_EQ (frames[200], 2000); // beyond buffer
}

TEST (FrameMappingTest, LinearMappingWithReferenceX)
{
  // reference_x=50, reference_width=100
  // Pixel 0 → fraction 50/100 = 0.5 → frame 500
  auto frames = compute_linear_frame_mapping (50, 100, 50, 1000);
  ASSERT_EQ (frames.size (), 51u);
  EXPECT_EQ (frames[0], 500);
  EXPECT_EQ (frames[50], 1000);
}

TEST (FrameMappingTest, NonLinearQuadraticMapping)
{
  // Simulates a tempo ramp: content fraction → frame is quadratic
  // frame(frac) = frac^2 * 1000
  // At frac=0: 0, frac=0.5: 250, frac=1.0: 1000
  auto frames = compute_frame_mapping (100, 100, 0, [] (double f) {
    return static_cast<int64_t> (f * f * 1000);
  });
  ASSERT_EQ (frames.size (), 101u);
  EXPECT_EQ (frames[0], 0);
  EXPECT_EQ (frames[50], 250);
  EXPECT_EQ (frames[100], 1000);

  // Verify the mapping is non-uniform: pixel 0 covers more frames than pixel 99
  const int64_t early_span = frames[1] - frames[0];   // frac 0.00→0.01
  const int64_t late_span = frames[100] - frames[99]; // frac 0.99→1.00
  EXPECT_GT (late_span, early_span);
}

TEST (FrameMappingTest, NonLinearMappingMatchesConverter)
{
  // Verify that each frame matches the converter function exactly
  const auto converter = [] (double f) {
    return static_cast<int64_t> (std::pow (f, 1.5) * 5000);
  };
  auto frames = compute_frame_mapping (50, 50, 0, converter);
  ASSERT_EQ (frames.size (), 51u);
  for (int i = 0; i <= 50; ++i)
    {
      const double frac = static_cast<double> (i) / 50.0;
      EXPECT_EQ (frames[i], converter (frac));
    }
}

TEST (FrameMappingTest, ZeroCanvasWidthReturnsEmpty)
{
  auto frames = compute_linear_frame_mapping (0, 100, 0, 1000);
  EXPECT_TRUE (frames.empty ());
}

TEST (FrameMappingTest, ReferenceWidthFallbackToCanvasWidth)
{
  // reference_width=0 → should fall back to canvas_width
  auto frames = compute_linear_frame_mapping (10, 0, 0, 100);
  ASSERT_EQ (frames.size (), 11u);
  EXPECT_EQ (frames[0], 0);
  EXPECT_EQ (frames[10], 100);
}

// ===========================================================================
// compute_waveform_peaks tests (using linear frame mapping)
// ===========================================================================

TEST (WaveformPeakComputationTest, BasicMonoBuffer)
{
  juce::AudioSampleBuffer buf (1, 1000);
  for (int i = 0; i < 500; ++i)
    buf.setSample (0, i, 1.0f);
  for (int i = 500; i < 1000; ++i)
    buf.setSample (0, i, -1.0f);

  auto peaks = compute_peaks_linear (buf, 100, 100, 0);

  ASSERT_EQ (peaks.size (), 1u);
  ASSERT_EQ (peaks[0].size (), 100u);

  EXPECT_FLOAT_EQ (peaks[0][0].min, 1.0f);
  EXPECT_FLOAT_EQ (peaks[0][0].max, 1.0f);
  EXPECT_FLOAT_EQ (peaks[0][49].min, 1.0f);
  EXPECT_FLOAT_EQ (peaks[0][49].max, 1.0f);

  EXPECT_FLOAT_EQ (peaks[0][50].min, 0.0f);
  EXPECT_FLOAT_EQ (peaks[0][50].max, 0.0f);
  EXPECT_FLOAT_EQ (peaks[0][99].min, 0.0f);
  EXPECT_FLOAT_EQ (peaks[0][99].max, 0.0f);
}

TEST (WaveformPeakComputationTest, BasicStereoBuffer)
{
  auto buf = make_stereo_buffer (100, 0.5f, -0.5f);
  auto peaks = compute_peaks_linear (buf, 10, 10, 0);

  ASSERT_EQ (peaks.size (), 2u);
  EXPECT_FLOAT_EQ (peaks[0][0].min, 0.75f);
  EXPECT_FLOAT_EQ (peaks[0][0].max, 0.75f);
  EXPECT_FLOAT_EQ (peaks[1][0].min, 0.25f);
  EXPECT_FLOAT_EQ (peaks[1][0].max, 0.25f);
}

TEST (WaveformPeakComputationTest, PeakRangeSpansPositiveAndNegative)
{
  juce::AudioSampleBuffer buf2 (1, 10);
  for (int i = 0; i < 5; ++i)
    buf2.setSample (0, i, 1.0f);
  for (int i = 5; i < 10; ++i)
    buf2.setSample (0, i, -1.0f);

  auto peaks = compute_peaks_linear (buf2, 1, 1, 0);
  ASSERT_EQ (peaks[0].size (), 1u);
  EXPECT_FLOAT_EQ (peaks[0][0].min, 0.0f);
  EXPECT_FLOAT_EQ (peaks[0][0].max, 1.0f);
}

TEST (WaveformPeakComputationTest, NoLoopCanvasWiderThanReference)
{
  auto buf = make_mono_buffer (1000, 1.0f);
  auto peaks = compute_peaks_linear (buf, 200, 100, 0);

  ASSERT_EQ (peaks[0].size (), 200u);
  EXPECT_FLOAT_EQ (peaks[0][50].min, 1.0f);
  EXPECT_FLOAT_EQ (peaks[0][50].max, 1.0f);
  EXPECT_FLOAT_EQ (peaks[0][100].min, 0.5f);
  EXPECT_FLOAT_EQ (peaks[0][100].max, 0.5f);
  EXPECT_FLOAT_EQ (peaks[0][199].min, 0.5f);
  EXPECT_FLOAT_EQ (peaks[0][199].max, 0.5f);
}

TEST (WaveformPeakComputationTest, LoopWrapWithinBounds)
{
  auto buf = make_mono_buffer (1000, 0.0f);
  for (int i = 0; i < 500; ++i)
    buf.setSample (0, i, 1.0f);

  auto peaks = compute_peaks_linear (
    buf, 200, 100, 0, /*loop_wrap_start=*/0, /*loop_wrap_length=*/500, true);

  ASSERT_EQ (peaks[0].size (), 200u);
  EXPECT_FLOAT_EQ (peaks[0][100].min, 1.0f);
  EXPECT_FLOAT_EQ (peaks[0][100].max, 1.0f);
}

TEST (WaveformPeakComputationTest, LoopEndExceedsBufferDoesNotCrash)
{
  auto buf = make_mono_buffer (1000, 0.0f);

  EXPECT_NO_THROW ({
    auto peaks = compute_peaks_linear (
      buf, 20, 10, 0,
      /*loop_wrap_start=*/600, /*loop_wrap_length=*/1000, true);
    for (const auto &channel : peaks)
      for (const auto &p : channel)
        {
          EXPECT_GE (p.min, 0.0f);
          EXPECT_LE (p.min, 1.0f);
          EXPECT_GE (p.max, 0.0f);
          EXPECT_LE (p.max, 1.0f);
        }
  });
}

TEST (WaveformPeakComputationTest, EmptyBufferReturnsEmpty)
{
  juce::AudioSampleBuffer buf (1, 0);
  auto                    peaks = compute_peaks_linear (buf, 100, 100, 0);
  EXPECT_TRUE (peaks.empty ());
}

TEST (WaveformPeakComputationTest, ZeroCanvasWidthReturnsEmpty)
{
  auto buf = make_mono_buffer (100, 1.0f);
  auto peaks = compute_peaks_linear (buf, 0, 100, 0);
  EXPECT_TRUE (peaks.empty ());
}

TEST (WaveformPeakComputationTest, ZeroChannelsReturnsEmpty)
{
  juce::AudioSampleBuffer buf (0, 100);
  auto                    peaks = compute_peaks_linear (buf, 100, 100, 0);
  EXPECT_TRUE (peaks.empty ());
}

TEST (WaveformPeakComputationTest, SingleSampleBuffer)
{
  juce::AudioSampleBuffer buf (1, 1);
  buf.setSample (0, 0, 0.7f);

  auto peaks = compute_peaks_linear (buf, 10, 10, 0);
  ASSERT_EQ (peaks[0].size (), 10u);
  EXPECT_FLOAT_EQ (peaks[0][0].min, 0.85f);
  EXPECT_FLOAT_EQ (peaks[0][0].max, 0.85f);
}

TEST (WaveformPeakComputationTest, ReferenceXOffsetShiftsContent)
{
  auto buf = make_mono_buffer (1000, 0.0f);
  buf.setSample (0, 500, 1.0f);

  auto peaks = compute_peaks_linear (buf, 50, 100, 50);
  ASSERT_EQ (peaks[0].size (), 50u);
  EXPECT_FLOAT_EQ (peaks[0][0].max, 1.0f);
}

// ===========================================================================
// Non-linear frame mapping with peaks (simulated tempo ramp)
// ===========================================================================

TEST (WaveformPeakComputationTest, NonLinearMappingPlacesPeakAtCorrectPixel)
{
  // Buffer: 1000 samples, all silence except sample 250 = +1.0
  auto buf = make_mono_buffer (1000, 0.0f);
  buf.setSample (0, 250, 1.0f);

  // Quadratic mapping: frame = frac^2 * 1000
  // frac=0.5 → frame=250. So pixel 50 (of 100) should show the impulse.
  auto frames = compute_frame_mapping (100, 100, 0, [] (double f) {
    return static_cast<int64_t> (f * f * 1000);
  });
  auto peaks = compute_waveform_peaks (buf, frames, 100, 0, 0, false);

  ASSERT_EQ (peaks[0].size (), 100u);

  // With quadratic mapping, the impulse at frame 250 should appear at pixel 50,
  // NOT at pixel 25 (which is where linear mapping would put 250/1000*100).
  EXPECT_FLOAT_EQ (peaks[0][50].max, 1.0f);

  // With linear mapping, frame 250 = pixel 25 — verify it's NOT at pixel 50.
  auto linear_peaks = compute_peaks_linear (buf, 100, 100, 0);
  EXPECT_FLOAT_EQ (linear_peaks[0][25].max, 1.0f);
  // Pixel 50 is silence (amplitude 0.0 → normalized 0.5)
  EXPECT_FLOAT_EQ (linear_peaks[0][50].max, 0.5f);
}

// ===========================================================================
// compute_timeline_frame_mapping tests
// ===========================================================================

namespace
{

constexpr auto kTestSampleRate = units::sample_rate (44100.0);

} // namespace

TEST (TimelineFrameMappingTest, ConstantTempoMatchesLinear)
{
  dsp::TempoMap tempo_map (kTestSampleRate);

  const dsp::TimelineTick clip_start_tick{ units::ticks (0.0) };
  const double            timeline_tick_duration = 1920.0;

  const auto tl_frames = compute_timeline_frame_mapping (
    100, 100, 0, tempo_map, clip_start_tick, timeline_tick_duration);

  const int64_t total_samples =
    tempo_map
      .tick_to_samples_rounded (
        dsp::TimelineTick{ units::ticks (timeline_tick_duration) })
      .in (units::samples);
  const auto lin_frames =
    compute_linear_frame_mapping (100, 100, 0, total_samples);

  ASSERT_EQ (tl_frames.size (), lin_frames.size ());
  ASSERT_EQ (tl_frames.size (), 101u);
  for (size_t i = 0; i < tl_frames.size (); ++i)
    {
      EXPECT_NEAR (tl_frames[i], lin_frames[i], 1) << "Mismatch at pixel " << i;
    }
}

TEST (TimelineFrameMappingTest, TempoChangeProducesNonLinearMapping)
{
  // 120 BPM for first 1920 ticks, 60 BPM thereafter
  dsp::TempoMap tempo_map (kTestSampleRate);
  tempo_map.add_tempo_event (
    units::ticks (1920), units::bpm (60.0), dsp::TempoMap::CurveType::Constant);

  const dsp::TimelineTick clip_start_tick{ units::ticks (0.0) };
  const double            timeline_tick_duration = 3840.0;

  // At 120 BPM: 1920 ticks = 44100 samples
  // At 60 BPM: 1920 ticks = 88200 samples
  // Total: 132300 samples
  const int64_t midpoint_sample = 44100;
  const int64_t total_samples = 132300;

  const auto frames = compute_timeline_frame_mapping (
    100, 100, 0, tempo_map, clip_start_tick, timeline_tick_duration);

  ASSERT_EQ (frames.size (), 101u);

  // At fraction 0 (clip start): frame 0
  EXPECT_EQ (frames[0], 0);

  // At fraction 0.5 (tick 1920 — tempo change boundary):
  // Non-linear: tick_to_samples(1920) = 44100
  // Linear would give: 0.5 * 132300 = 66150
  EXPECT_EQ (frames[50], midpoint_sample);
  EXPECT_NE (frames[50], total_samples / 2);

  // At fraction 1.0 (tick 3840): total samples
  EXPECT_EQ (frames[100], total_samples);

  // Non-uniformity: first-half pixels cover fewer samples than second-half
  // First quarter (120 BPM): 22050 samples across 25 pixels
  // Third quarter (60 BPM): 44100 samples across 25 pixels
  const int64_t first_quarter_span = frames[25] - frames[0];
  const int64_t third_quarter_span = frames[75] - frames[50];
  EXPECT_GT (third_quarter_span, first_quarter_span);
}

TEST (TimelineFrameMappingTest, NonZeroStartTickMatchesRelativeOffset)
{
  // Clip starts at tick 3840 (in the 60 BPM region of the previous test's map)
  dsp::TempoMap tempo_map (kTestSampleRate);
  tempo_map.add_tempo_event (
    units::ticks (1920), units::bpm (60.0), dsp::TempoMap::CurveType::Constant);

  const dsp::TimelineTick clip_start_tick{ units::ticks (3840.0) };
  const double            timeline_tick_duration = 1920.0;

  const auto frames = compute_timeline_frame_mapping (
    50, 50, 0, tempo_map, clip_start_tick, timeline_tick_duration);

  ASSERT_EQ (frames.size (), 51u);

  // All 1920 ticks at 60 BPM: 88200 samples total
  const auto clip_start_sample =
    tempo_map.tick_to_samples_rounded (clip_start_tick);
  const auto clip_end_sample = tempo_map.tick_to_samples_rounded (
    dsp::TimelineTick{ units::ticks (3840.0 + 1920.0) });
  const int64_t expected_total =
    (clip_end_sample - clip_start_sample).in (units::samples);

  EXPECT_EQ (frames[0], 0);
  EXPECT_EQ (frames[50], expected_total);
}

TEST (TimelineFrameMappingTest, RespectsReferenceX)
{
  dsp::TempoMap tempo_map (kTestSampleRate);
  tempo_map.add_tempo_event (
    units::ticks (1920), units::bpm (60.0), dsp::TempoMap::CurveType::Constant);

  const dsp::TimelineTick clip_start_tick{ units::ticks (0.0) };
  const double            timeline_tick_duration = 3840.0;

  // reference_x = 50, reference_width = 100
  // Pixel 0 → fraction 50/100 = 0.5 → tick 1920 → sample 44100
  const auto frames = compute_timeline_frame_mapping (
    50, 100, 50, tempo_map, clip_start_tick, timeline_tick_duration);

  ASSERT_EQ (frames.size (), 51u);
  EXPECT_EQ (frames[0], 44100);
}

} // namespace zrythm::frontends::gui::qquick
