// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <vector>

#include "dsp/content_time_warp.h"
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

  std::unique_ptr<TempoMap> map_;
};

// Identity warp points (delta == content) with source BPM 120 at project
// tempo 120 → source_frame == output_frame at every anchor.
TEST_F (TempoWarpMapTest, IdentityWarpPointsProduceIdentityMap)
{
  constexpr int64_t                       kFrames = 44100;
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { units::ticks (0.0),    units::ticks (0.0)    },
    { units::ticks (1920.0), units::ticks (1920.0) }
  };

  const TimeWarpMap twm = to_time_warp_map (
    warp_points, *map_, units::ticks (0.0), 120.0, units::samples (kFrames));

  ASSERT_TRUE (twm.is_valid ());
  EXPECT_EQ (twm.source_length, units::samples (kFrames));
  EXPECT_EQ (twm.output_length, units::samples (kFrames));
  EXPECT_EQ (twm.anchors.front ().source_frame, units::samples (0));
  EXPECT_EQ (twm.anchors.front ().output_frame, units::samples (0));
  EXPECT_EQ (twm.anchors.back ().source_frame, units::samples (kFrames));
  EXPECT_EQ (twm.anchors.back ().output_frame, units::samples (kFrames));
}

// Source BPM 240 at project tempo 120: each content second maps to 2x the
// timeline duration. Source audio (1s) doubles in output length.
TEST_F (TempoWarpMapTest, SourceBpmDoubleProducesDoubleOutput)
{
  constexpr int64_t kFrames = 44100;
  // 1 second @ 240 BPM = 3840 ticks. Identity delta = 3840.
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { units::ticks (0.0),    units::ticks (0.0)    },
    { units::ticks (3840.0), units::ticks (3840.0) }
  };

  const TimeWarpMap twm = to_time_warp_map (
    warp_points, *map_, units::ticks (0.0), 240.0, units::samples (kFrames));

  ASSERT_TRUE (twm.is_valid ());
  EXPECT_EQ (twm.output_length, units::samples (2 * kFrames));
  EXPECT_EQ (twm.anchors.back ().output_frame, units::samples (2 * kFrames));
}

// Tempo-derived warp points (musical mode OFF): delta_ticks maps source
// ticks to project-tempo ticks so source_frame == output_frame (no stretch).
TEST_F (TempoWarpMapTest, TempoDerivedWarpProducesIdentitySamples)
{
  constexpr int64_t kFrames = 44100;
  // Source BPM 120, project tempo 120. Content 1920 ticks.
  // Tempo-derived delta: 1920 (same at 120 BPM).
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { units::ticks (0.0),    units::ticks (0.0)    },
    { units::ticks (1920.0), units::ticks (1920.0) }
  };

  const TimeWarpMap twm = to_time_warp_map (
    warp_points, *map_, units::ticks (0.0), 120.0, units::samples (kFrames));

  ASSERT_TRUE (twm.is_valid ());
  bool all_identity = std::ranges::all_of (twm.anchors, [] (const WarpAnchor &a) {
    return a.source_frame == a.output_frame;
  });
  EXPECT_TRUE (all_identity);
}

// Multiple warp points map 1:1 to anchors.
TEST_F (TempoWarpMapTest, MultipleWarpPointsMapToOneToOne)
{
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { units::ticks (0.0),    units::ticks (0.0)    },
    { units::ticks (960.0),  units::ticks (960.0)  },
    { units::ticks (1920.0), units::ticks (1920.0) }
  };

  const TimeWarpMap twm = to_time_warp_map (
    warp_points, *map_, units::ticks (0.0), 120.0, units::samples (44100));

  ASSERT_TRUE (twm.is_valid ());
  EXPECT_EQ (twm.anchors.size (), 3u);
}

// Non-zero region start: output frames are relative to region start.
TEST_F (TempoWarpMapTest, NonZeroRegionStartPreservesInvariants)
{
  constexpr int64_t                       kFrames = 44100;
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { units::ticks (0.0),    units::ticks (0.0)    },
    { units::ticks (1920.0), units::ticks (1920.0) }
  };

  const auto        start = units::ticks (1920.0); // region starts at bar 2
  const TimeWarpMap twm = to_time_warp_map (
    warp_points, *map_, start, 120.0, units::samples (kFrames));

  EXPECT_TRUE (twm.is_valid ());
  // Output frames are relative to region start (subtract region start samples).
  EXPECT_EQ (twm.anchors.front ().output_frame, units::samples (0));
}

// Non-positive source BPM is a programming error.
TEST_F (TempoWarpMapTest, NonPositiveSourceBpmThrows)
{
  std::vector<ContentTimeWarp::WarpPoint> wp = {
    { units::ticks (0.0), units::ticks (0.0) }
  };
  EXPECT_THROW (
    (void) to_time_warp_map (
      wp, *map_, units::ticks (0.0), 0.0, units::samples (100)),
    std::invalid_argument);
  EXPECT_THROW (
    (void) to_time_warp_map (
      wp, *map_, units::ticks (0.0), -10.0, units::samples (100)),
    std::invalid_argument);
}

// is_sample_space_identity: ±2 tolerance for rounding noise.
TEST_F (TempoWarpMapTest, SampleSpaceIdentityAcceptsSmallDifferences)
{
  std::vector<WarpAnchor> anchors = {
    { units::samples (0),   units::samples (0)   },
    { units::samples (100), units::samples (101) }, // +1
    { units::samples (200), units::samples (198) }, // -2
    { units::samples (300), units::samples (300) }, // exact
  };
  EXPECT_TRUE (is_sample_space_identity (anchors));
}

TEST_F (TempoWarpMapTest, SampleSpaceIdentityRejectsLargeDifferences)
{
  std::vector<WarpAnchor> exact = {
    { units::samples (0),   units::samples (0)   },
    { units::samples (100), units::samples (100) },
  };
  EXPECT_TRUE (is_sample_space_identity (exact));

  std::vector<WarpAnchor> too_far = {
    { units::samples (0),   units::samples (0)   },
    { units::samples (100), units::samples (103) }, // +3 > tolerance
  };
  EXPECT_FALSE (is_sample_space_identity (too_far));
}

// When sub-sample output rounding ties the terminal anchor's output_frame to
// its predecessor's, the cleanup must still keep the terminal so that the last
// anchor maps source_length -> output_length (the TimeWarpMap invariant).
TEST_F (TempoWarpMapTest, TerminalAnchorPreservedOnOutputTie)
{
  // At 120 BPM / 44100 Hz, samples_per_tick = 22.96875.
  // deltas 100.0 and 100.01 both round to output sample 2297 (tie), while their
  // source frames (2297 and 4594) differ. Without pinning the terminal, the
  // cleanup drops the {4594, 2297} anchor via the output_frame <= last check.
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { units::ticks (0.0),   units::ticks (0.0)    },
    { units::ticks (100.0), units::ticks (100.0)  },
    { units::ticks (200.0), units::ticks (100.01) },
  };
  constexpr int64_t kSourceFrames = 4594; // source_frame_for(200 ticks)
  const TimeWarpMap twm = to_time_warp_map (
    warp_points, *map_, units::ticks (0.0), 120.0,
    units::samples (kSourceFrames));

  // The terminal anchor must survive and map source_length -> output_length.
  EXPECT_EQ (twm.anchors.back ().source_frame, twm.source_length);
}

} // namespace zrythm::dsp
