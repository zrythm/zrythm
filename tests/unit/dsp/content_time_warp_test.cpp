// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/content_time_warp.h"
#include "dsp/position.h"
#include "dsp/tempo_map.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "dsp/tempo_warp_map.h"
#include "dsp/time_warp_map.h"
#include "utils/units.h"

#include <QSignalSpy>

#include <gtest/gtest.h>

namespace zrythm::dsp
{
class ContentTimeWarpTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map_ = std::make_unique<TempoMap> (units::sample_rate (44100.0));
    tempo_map_->add_tempo_event (
      units::ticks (0), units::bpm (120.0), TempoMap::CurveType::Constant);
    tempo_map_wrapper_ = std::make_unique<TempoMapWrapper> (*tempo_map_);

    pos_ = std::make_unique<TimelinePosition> ();
    pos_->setTicks (0.0);

    len_ = std::make_unique<ContentPosition> ();
    len_->setTicks (7680.0);

    warp_ = std::make_unique<ContentTimeWarp> (
      *tempo_map_wrapper_, pos_.get (), len_.get ());
  }

  std::unique_ptr<TempoMap>         tempo_map_;
  std::unique_ptr<TempoMapWrapper>  tempo_map_wrapper_;
  std::unique_ptr<TimelinePosition> pos_;
  std::unique_ptr<ContentPosition>  len_;
  std::unique_ptr<ContentTimeWarp>  warp_;
};

TEST_F (ContentTimeWarpTest, ProjectModeIdentity)
{
  warp_->configure_as_project ();
  EXPECT_TRUE (warp_->warpPoints ().empty ());
  pos_->setTicks (1000.0);
  EXPECT_NEAR (
    warp_->contentToTimeline (ContentTick{ units::ticks (500.0) }).asDouble (),
    1500.0, 0.5);
}

TEST_F (ContentTimeWarpTest, ProjectModeWithSourceBpmGeneratesIdentityPoints)
{
  warp_->configure_as_project (units::bpm (120.0));
  // Identity warp points generated (not empty) for to_time_warp_map
  // compatibility.
  EXPECT_EQ (warp_->warpPoints ().size (), 2u);
  EXPECT_TRUE (warp_->is_identity ());
}

TEST_F (ContentTimeWarpTest, SourceModeSameBpm)
{
  warp_->configure_as_source (units::bpm (120.0));
  EXPECT_NEAR (
    warp_->contentToTimeline (ContentTick{ units::ticks (7680.0) }).asDouble (),
    7680.0, 5.0);
}

TEST_F (ContentTimeWarpTest, SourceModeDoubleTempo)
{
  tempo_map_ = std::make_unique<TempoMap> (units::sample_rate (44100.0));
  tempo_map_->add_tempo_event (
    units::ticks (0), units::bpm (240.0), TempoMap::CurveType::Constant);
  tempo_map_wrapper_ = std::make_unique<TempoMapWrapper> (*tempo_map_);

  pos_ = std::make_unique<TimelinePosition> ();
  pos_->setTicks (0.0);
  len_ = std::make_unique<ContentPosition> ();
  len_->setTicks (7680.0);
  warp_ = std::make_unique<ContentTimeWarp> (
    *tempo_map_wrapper_, pos_.get (), len_.get ());
  warp_->configure_as_source (units::bpm (120.0));

  EXPECT_NEAR (
    warp_->contentToTimeline (ContentTick{ units::ticks (7680.0) }).asDouble (),
    15360.0, 5.0);
}

TEST_F (ContentTimeWarpTest, ContentToTimelineSamplesAbsolute)
{
  warp_->configure_as_source (units::bpm (120.0));
  EXPECT_NEAR (
    warp_->contentToTimelineSamples (ContentTick{ units::ticks (7680.0) })
      .in (units::samples),
    176400, 100);
}

TEST_F (ContentTimeWarpTest, SourceModeWithNonZeroPosition)
{
  pos_->setTicks (1920.0);
  warp_->configure_as_source (units::bpm (120.0));
  // pos=1920, source BPM=120, project BPM=120 -> identity delta
  // contentToTimeline(7680) = 1920 + 7680 = 9600
  EXPECT_NEAR (
    warp_->contentToTimeline (ContentTick{ units::ticks (7680.0) }).asDouble (),
    9600.0, 5.0);
}

TEST_F (ContentTimeWarpTest, TempoChangeTriggersRebuild)
{
  warp_->configure_as_source (units::bpm (120.0));
  const auto before =
    warp_->contentToTimeline (ContentTick{ units::ticks (7680.0) }).asDouble ();

  tempo_map_wrapper_->addTempoEvent (
    0, 240.0, TempoEventWrapper::CurveType::Constant);

  EXPECT_NE (
    before,
    warp_->contentToTimeline (ContentTick{ units::ticks (7680.0) }).asDouble ());
}

TEST_F (ContentTimeWarpTest, ProjectModeIgnoresTempoChange)
{
  warp_->configure_as_project ();
  const auto before =
    warp_->contentToTimeline (ContentTick{ units::ticks (7680.0) }).asDouble ();

  tempo_map_wrapper_->addTempoEvent (
    0, 240.0, TempoEventWrapper::CurveType::Constant);

  EXPECT_EQ (
    before,
    warp_->contentToTimeline (ContentTick{ units::ticks (7680.0) }).asDouble ());
}

TEST_F (ContentTimeWarpTest, LengthChangeTriggersRebuild)
{
  warp_->configure_as_source (units::bpm (120.0));
  EXPECT_FALSE (warp_->warpPoints ().empty ());
  len_->setTicks (3840.0);
  EXPECT_FALSE (warp_->warpPoints ().empty ());
}

TEST (WarpLookupTest, EmptyIsIdentity)
{
  EXPECT_DOUBLE_EQ (
    warp_lookup ({}, ContentTick{ units::ticks (1234.0) }).asDouble (), 1234.0);
}

TEST (WarpLookupTest, TwoWarpPointsLinear)
{
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { ContentTick{ units::ticks (0.0) },    TimelineTick{ units::ticks (0.0) } },
    { ContentTick{ units::ticks (7680.0) },
     TimelineTick{ units::ticks (15360.0) }                                    }
  };
  EXPECT_NEAR (
    warp_lookup (warp_points, ContentTick{ units::ticks (3840.0) }).asDouble (),
    7680.0, 0.5);
}

TEST (WarpLookupTest, ExtrapolationUsesLastSegment)
{
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { ContentTick{ units::ticks (0.0) },   TimelineTick{ units::ticks (0.0) }   },
    { ContentTick{ units::ticks (100.0) }, TimelineTick{ units::ticks (200.0) } },
    { ContentTick{ units::ticks (200.0) }, TimelineTick{ units::ticks (600.0) } }
  };
  EXPECT_NEAR (
    warp_lookup (warp_points, ContentTick{ units::ticks (300.0) }).asDouble (),
    1000.0, 0.5);
}

TEST (WarpLookupTest, ExtrapolationBeforeFirstPoint)
{
  // Forward slope 2.0: {0, 0}, {100, 200}
  // Content -50 before first point.
  // Extrapolation: delta = 0 + 2.0 * (-50 - 0) = -100
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { ContentTick{ units::ticks (0.0) },   TimelineTick{ units::ticks (0.0) }   },
    { ContentTick{ units::ticks (100.0) }, TimelineTick{ units::ticks (200.0) } }
  };
  EXPECT_NEAR (
    warp_lookup (warp_points, ContentTick{ units::ticks (-50.0) }).asDouble (),
    -100.0, 0.5);
}

// --- Reverse warp lookup tests ---

TEST (ReverseWarpLookupTest, EmptyIsIdentity)
{
  EXPECT_DOUBLE_EQ (
    reverse_warp_lookup ({}, TimelineTick{ units::ticks (1234.0) }).asDouble (),
    1234.0);
}

TEST (ReverseWarpLookupTest, TwoWarpPointsLinear)
{
  // Forward: content 3840 → timeline 7680 (2x stretch)
  // Reverse: timeline 7680 → content 3840
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { ContentTick{ units::ticks (0.0) },    TimelineTick{ units::ticks (0.0) } },
    { ContentTick{ units::ticks (7680.0) },
     TimelineTick{ units::ticks (15360.0) }                                    }
  };
  EXPECT_NEAR (
    reverse_warp_lookup (warp_points, TimelineTick{ units::ticks (7680.0) })
      .asDouble (),
    3840.0, 0.5);
}

TEST (ReverseWarpLookupTest, ExtrapolationUsesLastSegment)
{
  // Forward: content 300 → timeline 1000 (slope 2 on last segment: 600→1000)
  // Reverse: timeline 1000 → content 300
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { ContentTick{ units::ticks (0.0) },   TimelineTick{ units::ticks (0.0) }   },
    { ContentTick{ units::ticks (100.0) }, TimelineTick{ units::ticks (200.0) } },
    { ContentTick{ units::ticks (200.0) }, TimelineTick{ units::ticks (600.0) } }
  };
  EXPECT_NEAR (
    reverse_warp_lookup (warp_points, TimelineTick{ units::ticks (1000.0) })
      .asDouble (),
    300.0, 0.5);
}

TEST (ReverseWarpLookupTest, ExtrapolationBeforeFirstPoint)
{
  // Identity warp: {0, 0}, {100, 100} — slope 1.0
  // Delta -50 is before the first point's delta (0).
  // Extrapolation: content = 0 + 1.0 * (-50 - 0) = -50
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { ContentTick{ units::ticks (0.0) },   TimelineTick{ units::ticks (0.0) }   },
    { ContentTick{ units::ticks (100.0) }, TimelineTick{ units::ticks (100.0) } }
  };
  EXPECT_NEAR (
    reverse_warp_lookup (warp_points, TimelineTick{ units::ticks (-50.0) })
      .asDouble (),
    -50.0, 0.5);
}

TEST (ReverseWarpLookupTest, ExtrapolationBeforeFirstPointNonIdentity)
{
  // Forward slope 2.0: {0, 0}, {100, 200}
  // Reverse slope = dContent/dDelta = 100/200 = 0.5
  // Delta -100: content = 0 + 0.5 * (-100 - 0) = -50
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { ContentTick{ units::ticks (0.0) },   TimelineTick{ units::ticks (0.0) }   },
    { ContentTick{ units::ticks (100.0) }, TimelineTick{ units::ticks (200.0) } }
  };
  EXPECT_NEAR (
    reverse_warp_lookup (warp_points, TimelineTick{ units::ticks (-100.0) })
      .asDouble (),
    -50.0, 0.5);
}

TEST (ReverseWarpLookupTest, MultiSegmentDifferentSlopes)
{
  // Segment 1: content 0-100, delta 0-200 (slope 2)
  // Segment 2: content 100-200, delta 200-600 (slope 4)
  // Timeline 400 is in segment 2: (400-200)/(600-200) = 0.5
  // Content = 100 + 0.5 * (200-100) = 150
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { ContentTick{ units::ticks (0.0) },   TimelineTick{ units::ticks (0.0) }   },
    { ContentTick{ units::ticks (100.0) }, TimelineTick{ units::ticks (200.0) } },
    { ContentTick{ units::ticks (200.0) }, TimelineTick{ units::ticks (600.0) } }
  };
  EXPECT_NEAR (
    reverse_warp_lookup (warp_points, TimelineTick{ units::ticks (400.0) })
      .asDouble (),
    150.0, 0.5);
}

TEST_F (ContentTimeWarpTest, TimelineToContentRoundtrip)
{
  warp_->configure_as_source (units::bpm (120.0));

  // Roundtrip: content → timeline → content should return ~same value
  for (const double ct : { 0.0, 100.0, 1000.0, 3840.0, 7680.0 })
    {
      const auto content = ContentTick{ units::ticks (ct) };
      const auto timeline = warp_->contentToTimeline (content);
      const auto recovered = warp_->timelineToContent (timeline);
      EXPECT_NEAR (recovered.asDouble (), ct, 1.0)
        << "Roundtrip failed for content tick " << ct;
    }
}

TEST_F (ContentTimeWarpTest, TimelineToContentWithNonZeroPosition)
{
  pos_->setTicks (1920.0);
  warp_->configure_as_source (units::bpm (120.0));

  // A timeline position of clip_start + 3840 content ticks
  // should map back to ~3840 content ticks.
  const auto content = ContentTick{ units::ticks (3840.0) };
  const auto timeline = warp_->contentToTimeline (content);
  const auto recovered = warp_->timelineToContent (timeline);
  EXPECT_NEAR (recovered.asDouble (), 3840.0, 5.0);
}

TEST_F (ContentTimeWarpTest, TimelineToContentProjectModeIdentity)
{
  pos_->setTicks (1920.0);
  warp_->configure_as_project ();

  // Identity warp: timeline position == content position + clip_start
  const auto timeline = TimelineTick{ units::ticks (1920.0 + 500.0) };
  const auto recovered = warp_->timelineToContent (timeline);
  EXPECT_NEAR (recovered.asDouble (), 500.0, 0.5);
}

TEST_F (ContentTimeWarpTest, ProjectModeTimelineToContentBeforeClipStart)
{
  pos_->setTicks (1920.0);
  warp_->configure_as_project (units::bpm (120.0));

  // Timeline position before clip start.
  // Identity: content = timeline - clip_start = 0 - 1920 = -1920
  const auto timeline = TimelineTick{ units::ticks (0.0) };
  const auto recovered = warp_->timelineToContent (timeline);
  EXPECT_NEAR (recovered.asDouble (), -1920.0, 0.5);
}

// Source mode: timelineToContent computes the exact inverse via the tempo
// map, covering positions beyond the clip's warp-point range.
TEST_F (ContentTimeWarpTest, SourceModeTimelineToContentExactBeyondWarpRange)
{
  // Tempo halves at tick 1920 — BEYOND the short clip (length 960).
  tempo_map_wrapper_->addTempoEvent (
    1920, 60.0, TempoEventWrapper::CurveType::Constant);

  pos_->setTicks (0.0);
  len_->setTicks (960.0);
  warp_->configure_as_source (units::bpm (60.0));

  // Warp is non-identity (source 60 ≠ project 120).
  ASSERT_FALSE (warp_->is_identity ());
  // Warp points only cover content [0, 960] at constant 120 BPM.
  ASSERT_EQ (warp_->warpPoints ().size (), 2u);

  // Content tick 1920 at source BPM 60 = 2.0 seconds.
  // Timeline: 0-1s at 120 BPM = 1920 ticks, 1-2s at 60 BPM = 960 ticks.
  // Total timeline delta = 2880.
  const auto content_secs =
    ContentTick{ units::ticks (1920.0) }.asQuantity () / units::bpm (60.0);
  const auto target_timeline = tempo_map_->seconds_to_tick (content_secs);

  // Expected: 2.0s reverse-mapped through the tempo map = 1920 content ticks.
  const auto recovered =
    warp_->timelineToContent (TimelineTick{ target_timeline });
  EXPECT_NEAR (recovered.asDouble (), 1920.0, 0.5)
    << "Expected exact reverse mapping beyond the warp range";
}

// Source mode: the QML-facing relative pair agrees with the absolute pair
// beyond the warp range — the relative delta equals (contentToTimeline - clip
// position), and the reverse recovers the content position.
TEST_F (ContentTimeWarpTest, SourceModeRelativePairMatchesAbsoluteBeyondRange)
{
  // Tempo halves at tick 1920 — BEYOND the short clip (length 960).
  tempo_map_wrapper_->addTempoEvent (
    1920, 60.0, TempoEventWrapper::CurveType::Constant);

  pos_->setTicks (0.0);
  len_->setTicks (960.0);
  warp_->configure_as_source (units::bpm (60.0));
  ASSERT_EQ (warp_->warpPoints ().size (), 2u);

  // Content 1920 is beyond the clip; warp points only cover [0, 960].
  // Absolute: contentToTimeline(1920) at source 60 = 2.0s → timeline 2880.
  const double abs_delta =
    warp_->contentToTimeline (ContentTick{ units::ticks (1920.0) }).asDouble ()
    - pos_->asTick ().asDouble ();

  // Relative delta equals the absolute conversion minus the clip position.
  const auto rel_delta = warp_->contentToTimelineTicksRelative (
    ContentTick{ units::ticks (1920.0) });
  EXPECT_NEAR (rel_delta.asDouble (), abs_delta, 0.5);

  // Reverse relative lookup recovers the content position.
  const auto recovered = warp_->timelineTicksRelativeToContent (rel_delta);
  EXPECT_NEAR (recovered.asDouble (), 1920.0, 0.5);

  // The double QML-facing overloads agree with the strong-typed API.
  EXPECT_NEAR (
    warp_->contentToTimelineTicksRelative (1920.0), rel_delta.asDouble (), 1e-9);
  EXPECT_NEAR (
    warp_->timelineTicksRelativeToContent (rel_delta.asDouble ()),
    recovered.asDouble (), 1e-9);
}

TEST_F (ContentTimeWarpTest, LinearRampProducesDenseWarpPoints)
{
  // Add a Linear tempo change within the clip span.
  tempo_map_wrapper_->addTempoEvent (
    1920, 180.0, TempoEventWrapper::CurveType::Linear);
  len_->setTicks (7680.0);
  warp_->configure_as_source (units::bpm (120.0));

  // With a Linear segment from tick 1920 to the next boundary,
  // rebuild() should produce dense warp points (~50ms cadence).
  // At 120 BPM, factor = 2 * 960 = 1920 ticks/sec.
  // Stride = 0.05 * 1920 = 96 ticks.
  // The Linear segment spans many ticks, so we expect significantly
  // more than just boundary points.
  EXPECT_GT (warp_->warpPoints ().size (), 5u);
}

TEST_F (ContentTimeWarpTest, IsIdentityProjectMode)
{
  warp_->configure_as_project ();
  EXPECT_TRUE (warp_->is_identity ());
}

TEST_F (ContentTimeWarpTest, IsIdentitySameBpm)
{
  warp_->configure_as_source (units::bpm (120.0));
  // Source BPM == project BPM (120), constant tempo -> identity
  EXPECT_TRUE (warp_->is_identity ());
}

TEST_F (ContentTimeWarpTest, IsNotIdentityDifferentBpm)
{
  warp_->configure_as_source (units::bpm (60.0));
  // Source BPM 60 != project BPM 120 -> stretched
  EXPECT_FALSE (warp_->is_identity ());
}

// In Project mode with source BPM, moving the clip changes which tempo events
// fall inside it, so mapChanged MUST fire to trigger cache invalidation.
TEST_F (
  ContentTimeWarpTest,
  ProjectModeEmitsMapChangedOnPositionMoveWithSourceBpm)
{
  warp_->configure_as_project (units::bpm (120.0));
  QSignalSpy spy (warp_.get (), &ContentTimeWarp::mapChanged);
  pos_->setTicks (1920.0);
  EXPECT_GE (spy.count (), 1);
}

// Project mode without source BPM (MIDI clips, unknown BPM): empty identity
// warp mapping.
TEST_F (ContentTimeWarpTest, ProjectModeEmptyPointsWithoutSourceBpm)
{
  warp_->configure_as_project ();
  EXPECT_TRUE (warp_->warpPoints ().empty ());
  EXPECT_TRUE (warp_->is_identity ());
}

// Project mode without source BPM: the warp mapping stays empty regardless of
// position or tempo, so neither change produces mapChanged.
TEST_F (
  ContentTimeWarpTest,
  ProjectModeNoSourceBpmDoesNotEmitMapChangedOnPositionMove)
{
  warp_->configure_as_project ();
  QSignalSpy pos_spy (warp_.get (), &ContentTimeWarp::mapChanged);
  pos_->setTicks (1920.0);
  EXPECT_EQ (pos_spy.count (), 0);

  // Tempo changes leave the mapping unchanged as well.
  QSignalSpy tempo_spy (warp_.get (), &ContentTimeWarp::mapChanged);
  tempo_map_wrapper_->addTempoEvent (
    3840, 240.0, TempoEventWrapper::CurveType::Constant);
  EXPECT_EQ (tempo_spy.count (), 0);
}

//--- Project mode tempo-boundary warp point tests ---

// Musical-mode clip (Project mode with source BPM) must emit identity warp
// points at each tempo-event boundary so that to_time_warp_map can derive
// per-segment sample-space stretch anchors that follow the tempo curve.
TEST_F (ContentTimeWarpTest, ProjectModeWithSourceBpmEmitsBoundaryPoints)
{
  // Add a tempo change inside the clip span (tick 3840 = bar 3 at 120 BPM).
  tempo_map_wrapper_->addTempoEvent (
    3840, 240.0, TempoEventWrapper::CurveType::Constant);

  warp_->configure_as_project (units::bpm (120.0));

  // With tempo events at tick 0 and 3840, plus the terminal at length 7680:
  // boundary at content 0, boundary at content 3840, terminal at content 7680.
  // That's 3 warp points (no dense sampling needed for Constant curve).
  EXPECT_EQ (warp_->warpPoints ().size (), 3u);
}

// All Project-mode warp points must have delta == content (identity in tick
// space). The tempo-following happens in sample space via to_time_warp_map.
TEST_F (ContentTimeWarpTest, ProjectModeBoundaryPointsAreIdentity)
{
  tempo_map_wrapper_->addTempoEvent (
    3840, 240.0, TempoEventWrapper::CurveType::Constant);

  warp_->configure_as_project (units::bpm (120.0));

  for (const auto &wp : warp_->warpPoints ())
    {
      EXPECT_NEAR (
        wp.content_ticks.asDouble (), wp.timeline_delta_ticks.asDouble (), 0.5)
        << "Project mode warp point must be identity (delta == content)";
    }
  EXPECT_TRUE (warp_->is_identity ());
}

// Project mode with a Linear tempo ramp must emit dense warp points to
// approximate the curve in sample space.
TEST_F (ContentTimeWarpTest, ProjectModeLinearRampProducesDensePoints)
{
  tempo_map_wrapper_->addTempoEvent (
    1920, 180.0, TempoEventWrapper::CurveType::Linear);

  warp_->configure_as_project (units::bpm (120.0));

  // Linear ramp → dense sampling at ~50ms cadence.
  EXPECT_GT (warp_->warpPoints ().size (), 5u);

  // All points still identity.
  EXPECT_TRUE (warp_->is_identity ());
}

// Project mode with source BPM must rebuild when tempo events change.
TEST_F (ContentTimeWarpTest, ProjectModeWithSourceBpmRespondsToTempoChange)
{
  warp_->configure_as_project (units::bpm (120.0));
  const auto points_before = warp_->warpPoints ().size ();

  tempo_map_wrapper_->addTempoEvent (
    3840, 240.0, TempoEventWrapper::CurveType::Constant);

  const auto points_after = warp_->warpPoints ().size ();
  EXPECT_GT (points_after, points_before);
}

// Musical-mode clip (Project mode) with constant tempo matching source BPM:
// sample-space identity → no stretch needed.
TEST_F (ContentTimeWarpTest, ProjectModeConstantTempoIsSampleSpaceIdentity)
{
  warp_->configure_as_project (units::bpm (120.0));

  auto warp_map = to_time_warp_map (
    warp_->warpPoints (), *tempo_map_, TimelineTick{ units::ticks (0.0) },
    units::bpm (120.0), units::samples (176400));
  EXPECT_TRUE (is_sample_space_identity (warp_map.anchors));
}

// Musical-mode clip (Project mode) with tempo automation: sample-space
// anchors trace the tempo curve → stretch IS needed.
TEST_F (ContentTimeWarpTest, ProjectModeTempoAutomationNeedsStretch)
{
  tempo_map_wrapper_->addTempoEvent (
    3840, 240.0, TempoEventWrapper::CurveType::Constant);

  warp_->configure_as_project (units::bpm (120.0));

  auto warp_map = to_time_warp_map (
    warp_->warpPoints (), *tempo_map_, TimelineTick{ units::ticks (0.0) },
    units::bpm (120.0), units::samples (176400));
  EXPECT_FALSE (is_sample_space_identity (warp_map.anchors));
}

//--- Warped mode tests ---

TEST_F (ContentTimeWarpTest, WarpedModeUsesUserMarkers)
{
  // User markers: content 0→0, content 3840→7680 (2x slope)
  std::vector<ContentTimeWarp::WarpPoint> markers = {
    { ContentTick{ units::ticks (0.0) },    TimelineTick{ units::ticks (0.0) } },
    { ContentTick{ units::ticks (3840.0) },
     TimelineTick{ units::ticks (7680.0) }                                     },
  };
  warp_->configure_as_warped (units::bpm (120.0), markers);

  // Terminal point at length=7680 with 2x slope: delta = 7680 + (7680-3840)*2 =
  // 15360
  const auto wp = warp_->warpPoints ();
  EXPECT_GE (wp.size (), 3u);
  // Mid-point should match the user marker (not tempo-derived)
  EXPECT_NEAR (wp[1].content_ticks.asDouble (), 3840.0, 0.5);
  EXPECT_NEAR (wp[1].timeline_delta_ticks.asDouble (), 7680.0, 0.5);
}

TEST_F (ContentTimeWarpTest, WarpedModePrependsOriginIfMissing)
{
  std::vector<ContentTimeWarp::WarpPoint> markers = {
    { ContentTick{ units::ticks (1000.0) },
     TimelineTick{ units::ticks (2000.0) } },
  };
  warp_->configure_as_warped (units::bpm (120.0), markers);

  const auto wp = warp_->warpPoints ();
  EXPECT_NEAR (wp.front ().content_ticks.asDouble (), 0.0, 0.5);
  EXPECT_NEAR (wp.front ().timeline_delta_ticks.asDouble (), 0.0, 0.5);
}

TEST_F (ContentTimeWarpTest, WarpedModeAppendsTerminalAtLength)
{
  // Length is 7680 (from fixture)
  std::vector<ContentTimeWarp::WarpPoint> markers = {
    { ContentTick{ units::ticks (0.0) },    TimelineTick{ units::ticks (0.0) } },
    { ContentTick{ units::ticks (1000.0) },
     TimelineTick{ units::ticks (2000.0) }                                     },
  };
  warp_->configure_as_warped (units::bpm (120.0), markers);

  const auto wp = warp_->warpPoints ();
  EXPECT_NEAR (wp.back ().content_ticks.asDouble (), 7680.0, 0.5);
  // Extrapolate from slope 2: delta = 2000 + (7680-1000)*2 = 15360
  EXPECT_NEAR (wp.back ().timeline_delta_ticks.asDouble (), 15360.0, 0.5);
}

TEST_F (ContentTimeWarpTest, WarpedModeEmptyMarkersIsIdentity)
{
  warp_->configure_as_warped (units::bpm (120.0), {});
  const auto wp = warp_->warpPoints ();
  // Only {0,0} and {length, length}
  EXPECT_EQ (wp.size (), 2u);
  EXPECT_TRUE (warp_->is_identity ());
}

} // namespace zrythm::dsp
