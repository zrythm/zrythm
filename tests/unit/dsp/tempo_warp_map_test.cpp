// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"
#include "dsp/tempo_warp_map.h"
#include "utils/units.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

class TempoWarpMapTest : public ::testing::Test
{
protected:
  static constexpr auto SAMPLE_RATE = units::sample_rate (44100.0);

  void SetUp () override { map_ = std::make_unique<TempoMap> (SAMPLE_RATE); }

  /// Expected total output length for a region [start, start+musical_ticks].
  units::sample_t
  expected_output (units::precise_tick_t start, units::precise_tick_t end) const
  {
    return map_->tick_to_samples_rounded (end)
           - map_->tick_to_samples_rounded (start);
  }

  std::unique_ptr<TempoMap> map_;
};

// At the project's default tempo with a matching source BPM, the warp is the
// identity: output length equals source length, with exactly two anchors.
TEST_F (TempoWarpMapTest, ConstantTempoRatioOneIsIdentity)
{
  constexpr int64_t kFrames = 44100; // 1 second at 44.1 kHz
  const auto        start = units::ticks (0.0);
  const TimeWarpMap map =
    compute_tempo_warp_map (*map_, start, 120.f, units::samples (kFrames));

  ASSERT_TRUE (map.is_valid ());
  EXPECT_EQ (map.anchors.size (), 2u);
  EXPECT_EQ (map.source_length, units::samples (kFrames));
  EXPECT_EQ (map.output_length, units::samples (kFrames));
  EXPECT_EQ (map.anchors.front ().source_frame, units::samples (0));
  EXPECT_EQ (map.anchors.front ().output_frame, units::samples (0));
  EXPECT_EQ (map.anchors.back ().source_frame, units::samples (kFrames));
  EXPECT_EQ (map.anchors.back ().output_frame, units::samples (kFrames));
}

// Doubling the source BPM doubles the musical length, hence the output length.
TEST_F (TempoWarpMapTest, ConstantTempoRatioTwoDoublesLength)
{
  constexpr int64_t kFrames = 44100;
  const auto        start = units::ticks (0.0);
  const TimeWarpMap map =
    compute_tempo_warp_map (*map_, start, 240.f, units::samples (kFrames));

  ASSERT_TRUE (map.is_valid ());
  EXPECT_EQ (map.output_length, units::samples (2 * kFrames));
  EXPECT_EQ (map.anchors.back ().output_frame, units::samples (2 * kFrames));
}

// A stepped tempo change inside the region adds a boundary anchor and the
// output length follows the tempo map integral.
TEST_F (TempoWarpMapTest, SteppedTempoAddsBoundaryAnchor)
{
  // Region spans 3840 ticks of musical content (source_bpm 120, 2 s of audio).
  constexpr int64_t kFrames = 88200;
  const auto        start = units::ticks (0.0);
  const auto        musical_ticks = units::ticks (3840.0);
  const auto        region_end = start + musical_ticks;

  // Halve the tempo at the midpoint (constant, i.e. a step).
  map_->add_tempo_event (
    units::ticks (1920), 60.0, TempoMap::CurveType::Constant);

  const TimeWarpMap map =
    compute_tempo_warp_map (*map_, start, 120.f, units::samples (kFrames));

  ASSERT_TRUE (map.is_valid ());
  EXPECT_EQ (map.output_length, expected_output (start, region_end));
  // Two constant segments -> exactly three anchors (start, boundary, end).
  ASSERT_EQ (map.anchors.size (), 3u);
  // The boundary anchor sits at the source frame corresponding to tick 1920.
  // (1920 ticks @ source 120 BPM = 1 s = 44100 frames.)
  EXPECT_EQ (map.anchors[1].source_frame, units::samples (44100));
  EXPECT_EQ (map.anchors[1].output_frame, units::samples (44100));
}

// A linear tempo ramp inside the region produces a dense set of anchors so the
// mapping tracks the continuously varying ratio.
TEST_F (TempoWarpMapTest, LinearRampProducesDenseAnchors)
{
  constexpr int64_t kFrames = 88200;
  const auto        start = units::ticks (0.0);
  const auto        musical_ticks = units::ticks (3840.0);
  const auto        region_end = start + musical_ticks;

  // Ramp 120 -> 240 over [0, 1920], then constant 240 over [1920, 3840].
  map_->add_tempo_event (units::ticks (0), 120.0, TempoMap::CurveType::Linear);
  map_->add_tempo_event (
    units::ticks (1920), 240.0, TempoMap::CurveType::Constant);

  const TimeWarpMap map =
    compute_tempo_warp_map (*map_, start, 120.f, units::samples (kFrames));

  ASSERT_TRUE (map.is_valid ());
  EXPECT_EQ (map.output_length, expected_output (start, region_end));
  // Far more than the 3 anchors a purely stepped map would have, thanks to the
  // dense anchors inside the linear segment.
  EXPECT_GT (map.anchors.size (), 3u);
}

// Region starting 1 tick before a linear ramp still produces dense anchors
// for the ramp portion.
TEST_F (TempoWarpMapTest, RegionStartsBeforeLinearRampStillStretches)
{
  constexpr int64_t kFrames = 88200; // 2 s @ 44.1 kHz
  // Default tempo at tick 0 is 120 BPM Constant.
  // Linear ramp 120 -> 140 over [1920, 3840].
  map_->add_tempo_event (
    units::ticks (1920), 120.0, TempoMap::CurveType::Linear);
  map_->add_tempo_event (
    units::ticks (3840), 140.0, TempoMap::CurveType::Constant);

  // Region starts 1 tick before the ramp.
  const auto start = units::ticks (1919.0);

  const TimeWarpMap map =
    compute_tempo_warp_map (*map_, start, 120.f, units::samples (kFrames));

  ASSERT_TRUE (map.is_valid ());

  // Must have anchors with source_frame != output_frame (i.e., stretching
  // is needed).
  bool has_non_identity =
    std::ranges::any_of (map.anchors, [] (const WarpAnchor &a) {
      return a.source_frame != a.output_frame;
    });
  EXPECT_TRUE (has_non_identity);

  // Must have more than 3 anchors (the minimum for start + boundary + end
  // with no dense anchors).
  EXPECT_GT (map.anchors.size (), 3u);
}

// Any produced map must always satisfy the warp invariants.
TEST_F (TempoWarpMapTest, NonZeroRegionStartPreservesInvariants)
{
  constexpr int64_t kFrames = 44100;
  const auto        start = units::ticks (1920.0); // region starts at bar 2
  const TimeWarpMap map =
    compute_tempo_warp_map (*map_, start, 140.f, units::samples (kFrames));

  EXPECT_TRUE (map.is_valid ());
}

// A non-positive source BPM is a programming error.
TEST_F (TempoWarpMapTest, NonPositiveSourceBpmThrows)
{
  EXPECT_THROW (
    (void) compute_tempo_warp_map (
      *map_, units::ticks (0.0), 0.f, units::samples (100)),
    std::invalid_argument);
  EXPECT_THROW (
    (void) compute_tempo_warp_map (
      *map_, units::ticks (0.0), -10.f, units::samples (100)),
    std::invalid_argument);
}

} // namespace zrythm::dsp
