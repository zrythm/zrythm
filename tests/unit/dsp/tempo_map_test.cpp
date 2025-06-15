// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{
class TempoMapTest : public ::testing::Test
{
protected:
  void SetUp () override { map = std::make_unique<TempoMap> (44100.0); }

  std::unique_ptr<TempoMap> map;
};

// Test initial state
TEST_F (TempoMapTest, InitialState)
{
  EXPECT_EQ (map->getEvents ().size (), 1);
  EXPECT_EQ (map->getEvents ()[0].tick, 0);
  EXPECT_DOUBLE_EQ (map->getEvents ()[0].bpm, 120.0);
  EXPECT_EQ (map->getEvents ()[0].curve, TempoMap::CurveType::Constant);

  EXPECT_EQ (map->getTimeSignatureEvents ().size (), 1);
  EXPECT_EQ (map->getTimeSignatureEvents ()[0].tick, 0);
  EXPECT_EQ (map->getTimeSignatureEvents ()[0].numerator, 4);
  EXPECT_EQ (map->getTimeSignatureEvents ()[0].denominator, 4);
}

// Test tempo event management
TEST_F (TempoMapTest, TempoEventManagement)
{
  // Add constant tempo event
  map->addEvent (1920, 140.0, TempoMap::CurveType::Constant);
  EXPECT_EQ (map->getEvents ().size (), 2);

  // Add linear tempo event
  map->addEvent (3840, 160.0, TempoMap::CurveType::Linear);
  EXPECT_EQ (map->getEvents ().size (), 3);

  // Update existing event
  map->addEvent (1920, 150.0, TempoMap::CurveType::Linear);
  EXPECT_EQ (map->getEvents ().size (), 3);
  EXPECT_DOUBLE_EQ (map->getEvents ()[1].bpm, 150.0);
  EXPECT_EQ (map->getEvents ()[1].curve, TempoMap::CurveType::Linear);

  // Remove event
  map->removeEvent (3840);
  EXPECT_EQ (map->getEvents ().size (), 2);
}

// Test time signature management
TEST_F (TempoMapTest, TimeSignatureManagement)
{
  // Add time signature
  map->addTimeSignatureEvent (1920, 3, 4);
  EXPECT_EQ (map->getTimeSignatureEvents ().size (), 2);

  // Update existing
  map->addTimeSignatureEvent (1920, 5, 8);
  EXPECT_EQ (map->getTimeSignatureEvents ().size (), 2);
  EXPECT_EQ (map->getTimeSignatureEvents ()[1].numerator, 5);
  EXPECT_EQ (map->getTimeSignatureEvents ()[1].denominator, 8);

  // Remove
  map->removeTimeSignatureEvent (1920);
  EXPECT_EQ (map->getTimeSignatureEvents ().size (), 1);
}

// Test constant tempo conversions
TEST_F (TempoMapTest, ConstantTempoConversions)
{
  // 120 BPM = 0.5 seconds per beat
  // 1 beat = 960 ticks -> 1 tick = 0.5/960 seconds

  // Test tick to seconds
  EXPECT_DOUBLE_EQ (map->tickToSeconds (0z), 0.0);
  EXPECT_DOUBLE_EQ (map->tickToSeconds (960z), 0.5);
  EXPECT_DOUBLE_EQ (map->tickToSeconds (1920z), 1.0);

  // Test seconds to tick
  EXPECT_DOUBLE_EQ (map->secondsToTick (0.0), 0.0);
  EXPECT_DOUBLE_EQ (map->secondsToTick (0.5), 960.0);
  EXPECT_DOUBLE_EQ (map->secondsToTick (1.0), 1920.0);

  // Test samples conversion
  EXPECT_DOUBLE_EQ (map->tickToSamples (960z), 0.5 * 44100.0);
  EXPECT_DOUBLE_EQ (map->samplesToTick (0.5 * 44100.0), 960.0);
}

// Test linear tempo ramp conversions
TEST_F (TempoMapTest, LinearTempoRamp)
{
  // Create a linear ramp segment from 120 to 180 BPM over 4 beats
  const int64_t startRamp = 0;
  const int64_t endRamp = 4 * 960; // 4 beats
  map->addEvent (startRamp, 120.0, TempoMap::CurveType::Linear);
  map->addEvent (endRamp, 180.0, TempoMap::CurveType::Constant);

  // Test midpoint (2 beats in) should be 150 BPM
  const int64_t midTick = endRamp / 2;

  // Calculate expected time using integral formula
  const double segmentTicks = static_cast<double> (endRamp - startRamp);
  const double fraction = 0.5;
  const double currentBpm = 120.0 + fraction * (180.0 - 120.0);
  const double expectedTime =
    (60.0 * segmentTicks) / (960.0 * (180.0 - 120.0))
    * std::log (currentBpm / 120.0);

  EXPECT_NEAR (map->tickToSeconds (midTick), expectedTime, 1e-8);

  // Test reverse conversion
  EXPECT_NEAR (map->secondsToTick (expectedTime), midTick, 1e-6);
}

// Test musical position conversions
TEST_F (TempoMapTest, MusicalPositionConversion)
{
  // Test default 4/4 time
  auto pos = map->tickToMusicalPosition (0);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tickToMusicalPosition (960); // Quarter note
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 2);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tickToMusicalPosition (240); // Sixteenth note
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 2);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tickToMusicalPosition (241); // Sixteenth +1 tick
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 2);
  EXPECT_EQ (pos.tick, 1);

  // Test reverse conversion
  EXPECT_EQ (map->musicalPositionToTick ({ 1, 1, 1, 0 }), 0);
  EXPECT_EQ (map->musicalPositionToTick ({ 1, 2, 1, 0 }), 960);
  EXPECT_EQ (map->musicalPositionToTick ({ 1, 1, 2, 0 }), 240);
  EXPECT_EQ (map->musicalPositionToTick ({ 1, 1, 2, 1 }), 241);
}

// Test time signature changes
TEST_F (TempoMapTest, TimeSignatureChanges)
{
  // Add time signatures
  map->addTimeSignatureEvent (0, 4, 4);         // Bar 1: 4/4
  const int64_t bar5Start = 4 * 4 * 960;        // Bar 5 start
  map->addTimeSignatureEvent (bar5Start, 3, 4); // Bar 5: 3/4
  const int64_t bar8Start =
    bar5Start + 3 * 3 * 960;                    // Bar 8 start (3 bars of 3/4)
  map->addTimeSignatureEvent (bar8Start, 7, 8); // Bar 8: 7/8

  // Test positions
  auto pos = map->tickToMusicalPosition (0);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tickToMusicalPosition (bar5Start - 1);
  EXPECT_EQ (pos.bar, 4);
  EXPECT_EQ (pos.beat, 4);
  EXPECT_EQ (pos.sixteenth, 4);
  EXPECT_EQ (pos.tick, (960 / 4) - 1);

  // Bar 5 (3/4)
  pos = map->tickToMusicalPosition (bar5Start);
  EXPECT_EQ (pos.bar, 5);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tickToMusicalPosition (bar5Start + 2 * 960);
  EXPECT_EQ (pos.bar, 5);
  EXPECT_EQ (pos.beat, 3);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // beats per bar (numerator) is 3, so we should move to a new bar
  pos = map->tickToMusicalPosition (bar5Start + 3 * 960);
  EXPECT_EQ (pos.bar, 6);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // Bar 8 (7/8)
  pos = map->tickToMusicalPosition (bar8Start);
  EXPECT_EQ (pos.bar, 8);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // Test non-quarter-note beat unit
  // Bar 8 + 1/4 note (2 beat units) + 1/16th note (1 sixteenth) + 1 tick
  pos = map->tickToMusicalPosition (bar8Start + 960 + 960 / 4 + 1);
  EXPECT_EQ (pos.bar, 8);
  EXPECT_EQ (pos.beat, 3);
  EXPECT_EQ (pos.sixteenth, 2);
  EXPECT_EQ (pos.tick, 1);

  // Test reverse conversions
  EXPECT_EQ (map->musicalPositionToTick ({ 5, 1, 1, 0 }), bar5Start);
  EXPECT_EQ (map->musicalPositionToTick ({ 5, 3, 1, 0 }), bar5Start + 2 * 960);
  EXPECT_EQ (map->musicalPositionToTick ({ 8, 1, 1, 0 }), bar8Start);
  EXPECT_EQ (
    map->musicalPositionToTick ({ 8, 3, 2, 1 }), bar8Start + 960 + 960 / 4 + 1);
}

// MultiSegmentLinearRamp test
TEST_F (TempoMapTest, MultiSegmentLinearRamp)
{
  // Setup tempo events
  map->addEvent (960, 120.0, TempoMap::CurveType::Linear);
  map->addEvent (1920, 180.0, TempoMap::CurveType::Constant);

  // Test before ramp
  EXPECT_DOUBLE_EQ (map->tickToSeconds (480z), 0.25);

  // Test start of ramp
  EXPECT_DOUBLE_EQ (map->tickToSeconds (960z), 0.5);

  // Test midpoint of ramp (150 BPM)
  const double midTime = map->tickToSeconds (1440z);
  const double expectedMidTime =
    0.5 + (60.0 * 960) / (960.0 * 60.0) * std::log (150.0 / 120.0);
  EXPECT_NEAR (midTime, expectedMidTime, 1e-8);

  // Test end of ramp
  const double endTime = map->tickToSeconds (1920z);
  const double expectedEndTime =
    0.5 + (60.0 * 960) / (960.0 * 60.0) * std::log (180.0 / 120.0);
  EXPECT_NEAR (endTime, expectedEndTime, 1e-8);

  // Test after ramp (480 ticks at 180 BPM)
  const double afterRampTime = endTime + (480.0 / 960.0) * (60.0 / 180.0);
  EXPECT_NEAR (map->tickToSeconds (2400z), afterRampTime, 1e-8);
}

// Test linear ramp as last event
TEST_F (TempoMapTest, LinearRampLastEvent)
{
  // Setup:
  // [0, 960): Constant 120 BPM
  // [960, ∞): Linear ramp 120 → ? BPM (should be constant 120 since no end point)
  map->addEvent (960, 120.0, TempoMap::CurveType::Linear);

  // Should be constant after 960 because no endpoint for ramp
  EXPECT_DOUBLE_EQ (map->tickToSeconds (960z), 0.5);
  EXPECT_DOUBLE_EQ (map->tickToSeconds (1440z), 0.5 + 0.25);
  EXPECT_DOUBLE_EQ (map->tickToSeconds (1920z), 0.5 + 0.5);
}

// Test edge cases
TEST_F (TempoMapTest, EdgeCases)
{
  // Zero and negative time
  EXPECT_DOUBLE_EQ (map->secondsToTick (0.0), 0.0);
  EXPECT_DOUBLE_EQ (map->secondsToTick (-1.0), 0.0);

  // Empty tempo map
  TempoMap emptyMap (960);
  emptyMap.removeEvent (0);
  EXPECT_DOUBLE_EQ (emptyMap.tickToSeconds (960z), 0.0);

  // Near-constant ramp
  map->addEvent (960, 120.001, TempoMap::CurveType::Linear);
  map->addEvent (1920, 120.002, TempoMap::CurveType::Constant);
  EXPECT_NEAR (map->tickToSeconds (1440z), 0.5 + 0.25, 1e-5);

  // Invalid positions
  EXPECT_THROW (
    map->musicalPositionToTick ({ 0, 1, 1, 0 }), std::invalid_argument);
  EXPECT_THROW (
    map->musicalPositionToTick ({ 1, 0, 1, 0 }), std::invalid_argument);
  EXPECT_THROW (
    map->musicalPositionToTick ({ 1, 1, 0, 0 }), std::invalid_argument);
  EXPECT_THROW (
    map->musicalPositionToTick ({ 1, 1, 1, -1 }), std::invalid_argument);

  // Position beyond last time signature
  EXPECT_GT (map->musicalPositionToTick ({ 100, 1, 1, 0 }), 0);
}

// Test fractional ticks
TEST_F (TempoMapTest, FractionalTicks)
{
  // Test constant tempo
  const double expectedTime = 480.5 * (0.5 / 960.0);
  EXPECT_DOUBLE_EQ (map->tickToSeconds (480.5), expectedTime);

  // Test reverse conversion
  EXPECT_NEAR (map->secondsToTick (expectedTime), 480.5, 1e-6);
}

// Test sample rate changes
TEST_F (TempoMapTest, SampleRateChanges)
{
  const double newRate = 48000.0;
  map->setSampleRate (newRate);

  EXPECT_DOUBLE_EQ (map->tickToSamples (960z), 0.5 * newRate);
  EXPECT_DOUBLE_EQ (map->samplesToTick (0.5 * newRate), 960.0);
}

// Test complex time signature with different beat units
TEST_F (TempoMapTest, ComplexTimeSignatures)
{
  map->addTimeSignatureEvent (0, 6, 8); // 6/8 time

  // end at 6 beats
  const auto bar1Ticks = static_cast<int64_t> (6 * (TempoMap::getPPQ () / 2));
  const int64_t bar1End = bar1Ticks - 1;

  auto pos = map->tickToMusicalPosition (0);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);

  pos = map->tickToMusicalPosition (bar1End);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 6);

  // Test 7/16 time
  map->addTimeSignatureEvent (bar1Ticks, 7, 16);

  // end at 7 beats
  const auto bar2Ticks = static_cast<int64_t> (7 * (TempoMap::getPPQ () / 4));
  const int64_t bar2Start = bar1Ticks;
  const int64_t bar2End = bar2Start + bar2Ticks - 1;

  pos = map->tickToMusicalPosition (bar2Start);
  EXPECT_EQ (pos.bar, 2);
  EXPECT_EQ (pos.beat, 1);

  pos = map->tickToMusicalPosition (bar2End);
  EXPECT_EQ (pos.bar, 2);
  EXPECT_EQ (pos.beat, 7);
}

// Test tempo and time signature interaction
TEST_F (TempoMapTest, TempoAndTimeSignatureInteraction)
{
  // Add time signature change at bar 5
  const int64_t bar5Start = 4 * 4 * 960; // Bar 5 start
  map->addTimeSignatureEvent (bar5Start, 3, 4);

  // Add tempo change at bar 3
  const int64_t bar3Start = 2 * 4 * 960; // Bar 3 start
  map->addEvent (bar3Start, 140.0, TempoMap::CurveType::Constant);

  // Test position at bar 5
  auto pos = map->tickToMusicalPosition (bar5Start);
  EXPECT_EQ (pos.bar, 5);
  EXPECT_EQ (pos.beat, 1);

  // Calculate expected time
  const double bars1_2 = 8.0;                      // 2 bars * 4 beats
  const double bars3_4 = 8.0;                      // 2 bars * 4 beats
  const double time1_2 = bars1_2 * (60.0 / 120.0); // 4.0 seconds
  const double time3_4 = bars3_4 * (60.0 / 140.0); // ~3.42857 seconds
  const double expectedTime = time1_2 + time3_4;

  EXPECT_NEAR (map->tickToSeconds (bar5Start), expectedTime, 1e-5);
}
}
