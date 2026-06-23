// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/atomic_position.h"
#include "dsp/atomic_position_qml_adapter.h"
#include "dsp/content_time_warp.h"
#include "dsp/tempo_map.h"
#include "dsp/tempo_map_qml_adapter.h"
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

    auto tcf =
      AtomicPosition::TimeConversionFunctions::from_tempo_map (*tempo_map_);
    pos_ = std::make_unique<AtomicPosition> (*tcf);
    pos_->set_ticks (units::ticks (0.0));
    pos_adapter_ = std::make_unique<AtomicPositionQmlAdapter> (
      *pos_, *tempo_map_wrapper_, std::nullopt);

    len_ = std::make_unique<AtomicPosition> (*tcf);
    len_->set_ticks (units::ticks (7680.0));
    len_adapter_ = std::make_unique<AtomicPositionQmlAdapter> (
      *len_, *tempo_map_wrapper_, std::nullopt);

    warp_ = std::make_unique<ContentTimeWarp> (
      *tempo_map_wrapper_, pos_adapter_.get (), len_adapter_.get ());
  }

  std::unique_ptr<TempoMap>                 tempo_map_;
  std::unique_ptr<TempoMapWrapper>          tempo_map_wrapper_;
  std::unique_ptr<AtomicPosition>           pos_;
  std::unique_ptr<AtomicPositionQmlAdapter> pos_adapter_;
  std::unique_ptr<AtomicPosition>           len_;
  std::unique_ptr<AtomicPositionQmlAdapter> len_adapter_;
  std::unique_ptr<ContentTimeWarp>          warp_;
};

TEST_F (ContentTimeWarpTest, ProjectModeIdentity)
{
  warp_->configure_as_project ();
  EXPECT_TRUE (warp_->warpPoints ().empty ());
  pos_->set_ticks (units::ticks (1000.0));
  EXPECT_NEAR (
    warp_->content_to_timeline_ticks (units::ticks (500.0)).in (units::ticks),
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
    warp_->content_to_timeline_ticks (units::ticks (7680.0)).in (units::ticks),
    7680.0, 5.0);
}

TEST_F (ContentTimeWarpTest, SourceModeDoubleTempo)
{
  tempo_map_ = std::make_unique<TempoMap> (units::sample_rate (44100.0));
  tempo_map_->add_tempo_event (
    units::ticks (0), units::bpm (240.0), TempoMap::CurveType::Constant);
  tempo_map_wrapper_ = std::make_unique<TempoMapWrapper> (*tempo_map_);

  auto tcf =
    AtomicPosition::TimeConversionFunctions::from_tempo_map (*tempo_map_);
  pos_ = std::make_unique<AtomicPosition> (*tcf);
  pos_->set_ticks (units::ticks (0.0));
  pos_adapter_ = std::make_unique<AtomicPositionQmlAdapter> (
    *pos_, *tempo_map_wrapper_, std::nullopt);
  len_ = std::make_unique<AtomicPosition> (*tcf);
  len_->set_ticks (units::ticks (7680.0));
  len_adapter_ = std::make_unique<AtomicPositionQmlAdapter> (
    *len_, *tempo_map_wrapper_, std::nullopt);
  warp_ = std::make_unique<ContentTimeWarp> (
    *tempo_map_wrapper_, pos_adapter_.get (), len_adapter_.get ());
  warp_->configure_as_source (units::bpm (120.0));

  EXPECT_NEAR (
    warp_->content_to_timeline_ticks (units::ticks (7680.0)).in (units::ticks),
    15360.0, 5.0);
}

TEST_F (ContentTimeWarpTest, ContentToTimelineSamplesAbsolute)
{
  warp_->configure_as_source (units::bpm (120.0));
  EXPECT_NEAR (
    warp_->content_to_timeline_samples (units::ticks (7680.0))
      .in (units::samples),
    176400, 100);
}

TEST_F (ContentTimeWarpTest, SourceModeWithNonZeroPosition)
{
  pos_->set_ticks (units::ticks (1920.0));
  warp_->configure_as_source (units::bpm (120.0));
  // pos=1920, source BPM=120, project BPM=120 -> identity delta
  // content_to_timeline_ticks(7680) = 1920 + 7680 = 9600
  EXPECT_NEAR (
    warp_->content_to_timeline_ticks (units::ticks (7680.0)).in (units::ticks),
    9600.0, 5.0);
}

TEST_F (ContentTimeWarpTest, TempoChangeTriggersRebuild)
{
  warp_->configure_as_source (units::bpm (120.0));
  const auto before =
    warp_->content_to_timeline_ticks (units::ticks (7680.0)).in (units::ticks);

  tempo_map_wrapper_->addTempoEvent (
    0, 240.0, TempoEventWrapper::CurveType::Constant);

  EXPECT_NE (
    before,
    warp_->content_to_timeline_ticks (units::ticks (7680.0)).in (units::ticks));
}

TEST_F (ContentTimeWarpTest, ProjectModeIgnoresTempoChange)
{
  warp_->configure_as_project ();
  const auto before =
    warp_->content_to_timeline_ticks (units::ticks (7680.0)).in (units::ticks);

  tempo_map_wrapper_->addTempoEvent (
    0, 240.0, TempoEventWrapper::CurveType::Constant);

  EXPECT_EQ (
    before,
    warp_->content_to_timeline_ticks (units::ticks (7680.0)).in (units::ticks));
}

TEST_F (ContentTimeWarpTest, LengthChangeTriggersRebuild)
{
  warp_->configure_as_source (units::bpm (120.0));
  EXPECT_FALSE (warp_->warpPoints ().empty ());
  len_adapter_->setTicks (3840.0);
  EXPECT_FALSE (warp_->warpPoints ().empty ());
}

TEST (WarpLookupTest, EmptyIsIdentity)
{
  EXPECT_DOUBLE_EQ (
    warp_lookup ({}, units::ticks (1234.0)).in (units::ticks), 1234.0);
}

TEST (WarpLookupTest, TwoWarpPointsLinear)
{
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { units::ticks (0.0),    units::ticks (0.0)     },
    { units::ticks (7680.0), units::ticks (15360.0) }
  };
  EXPECT_NEAR (
    warp_lookup (warp_points, units::ticks (3840.0)).in (units::ticks), 7680.0,
    0.5);
}

TEST (WarpLookupTest, ExtrapolationUsesLastSegment)
{
  std::vector<ContentTimeWarp::WarpPoint> warp_points = {
    { units::ticks (0.0),   units::ticks (0.0)   },
    { units::ticks (100.0), units::ticks (200.0) },
    { units::ticks (200.0), units::ticks (600.0) }
  };
  EXPECT_NEAR (
    warp_lookup (warp_points, units::ticks (300.0)).in (units::ticks), 1000.0,
    0.5);
}

TEST_F (ContentTimeWarpTest, LinearRampProducesDenseWarpPoints)
{
  // Add a Linear tempo change within the region span.
  tempo_map_wrapper_->addTempoEvent (
    1920, 180.0, TempoEventWrapper::CurveType::Linear);
  len_adapter_->setTicks (7680.0);
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

// In Project mode, timelineLengthTicks is position-independent
// ((pos + identity(length)) - pos == length). Moving the region must NOT
// emit mapChanged — it only causes redundant QML refreshes and cache
// invalidation (already handled by ArrangerObject's positionChanged).
TEST_F (ContentTimeWarpTest, ProjectModeDoesNotEmitMapChangedOnPositionMove)
{
  warp_->configure_as_project (units::bpm (120.0));
  QSignalSpy spy (warp_.get (), &ContentTimeWarp::mapChanged);
  pos_adapter_->setTicks (1920.0);
  EXPECT_EQ (spy.count (), 0);
}

//--- Warped mode tests ---

TEST_F (ContentTimeWarpTest, WarpedModeUsesUserMarkers)
{
  // User markers: content 0→0, content 3840→7680 (2x slope)
  std::vector<ContentTimeWarp::WarpPoint> markers = {
    { units::ticks (0.0),    units::ticks (0.0)    },
    { units::ticks (3840.0), units::ticks (7680.0) },
  };
  warp_->configure_as_warped (units::bpm (120.0), markers);

  // Terminal point at length=7680 with 2x slope: delta = 7680 + (7680-3840)*2 =
  // 15360
  const auto wp = warp_->warpPoints ();
  EXPECT_GE (wp.size (), 3u);
  // Mid-point should match the user marker (not tempo-derived)
  EXPECT_NEAR (wp[1].content_ticks.in (units::ticks), 3840.0, 0.5);
  EXPECT_NEAR (wp[1].timeline_delta_ticks.in (units::ticks), 7680.0, 0.5);
}

TEST_F (ContentTimeWarpTest, WarpedModePrependsOriginIfMissing)
{
  std::vector<ContentTimeWarp::WarpPoint> markers = {
    { units::ticks (1000.0), units::ticks (2000.0) },
  };
  warp_->configure_as_warped (units::bpm (120.0), markers);

  const auto wp = warp_->warpPoints ();
  EXPECT_NEAR (wp.front ().content_ticks.in (units::ticks), 0.0, 0.5);
  EXPECT_NEAR (wp.front ().timeline_delta_ticks.in (units::ticks), 0.0, 0.5);
}

TEST_F (ContentTimeWarpTest, WarpedModeAppendsTerminalAtLength)
{
  // Length is 7680 (from fixture)
  std::vector<ContentTimeWarp::WarpPoint> markers = {
    { units::ticks (0.0),    units::ticks (0.0)    },
    { units::ticks (1000.0), units::ticks (2000.0) },
  };
  warp_->configure_as_warped (units::bpm (120.0), markers);

  const auto wp = warp_->warpPoints ();
  EXPECT_NEAR (wp.back ().content_ticks.in (units::ticks), 7680.0, 0.5);
  // Extrapolate from slope 2: delta = 2000 + (7680-1000)*2 = 15360
  EXPECT_NEAR (wp.back ().timeline_delta_ticks.in (units::ticks), 15360.0, 0.5);
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
