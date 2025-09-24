// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{
class TempoMapTest : public ::testing::Test
{
protected:
  static constexpr auto SAMPLE_RATE = 44100.0 * mp_units::si::hertz;

  void SetUp () override { map = std::make_unique<TempoMap> (SAMPLE_RATE); }

  std::unique_ptr<TempoMap> map;
};

// Test initial state
TEST_F (TempoMapTest, InitialState)
{
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (0 * units::tick), 120.0);
  EXPECT_EQ (map->time_signature_at_tick (0 * units::tick).numerator, 4);
  EXPECT_EQ (map->time_signature_at_tick (0 * units::tick).denominator, 4);
}

// Test tempo event management
TEST_F (TempoMapTest, TempoEventManagement)
{
  // Add constant tempo event
  map->add_tempo_event (
    1920 * units::tick, 140.0, TempoMap::CurveType::Constant);
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (1920 * units::tick), 140.0);
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (1919 * units::tick), 120.0); // before the event

  // Add linear tempo event at 3840
  map->add_tempo_event (3840 * units::tick, 160.0, TempoMap::CurveType::Linear);
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (3840 * units::tick), 160.0);

  // Update existing event at 1920
  map->add_tempo_event (1920 * units::tick, 150.0, TempoMap::CurveType::Linear);
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (1920 * units::tick), 150.0);
  // Check the linear ramp: at the midpoint between 1920 and 3840, tempo should
  // be 155.0
  EXPECT_NEAR (map->tempo_at_tick (2880 * units::tick), 155.0, 1e-8);

  // Remove event at 3840
  map->remove_tempo_event (3840 * units::tick);
  // Now the tempo at 3840 should be the same as the previous event (150.0)
  // because the event was removed
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (3840 * units::tick), 150.0);
  // Also, the segment from 1920 onward should be constant, so at 2880 it should
  // be 150.0 (not ramping)
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (2880 * units::tick), 150.0);
}

// Test time signature management
TEST_F (TempoMapTest, TimeSignatureManagement)
{
  // Add time signature
  map->add_time_signature_event (1920 * units::tick, 3, 4);
  auto ts = map->time_signature_at_tick (1920 * units::tick);
  EXPECT_EQ (ts.numerator, 3);
  EXPECT_EQ (ts.denominator, 4);
  // Before the event, it should be 4/4
  ts = map->time_signature_at_tick (1919 * units::tick);
  EXPECT_EQ (ts.numerator, 4);
  EXPECT_EQ (ts.denominator, 4);

  // Update existing
  map->add_time_signature_event (1920 * units::tick, 5, 8);
  ts = map->time_signature_at_tick (1920 * units::tick);
  EXPECT_EQ (ts.numerator, 5);
  EXPECT_EQ (ts.denominator, 8);

  // Remove
  map->remove_time_signature_event (1920 * units::tick);
  ts = map->time_signature_at_tick (1920 * units::tick);
  EXPECT_EQ (ts.numerator, 4);
  EXPECT_EQ (ts.denominator, 4);
}

// Test constant tempo conversions
TEST_F (TempoMapTest, ConstantTempoConversions)
{
  // 120 BPM = 0.5 seconds per beat
  // 1 beat = 960 ticks -> 1 tick = 0.5/960 seconds

  // Test tick to seconds
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (0 * units::tick)
      .numerical_value_in (mp_units::si::second),
    0.0);
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (960 * units::tick)
      .numerical_value_in (mp_units::si::second),
    0.5);
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (1920 * units::tick)
      .numerical_value_in (mp_units::si::second),
    1.0);

  // Test seconds to tick
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (0.0s).numerical_value_in (units::tick), 0.0);
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (0.5s).numerical_value_in (units::tick), 960.0);
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (1.0s).numerical_value_in (units::tick), 1920.0);

  // Test samples conversion
  EXPECT_DOUBLE_EQ (
    map->tick_to_samples (960 * units::tick).numerical_value_in (units::sample),
    0.5 * 44100.0);
  EXPECT_DOUBLE_EQ (
    map->samples_to_tick (0.5 * 44100.0 * units::sample)
      .numerical_value_in (units::tick),
    960.0);
}

// Test linear tempo ramp conversions
TEST_F (TempoMapTest, LinearTempoRamp)
{
  // Create a linear ramp segment from 120 to 180 BPM over 4 beats
  const auto startRamp = 0 * units::tick;
  const auto endRamp = 4 * 960 * units::tick; // 4 beats
  map->add_tempo_event (startRamp, 120.0, TempoMap::CurveType::Linear);
  map->add_tempo_event (endRamp, 180.0, TempoMap::CurveType::Constant);

  // Test midpoint (2 beats in) should be 150 BPM
  const auto midTick = endRamp / 2;

  // Calculate expected time using integral formula
  const auto segmentTicks =
    static_cast<units::precise_tick_t> (endRamp - startRamp);
  const double fraction = 0.5;
  const double currentBpm = 120.0 + fraction * (180.0 - 120.0);
  const double expectedTime =
    (60.0 * segmentTicks.numerical_value_in (units::tick))
    / (960.0 * (180.0 - 120.0)) * std::log (currentBpm / 120.0);

  EXPECT_NEAR (
    map->tick_to_seconds (midTick).numerical_value_in (mp_units::si::second),
    expectedTime, 1e-8);

  // Test reverse conversion
  EXPECT_NEAR (
    map->seconds_to_tick (expectedTime * mp_units::si::second)
      .numerical_value_in (units::tick),
    midTick.numerical_value_in (units::tick), 1e-6);
}

// Test musical position conversions
TEST_F (TempoMapTest, MusicalPositionConversion)
{
  // Test default 4/4 time
  auto pos = map->tick_to_musical_position (0 * units::tick);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tick_to_musical_position (960 * units::tick); // Quarter note
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 2);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tick_to_musical_position (240 * units::tick); // Sixteenth note
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 2);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tick_to_musical_position (241 * units::tick); // Sixteenth +1 tick
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 2);
  EXPECT_EQ (pos.tick, 1);

  // Test reverse conversion
  EXPECT_EQ (
    map->musical_position_to_tick ({ 1, 1, 1, 0 })
      .numerical_value_in (units::tick),
    0);
  EXPECT_EQ (
    map->musical_position_to_tick ({ 1, 2, 1, 0 })
      .numerical_value_in (units::tick),
    960);
  EXPECT_EQ (
    map->musical_position_to_tick ({ 1, 1, 2, 0 })
      .numerical_value_in (units::tick),
    240);
  EXPECT_EQ (
    map->musical_position_to_tick ({ 1, 1, 2, 1 })
      .numerical_value_in (units::tick),
    241);
}

// Test time signature changes
TEST_F (TempoMapTest, TimeSignatureChanges)
{
  // Add time signatures
  map->add_time_signature_event (0 * units::tick, 4, 4); // Bar 1: 4/4
  const auto bar5Start = 4 * 4 * 960 * units::tick;      // Bar 5 start
  map->add_time_signature_event (bar5Start, 3, 4);       // Bar 5: 3/4
  const auto bar8Start =
    bar5Start + 3 * 3 * 960 * units::tick; // Bar 8 start (3 bars of 3/4)
  map->add_time_signature_event (bar8Start, 7, 8); // Bar 8: 7/8

  // Test positions
  auto pos = map->tick_to_musical_position (0 * units::tick);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tick_to_musical_position (bar5Start - 1 * units::tick);
  EXPECT_EQ (pos.bar, 4);
  EXPECT_EQ (pos.beat, 4);
  EXPECT_EQ (pos.sixteenth, 4);
  EXPECT_EQ (pos.tick, (960 / 4) - 1);

  // Bar 5 (3/4)
  pos = map->tick_to_musical_position (bar5Start);
  EXPECT_EQ (pos.bar, 5);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tick_to_musical_position (bar5Start + 2 * 960 * units::tick);
  EXPECT_EQ (pos.bar, 5);
  EXPECT_EQ (pos.beat, 3);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // beats per bar (numerator) is 3, so we should move to a new bar
  pos = map->tick_to_musical_position (bar5Start + 3 * 960 * units::tick);
  EXPECT_EQ (pos.bar, 6);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // Bar 8 (7/8)
  pos = map->tick_to_musical_position (bar8Start);
  EXPECT_EQ (pos.bar, 8);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // Test non-quarter-note beat unit
  // Bar 8 + 1/4 note (2 beat units) + 1/16th note (1 sixteenth) + 1 tick
  pos = map->tick_to_musical_position (
    bar8Start + 960 * units::tick + 960 * units::tick / 4 + 1 * units::tick);
  EXPECT_EQ (pos.bar, 8);
  EXPECT_EQ (pos.beat, 3);
  EXPECT_EQ (pos.sixteenth, 2);
  EXPECT_EQ (pos.tick, 1);

  // Test reverse conversions
  EXPECT_EQ (map->musical_position_to_tick ({ 5, 1, 1, 0 }), bar5Start);
  EXPECT_EQ (
    map->musical_position_to_tick ({ 5, 3, 1, 0 }),
    bar5Start + 2 * 960 * units::tick);
  EXPECT_EQ (map->musical_position_to_tick ({ 8, 1, 1, 0 }), bar8Start);
  EXPECT_EQ (
    map->musical_position_to_tick ({ 8, 3, 2, 1 }),
    bar8Start + 960 * units::tick + 960 * units::tick / 4 + 1 * units::tick);
}

// MultiSegmentLinearRamp test
TEST_F (TempoMapTest, MultiSegmentLinearRamp)
{
  // Setup tempo events
  map->add_tempo_event (960 * units::tick, 120.0, TempoMap::CurveType::Linear);
  map->add_tempo_event (
    1920 * units::tick, 180.0, TempoMap::CurveType::Constant);

  // Test before ramp
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (480 * units::tick)
      .numerical_value_in (mp_units::si::second),
    0.25);

  // Test start of ramp
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (960 * units::tick)
      .numerical_value_in (mp_units::si::second),
    0.5);

  // Test midpoint of ramp (150 BPM)
  const auto   midTime = map->tick_to_seconds (1440 * units::tick);
  const double expectedMidTime =
    0.5 + (60.0 * 960) / (960.0 * 60.0) * std::log (150.0 / 120.0);
  EXPECT_NEAR (
    midTime.numerical_value_in (mp_units::si::second), expectedMidTime, 1e-8);

  // Test end of ramp
  const auto   endTime = map->tick_to_seconds (1920 * units::tick);
  const double expectedEndTime =
    0.5 + (60.0 * 960) / (960.0 * 60.0) * std::log (180.0 / 120.0);
  EXPECT_NEAR (
    endTime.numerical_value_in (mp_units::si::second), expectedEndTime, 1e-8);

  // Test after ramp (480 ticks at 180 BPM)
  const double afterRampTime =
    endTime.numerical_value_in (mp_units::si::second)
    + (480.0 / 960.0) * (60.0 / 180.0);
  EXPECT_NEAR (
    map->tick_to_seconds (2400 * units::tick)
      .numerical_value_in (mp_units::si::second),
    afterRampTime, 1e-8);
}

// Test tempo lookups
TEST_F (TempoMapTest, TempoLookup)
{
  // Setup linear ramp from 120 to 180 BPM over 4 beats (3840 ticks)
  map->add_tempo_event (0 * units::tick, 120.0, TempoMap::CurveType::Linear);
  map->add_tempo_event (
    3840 * units::tick, 180.0, TempoMap::CurveType::Constant);

  // Test start of ramp
  auto tempo = map->tempo_at_tick (0 * units::tick);
  EXPECT_DOUBLE_EQ (tempo, 120.0);

  // Test 1/4 through ramp
  tempo = map->tempo_at_tick (960 * units::tick);
  EXPECT_NEAR (tempo, 135.0, 1e-8);

  // Test midpoint of ramp
  tempo = map->tempo_at_tick (1920 * units::tick);
  EXPECT_NEAR (tempo, 150.0, 1e-8);

  // Test 3/4 through ramp
  tempo = map->tempo_at_tick (2880 * units::tick);
  EXPECT_NEAR (tempo, 165.0, 1e-8);

  // Test end of ramp
  tempo = map->tempo_at_tick (3840 * units::tick);
  EXPECT_DOUBLE_EQ (tempo, 180.0);

  // Test after ramp
  tempo = map->tempo_at_tick (4800 * units::tick);
  EXPECT_DOUBLE_EQ (tempo, 180.0);
}

// Test linear ramp as last event
TEST_F (TempoMapTest, LinearRampLastEvent)
{
  // Setup:
  // [0, 960): Constant 120 BPM
  // [960, ∞): Linear ramp 120 → ? BPM (should be constant 120 since no end point)
  map->add_tempo_event (960 * units::tick, 120.0, TempoMap::CurveType::Linear);

  // Should be constant after 960 because no endpoint for ramp
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (960 * units::tick)
      .numerical_value_in (mp_units::si::second),
    0.5);
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (1440 * units::tick)
      .numerical_value_in (mp_units::si::second),
    0.5 + 0.25);
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (1920 * units::tick)
      .numerical_value_in (mp_units::si::second),
    0.5 + 0.5);
}

// Test edge cases
TEST_F (TempoMapTest, EdgeCases)
{
  // Zero and negative time
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (0.0s).numerical_value_in (units::tick), 0.0);
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (-1.0s).numerical_value_in (units::tick), 0.0);

  // Empty tempo map - default value active
  TempoMap emptyMap (960 * mp_units::si::hertz);
  emptyMap.remove_tempo_event (0 * units::tick);
  EXPECT_DOUBLE_EQ (
    emptyMap.tick_to_seconds (960 * units::tick)
      .numerical_value_in (mp_units::si::second),
    0.5);

  // Near-constant ramp
  map->add_tempo_event (960 * units::tick, 120.001, TempoMap::CurveType::Linear);
  map->add_tempo_event (
    1920 * units::tick, 120.002, TempoMap::CurveType::Constant);
  EXPECT_NEAR (
    map->tick_to_seconds (1440 * units::tick)
      .numerical_value_in (mp_units::si::second),
    0.5 + 0.25, 1e-5);

  // Invalid positions
  EXPECT_THROW (
    map->musical_position_to_tick ({ 0, 1, 1, 0 }), std::invalid_argument);
  EXPECT_THROW (
    map->musical_position_to_tick ({ 1, 0, 1, 0 }), std::invalid_argument);
  EXPECT_THROW (
    map->musical_position_to_tick ({ 1, 1, 0, 0 }), std::invalid_argument);
  EXPECT_THROW (
    map->musical_position_to_tick ({ 1, 1, 1, -1 }), std::invalid_argument);

  // Position beyond last time signature
  EXPECT_GT (
    map->musical_position_to_tick ({ 100, 1, 1, 0 })
      .numerical_value_in (units::tick),
    0);
}

// Test fractional ticks
TEST_F (TempoMapTest, FractionalTicks)
{
  // Test constant tempo
  const auto expectedTime = 480.5s * (0.5 / 960.0);
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (480.5 * units::tick)
      .numerical_value_in (mp_units::si::second),
    static_cast<double> (expectedTime.count ()));

  // Test reverse conversion
  EXPECT_NEAR (
    map->seconds_to_tick (expectedTime).numerical_value_in (units::tick), 480.5,
    1e-6);
}

// Test sample rate changes
TEST_F (TempoMapTest, SampleRateChanges)
{
  const double newRate = 48000.0;
  map->set_sample_rate (newRate * mp_units::si::hertz);

  EXPECT_DOUBLE_EQ (
    map->tick_to_samples (960 * units::tick).numerical_value_in (units::sample),
    0.5 * newRate);
  EXPECT_DOUBLE_EQ (
    map->samples_to_tick (0.5 * newRate * units::sample)
      .numerical_value_in (units::tick),
    960.0);
}

// Test complex time signature with different beat units
TEST_F (TempoMapTest, ComplexTimeSignatures)
{
  map->add_time_signature_event (0 * units::tick, 6, 8); // 6/8 time

  // end at 6 beats
  const auto bar1Ticks =
    static_cast<units::tick_t> (6 * units::tick * (TempoMap::get_ppq () / 2));
  const auto bar1End = bar1Ticks - (1 * units::tick);

  auto pos = map->tick_to_musical_position (0 * units::tick);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);

  pos = map->tick_to_musical_position (bar1End);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 6);

  // Test 7/16 time
  map->add_time_signature_event (bar1Ticks, 7, 16);

  // end at 7 beats
  const auto bar2Ticks =
    static_cast<units::tick_t> (7 * units::tick * (TempoMap::get_ppq () / 4));
  const auto bar2Start = bar1Ticks;
  const auto bar2End = bar2Start + bar2Ticks - 1 * units::tick;

  pos = map->tick_to_musical_position (bar2Start);
  EXPECT_EQ (pos.bar, 2);
  EXPECT_EQ (pos.beat, 1);

  pos = map->tick_to_musical_position (bar2End);
  EXPECT_EQ (pos.bar, 2);
  EXPECT_EQ (pos.beat, 7);
}

// Test time signature lookup
TEST_F (TempoMapTest, TimeSignatureLookup)
{
  // Default 4/4
  auto ts = map->time_signature_at_tick (0 * units::tick);
  EXPECT_EQ (ts.numerator, 4);
  EXPECT_EQ (ts.denominator, 4);

  // Add time signatures
  map->add_time_signature_event (
    1920 * units::tick, 3, 4); // Bar 3 (assuming 4/4)
  map->add_time_signature_event (3840 * units::tick, 5, 8); // Bar 5 (3/4)

  // Test before first change
  ts = map->time_signature_at_tick (0 * units::tick);
  EXPECT_EQ (ts.numerator, 4);
  EXPECT_EQ (ts.denominator, 4);

  // Test at first change point
  ts = map->time_signature_at_tick (1920 * units::tick);
  EXPECT_EQ (ts.numerator, 3);
  EXPECT_EQ (ts.denominator, 4);

  // Test between changes
  ts = map->time_signature_at_tick (2000 * units::tick);
  EXPECT_EQ (ts.numerator, 3);
  EXPECT_EQ (ts.denominator, 4);

  // Test at second change point
  ts = map->time_signature_at_tick (3840 * units::tick);
  EXPECT_EQ (ts.numerator, 5);
  EXPECT_EQ (ts.denominator, 8);

  // Test after last change
  ts = map->time_signature_at_tick (10000 * units::tick);
  EXPECT_EQ (ts.numerator, 5);
  EXPECT_EQ (ts.denominator, 8);

  // Test with empty map
  TempoMap emptyMap (SAMPLE_RATE);
  emptyMap.remove_time_signature_event (0 * units::tick);
  ts = emptyMap.time_signature_at_tick (0 * units::tick);
  EXPECT_EQ (ts.numerator, 4); // Default
  EXPECT_EQ (ts.denominator, 4);
}

// Test tempo and time signature interaction
TEST_F (TempoMapTest, TempoAndTimeSignatureInteraction)
{
  // Add time signature change at bar 5
  const auto bar5Start = 4 * 4 * 960 * units::tick; // Bar 5 start
  map->add_time_signature_event (bar5Start, 3, 4);

  // Add tempo change at bar 3
  const auto bar3Start = 2 * 4 * 960 * units::tick; // Bar 3 start
  map->add_tempo_event (bar3Start, 140.0, TempoMap::CurveType::Constant);

  // Test position at bar 5
  auto pos = map->tick_to_musical_position (bar5Start);
  EXPECT_EQ (pos.bar, 5);
  EXPECT_EQ (pos.beat, 1);

  // Calculate expected time
  const double bars1_2 = 8.0;                      // 2 bars * 4 beats
  const double bars3_4 = 8.0;                      // 2 bars * 4 beats
  const double time1_2 = bars1_2 * (60.0 / 120.0); // 4.0 seconds
  const double time3_4 = bars3_4 * (60.0 / 140.0); // ~3.42857 seconds
  const double expectedTime = time1_2 + time3_4;

  EXPECT_NEAR (
    map->tick_to_seconds (bar5Start).numerical_value_in (mp_units::si::second),
    expectedTime, 1e-5);
}

// Test serialization/deserialization
TEST_F (TempoMapTest, Serialization)
{
  // Add tempo and time signature events
  map->add_time_signature_event (1920 * units::tick, 3, 4);
  map->add_time_signature_event (3840 * units::tick, 5, 8);
  map->add_tempo_event (
    1920 * units::tick, 140.0, TempoMap::CurveType::Constant);
  map->add_tempo_event (3840 * units::tick, 160.0, TempoMap::CurveType::Linear);

  // Serialize to JSON
  nlohmann::json j;
  j = *map;

  // Deserialize to new object
  TempoMap deserialized_map{ SAMPLE_RATE };
  j.get_to (deserialized_map);

  // Verify by checking at various ticks
  std::vector<units::tick_t> test_ticks = {
    0 * units::tick,    960 * units::tick,  1920 * units::tick,
    2880 * units::tick, 3840 * units::tick, 4800 * units::tick
  };
  for (auto tick : test_ticks)
    {
      EXPECT_DOUBLE_EQ (
        map->tempo_at_tick (tick), deserialized_map.tempo_at_tick (tick));
      auto ts1 = map->time_signature_at_tick (tick);
      auto ts2 = deserialized_map.time_signature_at_tick (tick);
      EXPECT_EQ (ts1.numerator, ts2.numerator);
      EXPECT_EQ (ts1.denominator, ts2.denominator);
    }

  // Also test a conversion to be sure
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (2880 * units::tick)
      .numerical_value_in (mp_units::si::second),
    deserialized_map.tick_to_seconds (2880 * units::tick)
      .numerical_value_in (mp_units::si::second));
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (1.5s).numerical_value_in (units::tick),
    deserialized_map.seconds_to_tick (1.5s).numerical_value_in (units::tick));
}

// Test empty serialization
TEST_F (TempoMapTest, EmptySerialization)
{
  // Remove default events
  map->remove_tempo_event (0 * units::tick);
  map->remove_time_signature_event (0 * units::tick);

  // Serialize and deserialize
  nlohmann::json j;
  j = *map;
  TempoMap deserialized_map{ SAMPLE_RATE };
  j.get_to (deserialized_map);

  // Verify empty by checking default tempo and time signature
  EXPECT_DOUBLE_EQ (deserialized_map.tempo_at_tick (0 * units::tick), 120.0);
  auto ts = deserialized_map.time_signature_at_tick (0 * units::tick);
  EXPECT_EQ (ts.numerator, 4);
  EXPECT_EQ (ts.denominator, 4);
}

// Test samples to musical position conversion
TEST_F (TempoMapTest, SamplesToMusicalPosition)
{
  // Test basic conversion with default 120 BPM, 4/4 time
  auto pos = map->samples_to_musical_position (0 * units::sample);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // Test quarter note at 120 BPM (0.5 seconds = 22050 samples at 44.1kHz)
  const auto quarterNoteSamples = mp_units::value_cast<units::sample_t> (
    0.5 * units::sample * SAMPLE_RATE.numerical_value_in (mp_units::si::hertz));
  pos = map->samples_to_musical_position (quarterNoteSamples);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 2);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // Test half note (1 second = 44100 samples)
  const auto halfNoteSamples = mp_units::value_cast<units::sample_t> (
    1.0 * units::sample * SAMPLE_RATE.numerical_value_in (mp_units::si::hertz));
  pos = map->samples_to_musical_position (halfNoteSamples);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 3);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // Test full bar (2 seconds = 88200 samples)
  const auto fullBarSamples = mp_units::value_cast<units::sample_t> (
    2.0 * units::sample * SAMPLE_RATE.numerical_value_in (mp_units::si::hertz));
  pos = map->samples_to_musical_position (fullBarSamples);
  EXPECT_EQ (pos.bar, 2);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // Test fractional samples (should use floor rounding)
  pos =
    map->samples_to_musical_position (quarterNoteSamples - 1 * units::sample);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 4); // Last sixteenth of beat 1
  EXPECT_EQ (pos.tick, 239);    // 240 ticks per sixteenth - 1

  pos =
    map->samples_to_musical_position (quarterNoteSamples + 1 * units::sample);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 2);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);
}
}
