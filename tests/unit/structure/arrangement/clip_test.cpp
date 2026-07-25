// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/content_time_warp.h"
#include "dsp/tick_types.h"
#include "structure/arrangement/arranger_object_all.h"
#include "utils/object_registry.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{

// Clip is abstract; exercise its loop-range behaviour through MidiClip (the
// lightest concrete subclass).
class ClipTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    clip = std::make_unique<MidiClip> (*tempo_map_wrapper, registry, nullptr);

    clip->position ()->setTicks (0.0);
    clip->length ()->setTicks (200.0);
    // Freshly built clips track their bounds: positions are (0, 0, length).
    ASSERT_TRUE (clip->trackBounds ());
    ASSERT_DOUBLE_EQ (clip->loopEndPosition ()->ticks (), 200.0);
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  utils::ObjectRegistry                 registry;
  std::unique_ptr<MidiClip>             clip;
};

// set_loop_range writes an interdependent triplet atomically, bypassing the
// per-position clamping constraints. A sequential setTicks would clamp
// loop_start (180) against the current loop_end (200) -> 199, corrupting it.
// Here loop_end is unchanged so the clamp wouldn't fire; the next test covers
// the genuinely corrupting case.
TEST_F (ClipTest, SetLoopRangeAppliesTargetVerbatim)
{
  clip->set_loop_range (
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (180.0) },
    dsp::ContentTick{ units::ticks (200.0) });

  EXPECT_DOUBLE_EQ (clip->clipStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (clip->loopStartPosition ()->ticks (), 180.0);
  EXPECT_DOUBLE_EQ (clip->loopEndPosition ()->ticks (), 200.0);
  // set_loop_range disables length-tracking so the explicit values stick; it
  // does not re-engage it.
  EXPECT_FALSE (clip->trackBounds ());
}

// First shrink the range so loop_end is small, then expand loop_start beyond
// it: under sequential setTicks loop_start would be clamped to loop_end-1.
// set_loop_range must apply the target verbatim.
TEST_F (ClipTest, SetLoopRangeExpandsLoopStartBeyondCurrentLoopEnd)
{
  // Bring the clip to a state where loop_end is small (50).
  clip->set_loop_range (
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (50.0) });
  ASSERT_DOUBLE_EQ (clip->loopEndPosition ()->ticks (), 50.0);

  // Now expand loop_start beyond the current loop_end.
  clip->set_loop_range (
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (40.0) },
    dsp::ContentTick{ units::ticks (200.0) });

  EXPECT_DOUBLE_EQ (clip->clipStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (clip->loopStartPosition ()->ticks (), 40.0);
  EXPECT_DOUBLE_EQ (clip->loopEndPosition ()->ticks (), 200.0);
}

// set_loop_range also handles shrinking below all current values, which no
// fixed setTicks ordering can do correctly.
TEST_F (ClipTest, SetLoopRangeShrinksBelowCurrentValues)
{
  clip->set_loop_range (
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (10.0) });

  EXPECT_DOUBLE_EQ (clip->clipStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (clip->loopStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (clip->loopEndPosition ()->ticks (), 10.0);
}

// Out-of-range inputs are clamped holistically to the loop invariants, so the
// clip never ends up with loop_end <= loop_start or negative positions.
TEST_F (ClipTest, SetLoopRangeClampsOutOfRangeInputs)
{
  // loop_end below loop_start + 1 is raised to loop_start + 1.
  clip->set_loop_range (
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (100.0) },
    dsp::ContentTick{ units::ticks (50.0) });

  EXPECT_DOUBLE_EQ (clip->clipStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (clip->loopStartPosition ()->ticks (), 100.0);
  EXPECT_DOUBLE_EQ (clip->loopEndPosition ()->ticks (), 101.0);

  // Negative inputs clamp to 0; loop_end follows max(clip_start, loop_start).
  clip->set_loop_range (
    dsp::ContentTick{ units::ticks (-10.0) },
    dsp::ContentTick{ units::ticks (-5.0) },
    dsp::ContentTick{ units::ticks (-1.0) });

  EXPECT_DOUBLE_EQ (clip->clipStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (clip->loopStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (clip->loopEndPosition ()->ticks (), 1.0);
}

// content_position_at_timeline returns the played content position: the
// unwound content position offset by the clip start, wrapped into the loop
// range.
TEST_F (ClipTest, ContentPositionAtTimelineWithClipStartAndLoop)
{
  clip->position ()->setTicks (2000.0);
  clip->length ()->setTicks (4000.0);
  clip->set_loop_range (
    dsp::ContentTick{ units::ticks (500.0) },
    dsp::ContentTick{ units::ticks (1000.0) },
    dsp::ContentTick{ units::ticks (3000.0) });

  // Inside the intro leg: 500 + 400 = 900, no wrap
  EXPECT_DOUBLE_EQ (
    clip
      ->content_position_at_timeline (dsp::TimelineTick{ units::ticks (2400.0) })
      .asDouble (),
    900.0);

  // Past the first loop end: 500 + 2500 = 3000 >= loop_end (3000), so wrap
  // by the loop size (2000) -> 1000
  EXPECT_DOUBLE_EQ (
    clip
      ->content_position_at_timeline (dsp::TimelineTick{ units::ticks (4500.0) })
      .asDouble (),
    1000.0);
}

// With a non-linear warp, the wrap must happen in the unwound content domain
// (matching playback), not in the warped timeline-delta domain.
TEST_F (ClipTest, ContentPositionAtTimelineWithWarpedLoop)
{
  clip->position ()->setTicks (0.0);
  clip->length ()->setTicks (4000.0);
  clip->set_loop_range (
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (1000.0) });
  // Kinked warp: content [0,1000] -> delta [0,2000] (2x slope), then 0.5x:
  // content [1000,2000] -> delta [2000,2500], content [2000,4000] -> delta
  // [2500,3500]
  const std::vector<dsp::ContentTimeWarp::WarpPoint> markers = {
    { dsp::ContentTick{ units::ticks (0.0) },
     dsp::TimelineTick{ units::ticks (0.0) }    },
    { dsp::ContentTick{ units::ticks (1000.0) },
     dsp::TimelineTick{ units::ticks (2000.0) } },
    { dsp::ContentTick{ units::ticks (2000.0) },
     dsp::TimelineTick{ units::ticks (2500.0) } },
  };
  clip->contentWarp ()->configure_as_warped (units::bpm (120.0), markers);

  // Unwound content position at timeline 3000 is 3000: the start of the 4th
  // loop leg, so the played content position is the loop start (0)
  EXPECT_DOUBLE_EQ (
    clip
      ->content_position_at_timeline (dsp::TimelineTick{ units::ticks (3000.0) })
      .asDouble (),
    0.0);
}

// Sample-domain probes quantize to whole samples and can land slightly
// outside the clip's tick-domain span; positions within a 1-sample tolerance
// are clamped into the span.
TEST_F (ClipTest, ContentPositionAtTimelineClampsWithinSampleTolerance)
{
  // At the default 120 BPM / 44.1kHz, tick 17 maps to 390.46875 samples,
  // which rounds down to 390; converting 390 samples back yields ~16.98
  // ticks - just below the clip's start tick
  clip->position ()->setTicks (17.0);
  clip->length ()->setTicks (960.0);

  const auto probe = tempo_map->samples_to_tick (units::samples (390));
  ASSERT_LT (probe.asDouble (), 17.0);

  // Clamped to the clip start: played position is the content start (0)
  EXPECT_DOUBLE_EQ (clip->content_position_at_timeline (probe).asDouble (), 0.0);

  // Exactly at the clip's end (outside the [start, end) span but within the
  // tolerance): clamped to just before the end
  EXPECT_NEAR (
    clip
      ->content_position_at_timeline (dsp::TimelineTick{ units::ticks (977.0) })
      .asDouble (),
    960.0, 1e-6);
}

} // namespace zrythm::structure::arrangement
