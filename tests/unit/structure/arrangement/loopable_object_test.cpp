// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/atomic_position.h"
#include "dsp/atomic_position_qml_adapter.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/bounded_object.h"
#include "structure/arrangement/loopable_object.h"

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
      *start_position, true, parent.get ());

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
  EXPECT_TRUE (loop_range->trackLength ()); // Default should be true
  EXPECT_FALSE (loop_range->is_looped ());
}

// Test trackLength property and behavior
TEST_F (ArrangerObjectLoopRangeTest, TrackLengthBehavior)
{
  // Default behavior: loop end tracks bounds length
  bounds->length ()->setSamples (6000);
  EXPECT_EQ (loop_range->loopEndPosition ()->samples (), 6000);

  // Change loop end manually
  loop_range->loopEndPosition ()->setSamples (4000);
  EXPECT_EQ (loop_range->loopEndPosition ()->samples (), 4000);

  // When trackLength is enabled, changing bounds length should update loop end
  bounds->length ()->setSamples (7000);
  EXPECT_EQ (loop_range->loopEndPosition ()->samples (), 7000);

  // Disable trackLength
  loop_range->setTrackLength (false);
  EXPECT_FALSE (loop_range->trackLength ());

  // Changing bounds length should not affect loop end
  bounds->length ()->setSamples (8000);
  EXPECT_EQ (loop_range->loopEndPosition ()->samples (), 7000);

  // Manually change loop end while tracking disabled
  loop_range->loopEndPosition ()->setSamples (7500);
  EXPECT_EQ (loop_range->loopEndPosition ()->samples (), 7500);

  // Re-enable trackLength
  loop_range->setTrackLength (true);
  EXPECT_TRUE (loop_range->trackLength ());
  EXPECT_EQ (
    loop_range->loopEndPosition ()->samples (),
    8000); // Should update to current bounds length
}

// Test is_looped detection
TEST_F (ArrangerObjectLoopRangeTest, IsLooped)
{
  // Default state (not looped)
  EXPECT_FALSE (loop_range->is_looped ());

  // Modify clip start
  loop_range->clipStartPosition ()->setSamples (100);
  EXPECT_TRUE (loop_range->is_looped ());
  loop_range->clipStartPosition ()->setSamples (0);

  // Modify loop start
  loop_range->loopStartPosition ()->setSamples (100);
  EXPECT_TRUE (loop_range->is_looped ());
  loop_range->loopStartPosition ()->setSamples (0);

  // Modify loop end (with tracking enabled)
  loop_range->loopEndPosition ()->setSamples (4000);
  EXPECT_TRUE (loop_range->is_looped ());

  // Reset to default
  loop_range->setTrackLength (true);
  bounds->length ()->setSamples (5000);
  EXPECT_EQ (loop_range->loopEndPosition ()->samples (), 5000);
  EXPECT_FALSE (loop_range->is_looped ());
}

// Test length calculations
TEST_F (ArrangerObjectLoopRangeTest, LengthCalculations)
{
  // Disable tracking for manual setup
  loop_range->setTrackLength (false);

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
  loop_range->setTrackLength (false);

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
  loop_range->setTrackLength (false);
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
  loop_range->setTrackLength (false);
  loop_range->clipStartPosition ()->setSamples (500);
  loop_range->loopStartPosition ()->setSamples (1000);
  loop_range->loopEndPosition ()->setSamples (2000);

  // Serialize
  nlohmann::json j;
  to_json (j, *loop_range);

  // Create new object from serialized data
  dsp::AtomicPosition           new_start_pos (*time_conversion_funcs);
  dsp::AtomicPositionQmlAdapter new_start_adapter (
    new_start_pos, true, parent.get ());
  ArrangerObjectBounds    new_bounds (new_start_adapter);
  ArrangerObjectLoopRange new_loop_range (new_bounds);
  from_json (j, new_loop_range);

  // Verify state
  EXPECT_EQ (new_loop_range.clipStartPosition ()->samples (), 500);
  EXPECT_EQ (new_loop_range.loopStartPosition ()->samples (), 1000);
  EXPECT_EQ (new_loop_range.loopEndPosition ()->samples (), 2000);
  EXPECT_FALSE (new_loop_range.trackLength ());
}

// Test trackLengthChanged signal
TEST_F (ArrangerObjectLoopRangeTest, TrackLengthSignal)
{
  testing::MockFunction<void (bool)> mockTrackLengthChanged;

  QObject::connect (
    loop_range.get (), &ArrangerObjectLoopRange::trackLengthChanged,
    parent.get (), mockTrackLengthChanged.AsStdFunction ());

  EXPECT_CALL (mockTrackLengthChanged, Call (false)).Times (1);
  loop_range->setTrackLength (false);

  EXPECT_CALL (mockTrackLengthChanged, Call (true)).Times (1);
  loop_range->setTrackLength (true);
}

// Test edge cases
TEST_F (ArrangerObjectLoopRangeTest, EdgeCases)
{
  // Zero-length loop
  loop_range->setTrackLength (false);
  loop_range->loopStartPosition ()->setSamples (0);
  loop_range->loopEndPosition ()->setSamples (0);
  EXPECT_EQ (
    loop_range->get_loop_length_in_frames (),
    units::samples (static_cast<int64_t> (0)));
  EXPECT_EQ (loop_range->get_num_loops (false), 0);

  // Negative positions (should be clamped to 0)
  loop_range->clipStartPosition ()->setSamples (-100);
  loop_range->loopStartPosition ()->setSamples (-200);
  loop_range->loopEndPosition ()->setSamples (-50);
  EXPECT_EQ (loop_range->clipStartPosition ()->samples (), 0);
  EXPECT_EQ (loop_range->loopStartPosition ()->samples (), 0);
  EXPECT_EQ (loop_range->loopEndPosition ()->samples (), 0);

  // Loop start > loop end
  loop_range->loopStartPosition ()->setSamples (3000);
  loop_range->loopEndPosition ()->setSamples (1000);
  EXPECT_EQ (loop_range->get_loop_length_in_frames (), units::samples (0));

  // Enabling tracking with invalid bounds
  bounds->length ()->setSamples (-100);
  loop_range->setTrackLength (true);
  EXPECT_EQ (loop_range->loopEndPosition ()->samples (), 0); // Should clamp to 0
}

} // namespace zrythm::structure::arrangement
