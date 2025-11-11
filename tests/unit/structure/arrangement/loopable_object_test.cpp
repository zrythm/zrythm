// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/atomic_position.h"
#include "dsp/atomic_position_qml_adapter.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/bounded_object.h"
#include "structure/arrangement/loopable_object.h"

#include <QSignalSpy>

#include "helpers/mock_qobject.h"

#include "unit/dsp/atomic_position_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class ArrangerObjectLoopRangeTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    time_conversion_funcs = dsp::basic_conversion_providers ();
    start_position =
      std::make_unique<dsp::AtomicPosition> (*time_conversion_funcs);
    parent = std::make_unique<MockQObject> ();
    start_position_adapter = std::make_unique<dsp::AtomicPositionQmlAdapter> (
      *start_position, std::nullopt, parent.get ());

    // Create bounded object for loop range
    bounds = std::make_unique<ArrangerObjectBounds> (*start_position_adapter);

    // Create loop range with proper parenting
    loop_range = std::make_unique<ArrangerObjectLoopRange> (*bounds);

    // Set position and length
    start_position_adapter->setSamples (1000);
    bounds->length ()->setSamples (5000); // Object spans 1000-6000 samples
  }

  std::unique_ptr<dsp::AtomicPosition::TimeConversionFunctions>
                                                 time_conversion_funcs;
  std::unique_ptr<dsp::AtomicPosition>           start_position;
  std::unique_ptr<MockQObject>                   parent;
  std::unique_ptr<dsp::AtomicPositionQmlAdapter> start_position_adapter;
  std::unique_ptr<ArrangerObjectBounds>          bounds;
  std::unique_ptr<ArrangerObjectLoopRange>       loop_range;
};

// Test initial state
TEST_F (ArrangerObjectLoopRangeTest, InitialState)
{
  EXPECT_EQ (loop_range->clipStartPosition ()->samples (), 0);
  EXPECT_EQ (loop_range->loopStartPosition ()->samples (), 0);
  EXPECT_EQ (loop_range->loopEndPosition ()->samples (), 5000); // Should track
                                                                // bounds length
  EXPECT_TRUE (loop_range->trackBounds ()); // Default should be true
  EXPECT_FALSE (loop_range->looped ());
}

// Test trackBounds property and behavior
TEST_F (ArrangerObjectLoopRangeTest, TrackBoundsBehavior)
{
  // Default behavior: all positions track bounds when enabled
  bounds->length ()->setSamples (6000);
  EXPECT_EQ (loop_range->clipStartPosition ()->samples (), 0); // Should be 0
  EXPECT_EQ (loop_range->loopStartPosition ()->samples (), 0); // Should be 0
  EXPECT_EQ (
    loop_range->loopEndPosition ()->samples (), 6000); // Should track bounds

  // Change positions manually
  loop_range->clipStartPosition ()->setSamples (100);
  loop_range->loopStartPosition ()->setSamples (200);
  loop_range->loopEndPosition ()->setSamples (4000);
  EXPECT_EQ (loop_range->clipStartPosition ()->samples (), 100);
  EXPECT_EQ (loop_range->loopStartPosition ()->samples (), 200);
  EXPECT_EQ (loop_range->loopEndPosition ()->samples (), 4000);

  // When trackBounds is enabled, changing bounds length should reset all
  // positions
  bounds->length ()->setSamples (7000);
  EXPECT_EQ (
    loop_range->clipStartPosition ()->samples (), 0); // Should reset to 0
  EXPECT_EQ (
    loop_range->loopStartPosition ()->samples (), 0); // Should reset to 0
  EXPECT_EQ (
    loop_range->loopEndPosition ()->samples (), 7000); // Should track bounds

  // Disable trackBounds
  loop_range->setTrackBounds (false);
  EXPECT_FALSE (loop_range->trackBounds ());

  // Changing bounds length should not affect positions
  bounds->length ()->setSamples (8000);
  EXPECT_EQ (loop_range->clipStartPosition ()->samples (), 0);
  EXPECT_EQ (loop_range->loopStartPosition ()->samples (), 0);
  EXPECT_EQ (loop_range->loopEndPosition ()->samples (), 7000);

  // Manually change positions while tracking disabled
  loop_range->clipStartPosition ()->setSamples (150);
  loop_range->loopStartPosition ()->setSamples (250);
  loop_range->loopEndPosition ()->setSamples (7500);
  EXPECT_EQ (loop_range->clipStartPosition ()->samples (), 150);
  EXPECT_EQ (loop_range->loopStartPosition ()->samples (), 250);
  EXPECT_EQ (loop_range->loopEndPosition ()->samples (), 7500);

  // Re-enable trackBounds
  loop_range->setTrackBounds (true);
  EXPECT_TRUE (loop_range->trackBounds ());
  EXPECT_EQ (
    loop_range->clipStartPosition ()->samples (), 0); // Should reset to 0
  EXPECT_EQ (
    loop_range->loopStartPosition ()->samples (), 0); // Should reset to 0
  EXPECT_EQ (
    loop_range->loopEndPosition ()->samples (), 8000); // Should track bounds
}

// Test is_looped detection
TEST_F (ArrangerObjectLoopRangeTest, IsLooped)
{
  // Default state (not looped)
  EXPECT_FALSE (loop_range->looped ());

  // Modify clip start
  loop_range->clipStartPosition ()->setSamples (100);
  EXPECT_TRUE (loop_range->looped ());
  loop_range->clipStartPosition ()->setSamples (0);

  // Modify loop start
  loop_range->loopStartPosition ()->setSamples (100);
  EXPECT_TRUE (loop_range->looped ());
  loop_range->loopStartPosition ()->setSamples (0);

  // Modify loop end (with tracking enabled)
  loop_range->loopEndPosition ()->setSamples (4000);
  EXPECT_TRUE (loop_range->looped ());

  // Reset to default
  loop_range->setTrackBounds (true);
  bounds->length ()->setSamples (5000);
  EXPECT_EQ (loop_range->clipStartPosition ()->samples (), 0);
  EXPECT_EQ (loop_range->loopStartPosition ()->samples (), 0);
  EXPECT_EQ (loop_range->loopEndPosition ()->samples (), 5000);
  EXPECT_FALSE (loop_range->looped ());
}

// Test length calculations
TEST_F (ArrangerObjectLoopRangeTest, LengthCalculations)
{
  // Disable tracking for manual setup
  loop_range->setTrackBounds (false);

  loop_range->loopStartPosition ()->setSamples (1000);
  loop_range->loopEndPosition ()->setSamples (3000);

  EXPECT_DOUBLE_EQ (
    loop_range->get_loop_length_in_ticks ().in (units::ticks),
    time_conversion_funcs->samples_to_tick (units::samples (2000))
      .in (units::ticks));
  EXPECT_EQ (loop_range->get_loop_length_in_frames (), units::samples (2000));
}

// Test position operations
TEST_F (ArrangerObjectLoopRangeTest, PositionOperations)
{
  // Disable tracking for manual setup
  loop_range->setTrackBounds (false);

  loop_range->clipStartPosition ()->setSamples (500);
  loop_range->loopStartPosition ()->setSamples (1000);
  loop_range->loopEndPosition ()->setSamples (2000);

  EXPECT_EQ (loop_range->clipStartPosition ()->samples (), 500);
  EXPECT_EQ (loop_range->loopStartPosition ()->samples (), 1000);
  EXPECT_EQ (loop_range->loopEndPosition ()->samples (), 2000);
}

// Test loop count calculation
TEST_F (ArrangerObjectLoopRangeTest, GetNumLoops)
{
  // Setup loop: 1000-2000 samples (loop length = 1000)
  loop_range->setTrackBounds (false);
  loop_range->loopStartPosition ()->setSamples (1000);
  loop_range->loopEndPosition ()->setSamples (2000);
  loop_range->clipStartPosition ()->setSamples (0);

  // Full loops
  bounds->length ()->setSamples (3000); // 2 loops from 1000 to 3000 (1000*2)
  EXPECT_EQ (loop_range->get_num_loops (false), 2);

  // Partial loop
  bounds->length ()->setSamples (3500); // 2 full + 0.5 partial
  EXPECT_EQ (loop_range->get_num_loops (false), 2);
  EXPECT_EQ (loop_range->get_num_loops (true), 3);

  // With clip start offset
  loop_range->clipStartPosition ()->setSamples (500);
  bounds->length ()->setSamples (3500); // 3500 - 500 = 3000 playable samples
  EXPECT_EQ (loop_range->get_num_loops (false), 3);
}

// Test serialization/deserialization
TEST_F (ArrangerObjectLoopRangeTest, Serialization)
{
  // Set initial state
  loop_range->setTrackBounds (false);
  loop_range->clipStartPosition ()->setSamples (500);
  loop_range->loopStartPosition ()->setSamples (1000);
  loop_range->loopEndPosition ()->setSamples (2000);

  // Serialize
  nlohmann::json j;
  to_json (j, *loop_range);

  // Create new object from serialized data
  dsp::AtomicPosition           new_start_pos (*time_conversion_funcs);
  dsp::AtomicPositionQmlAdapter new_start_adapter (
    new_start_pos, std::nullopt, parent.get ());
  ArrangerObjectBounds    new_bounds (new_start_adapter);
  ArrangerObjectLoopRange new_loop_range (new_bounds);
  from_json (j, new_loop_range);

  // Verify state
  EXPECT_EQ (new_loop_range.clipStartPosition ()->samples (), 500);
  EXPECT_EQ (new_loop_range.loopStartPosition ()->samples (), 1000);
  EXPECT_EQ (new_loop_range.loopEndPosition ()->samples (), 2000);
  EXPECT_FALSE (new_loop_range.trackBounds ());
}

// Test trackBoundsChanged signal
TEST_F (ArrangerObjectLoopRangeTest, TrackBoundsSignal)
{
  QSignalSpy spy (
    loop_range.get (), &ArrangerObjectLoopRange::trackBoundsChanged);

  // Initially trackBounds is true, changing to false should emit signal
  loop_range->setTrackBounds (false);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (spy.at (0).at (0).toBool (), false);

  // Changing back to true should emit signal again
  loop_range->setTrackBounds (true);
  EXPECT_EQ (spy.count (), 2);
  EXPECT_EQ (spy.at (1).at (0).toBool (), true);

  // Setting to same value should not emit signal
  loop_range->setTrackBounds (true);
  EXPECT_EQ (spy.count (), 2); // Should still be 2
}

// Test loopedChanged signal
TEST_F (ArrangerObjectLoopRangeTest, LoopedSignal)
{
  QSignalSpy spy (loop_range.get (), &ArrangerObjectLoopRange::loopedChanged);

  // Initially not looped, so changing to looped should emit signal
  loop_range->clipStartPosition ()->setSamples (100);
  EXPECT_EQ (spy.count (), 1);
  EXPECT_TRUE (loop_range->looped ());

  // Reset to not looped state
  loop_range->clipStartPosition ()->setSamples (0);
  EXPECT_EQ (spy.count (), 2);
  EXPECT_FALSE (loop_range->looped ());

  // Test with loop start position
  loop_range->loopStartPosition ()->setSamples (100);
  EXPECT_EQ (spy.count (), 3);
  EXPECT_TRUE (loop_range->looped ());

  // Reset
  loop_range->loopStartPosition ()->setSamples (0);
  EXPECT_EQ (spy.count (), 4);
  EXPECT_FALSE (loop_range->looped ());

  // Test with loop end position
  loop_range->loopEndPosition ()->setSamples (4000);
  EXPECT_EQ (spy.count (), 5);
  EXPECT_TRUE (loop_range->looped ());
}

// Test edge cases
TEST_F (ArrangerObjectLoopRangeTest, EdgeCases)
{
  // Zero-length loop
  loop_range->setTrackBounds (false);
  loop_range->loopStartPosition ()->setSamples (0);
  loop_range->loopEndPosition ()->setSamples (0);
  // Loop end gets clamped to 1 tick
  EXPECT_GT (loop_range->loopEndPosition ()->samples (), 0);
  EXPECT_DOUBLE_EQ (loop_range->loopEndPosition ()->ticks (), 1);
  EXPECT_GT (
    loop_range->get_loop_length_in_frames (),
    units::samples (static_cast<int64_t> (0)));
  EXPECT_DOUBLE_EQ (
    loop_range->get_loop_length_in_ticks ().in (units::ticks), 1.0);
  EXPECT_GT (loop_range->get_num_loops (false), 0);

  // Negative positions (should be clamped to 0)
  loop_range->clipStartPosition ()->setSamples (-100);
  loop_range->loopStartPosition ()->setSamples (-200);
  loop_range->loopEndPosition ()->setSamples (-50);
  EXPECT_EQ (loop_range->clipStartPosition ()->samples (), 0);
  EXPECT_EQ (loop_range->loopStartPosition ()->samples (), 0);
  // Loop end gets clamped to a minimum value even when set to negative
  EXPECT_GT (loop_range->loopEndPosition ()->samples (), 0);
  EXPECT_DOUBLE_EQ (loop_range->loopEndPosition ()->ticks (), 1.0);

  // Loop start > loop end (should be constrained)
  loop_range->loopEndPosition ()->setSamples (3000); // Set loop end first
  loop_range->loopStartPosition ()->setSamples (
    4000); // Try to set loop start > loop end
  // Loop start gets clamped to be less than loop end (with some minimum gap)
  EXPECT_LT (
    loop_range->loopStartPosition ()->samples (),
    loop_range->loopEndPosition ()->samples ());
  // Loop length should be 1 tick when start and end are too close
  EXPECT_GT (loop_range->get_loop_length_in_frames (), units::samples (0));
  EXPECT_DOUBLE_EQ (
    loop_range->get_loop_length_in_ticks ().in (units::ticks), 1.0);

  // Enabling tracking with invalid bounds
  bounds->length ()->setSamples (-100);
  loop_range->setTrackBounds (true);
  EXPECT_EQ (loop_range->clipStartPosition ()->samples (), 0); // Should be 0
  EXPECT_EQ (loop_range->loopStartPosition ()->samples (), 0); // Should be 0
  // Loop end should be at least 1 tick greater than other positions
  EXPECT_GT (loop_range->loopEndPosition ()->ticks (), 0);
}

} // namespace zrythm::structure::arrangement
