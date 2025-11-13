// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"
#include "utils/units.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{
class TempoMapTest : public ::testing::Test
{
protected:
  static constexpr auto SAMPLE_RATE = units::sample_rate (44100.0);

  void SetUp () override { map = std::make_unique<TempoMap> (SAMPLE_RATE); }

  std::unique_ptr<TempoMap> map;
};

// Test initial state
TEST_F (TempoMapTest, InitialState)
{
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (units::ticks (0)), 120.0);
  EXPECT_EQ (map->time_signature_at_tick (units::ticks (0)).numerator, 4);
  EXPECT_EQ (map->time_signature_at_tick (units::ticks (0)).denominator, 4);
}

// Test tempo event management
TEST_F (TempoMapTest, TempoEventManagement)
{
  // Add constant tempo event
  map->add_tempo_event (
    units::ticks (1920), 140.0, TempoMap::CurveType::Constant);
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (units::ticks (1920)), 140.0);
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (1919)), 120.0); // before the event

  // Add linear tempo event at 3840
  map->add_tempo_event (units::ticks (3840), 160.0, TempoMap::CurveType::Linear);
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (units::ticks (3840)), 160.0);

  // Update existing event at 1920
  map->add_tempo_event (units::ticks (1920), 150.0, TempoMap::CurveType::Linear);
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (units::ticks (1920)), 150.0);
  // Check the linear ramp: at the midpoint between 1920 and 3840, tempo should
  // be 155.0
  EXPECT_NEAR (map->tempo_at_tick (units::ticks (2880)), 155.0, 1e-8);

  // Remove event at 3840
  map->remove_tempo_event (units::ticks (3840));
  // Now the tempo at 3840 should be the same as the previous event (150.0)
  // because the event was removed
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (units::ticks (3840)), 150.0);
  // Also, the segment from 1920 onward should be constant, so at 2880 it should
  // be 150.0 (not ramping)
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (units::ticks (2880)), 150.0);
}

// Test time signature management
TEST_F (TempoMapTest, TimeSignatureManagement)
{
  // Add time signature
  map->add_time_signature_event (units::ticks (1920), 3, 4);
  auto ts = map->time_signature_at_tick (units::ticks (1920));
  EXPECT_EQ (ts.numerator, 3);
  EXPECT_EQ (ts.denominator, 4);
  // Before the event, it should be 4/4
  ts = map->time_signature_at_tick (units::ticks (1919));
  EXPECT_EQ (ts.numerator, 4);
  EXPECT_EQ (ts.denominator, 4);

  // Update existing
  map->add_time_signature_event (units::ticks (1920), 5, 8);
  ts = map->time_signature_at_tick (units::ticks (1920));
  EXPECT_EQ (ts.numerator, 5);
  EXPECT_EQ (ts.denominator, 8);

  // Remove
  map->remove_time_signature_event (units::ticks (1920));
  ts = map->time_signature_at_tick (units::ticks (1920));
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
    map->tick_to_seconds (units::ticks (0)).in (units::seconds), 0.0);
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (units::ticks (960)).in (units::seconds), 0.5);
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (units::ticks (1920)).in (units::seconds), 1.0);

  // Test seconds to tick
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (units::seconds (0.0)).in (units::ticks), 0.0);
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (units::seconds (0.5)).in (units::ticks), 960.0);
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (units::seconds (1.0)).in (units::ticks), 1920.0);

  // Test samples conversion
  EXPECT_DOUBLE_EQ (
    map->tick_to_samples (units::ticks (960)).in (units::samples),
    0.5 * 44100.0);
  EXPECT_DOUBLE_EQ (
    map->samples_to_tick (units::samples (0.5 * 44100.0)).in (units::ticks),
    960.0);
}

// Test linear tempo ramp conversions
TEST_F (TempoMapTest, LinearTempoRamp)
{
  // Create a linear ramp segment from 120 to 180 BPM over 4 beats
  const auto startRamp = units::ticks (0);
  const auto endRamp = units::ticks (4 * 960); // 4 beats
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
    (60.0 * segmentTicks.in (units::ticks)) / (960.0 * (180.0 - 120.0))
    * std::log (currentBpm / 120.0);

  EXPECT_NEAR (
    map->tick_to_seconds (midTick).in (units::seconds), expectedTime, 1e-8);

  // Test reverse conversion
  EXPECT_NEAR (
    map->seconds_to_tick (units::seconds (expectedTime)).in (units::ticks),
    midTick.in (units::ticks), 1e-6);
}

// Test musical position conversions
TEST_F (TempoMapTest, MusicalPositionConversion)
{
  // Test default 4/4 time
  auto pos = map->tick_to_musical_position (units::ticks (0));
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tick_to_musical_position (units::ticks (960)); // Quarter note
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 2);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tick_to_musical_position (units::ticks (240)); // Sixteenth note
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 2);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tick_to_musical_position (units::ticks (241)); // Sixteenth +1 tick
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 2);
  EXPECT_EQ (pos.tick, 1);

  // Test reverse conversion
  EXPECT_EQ (
    map->musical_position_to_tick ({ 1, 1, 1, 0 }).in (units::ticks), 0);
  EXPECT_EQ (
    map->musical_position_to_tick ({ 1, 2, 1, 0 }).in (units::ticks), 960);
  EXPECT_EQ (
    map->musical_position_to_tick ({ 1, 1, 2, 0 }).in (units::ticks), 240);
  EXPECT_EQ (
    map->musical_position_to_tick ({ 1, 1, 2, 1 }).in (units::ticks), 241);
}

// Test time signature changes
TEST_F (TempoMapTest, TimeSignatureChanges)
{
  // Add time signatures
  map->add_time_signature_event (units::ticks (0), 4, 4); // Bar 1: 4/4
  const auto bar5Start = units::ticks (4 * 4 * 960);      // Bar 5 start
  map->add_time_signature_event (bar5Start, 3, 4);        // Bar 5: 3/4
  const auto bar8Start =
    bar5Start + units::ticks (3 * 3 * 960); // Bar 8 start (3 bars of 3/4)
  map->add_time_signature_event (bar8Start, 7, 8); // Bar 8: 7/8

  // Test positions
  auto pos = map->tick_to_musical_position (units::ticks (0));
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tick_to_musical_position (bar5Start - units::ticks (1));
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

  pos = map->tick_to_musical_position (bar5Start + units::ticks (2 * 960));
  EXPECT_EQ (pos.bar, 5);
  EXPECT_EQ (pos.beat, 3);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // beats per bar (numerator) is 3, so we should move to a new bar
  pos = map->tick_to_musical_position (bar5Start + units::ticks (3 * 960));
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
    bar8Start + units::ticks (960) + units::ticks (960 / 4) + units::ticks (1));
  EXPECT_EQ (pos.bar, 8);
  EXPECT_EQ (pos.beat, 3);
  EXPECT_EQ (pos.sixteenth, 2);
  EXPECT_EQ (pos.tick, 1);

  // Test reverse conversions
  EXPECT_EQ (map->musical_position_to_tick ({ 5, 1, 1, 0 }), bar5Start);
  EXPECT_EQ (
    map->musical_position_to_tick ({ 5, 3, 1, 0 }),
    bar5Start + units::ticks (2 * 960));
  EXPECT_EQ (map->musical_position_to_tick ({ 8, 1, 1, 0 }), bar8Start);
  EXPECT_EQ (
    map->musical_position_to_tick ({ 8, 3, 2, 1 }),
    bar8Start + units::ticks (960) + units::ticks (960 / 4) + units::ticks (1));
}

// MultiSegmentLinearRamp test
TEST_F (TempoMapTest, MultiSegmentLinearRamp)
{
  // Setup tempo events
  map->add_tempo_event (units::ticks (960), 120.0, TempoMap::CurveType::Linear);
  map->add_tempo_event (
    units::ticks (1920), 180.0, TempoMap::CurveType::Constant);

  // Test before ramp
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (units::ticks (480)).in (units::seconds), 0.25);

  // Test start of ramp
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (units::ticks (960)).in (units::seconds), 0.5);

  // Test midpoint of ramp (150 BPM)
  const auto   midTime = map->tick_to_seconds (units::ticks (1440));
  const double expectedMidTime =
    0.5 + (60.0 * 960) / (960.0 * 60.0) * std::log (150.0 / 120.0);
  EXPECT_NEAR (midTime.in (units::seconds), expectedMidTime, 1e-8);

  // Test end of ramp
  const auto   endTime = map->tick_to_seconds (units::ticks (1920));
  const double expectedEndTime =
    0.5 + (60.0 * 960) / (960.0 * 60.0) * std::log (180.0 / 120.0);
  EXPECT_NEAR (endTime.in (units::seconds), expectedEndTime, 1e-8);

  // Test after ramp (480 ticks at 180 BPM)
  const double afterRampTime =
    endTime.in (units::seconds) + (480.0 / 960.0) * (60.0 / 180.0);
  EXPECT_NEAR (
    map->tick_to_seconds (units::ticks (2400)).in (units::seconds),
    afterRampTime, 1e-8);
}

// Test tempo lookups
TEST_F (TempoMapTest, TempoLookup)
{
  // Setup linear ramp from 120 to 180 BPM over 4 beats (3840 ticks)
  map->add_tempo_event (units::ticks (0), 120.0, TempoMap::CurveType::Linear);
  map->add_tempo_event (
    units::ticks (3840), 180.0, TempoMap::CurveType::Constant);

  // Test start of ramp
  auto tempo = map->tempo_at_tick (units::ticks (0));
  EXPECT_DOUBLE_EQ (tempo, 120.0);

  // Test 1/4 through ramp
  tempo = map->tempo_at_tick (units::ticks (960));
  EXPECT_NEAR (tempo, 135.0, 1e-8);

  // Test midpoint of ramp
  tempo = map->tempo_at_tick (units::ticks (1920));
  EXPECT_NEAR (tempo, 150.0, 1e-8);

  // Test 3/4 through ramp
  tempo = map->tempo_at_tick (units::ticks (2880));
  EXPECT_NEAR (tempo, 165.0, 1e-8);

  // Test end of ramp
  tempo = map->tempo_at_tick (units::ticks (3840));
  EXPECT_DOUBLE_EQ (tempo, 180.0);

  // Test after ramp
  tempo = map->tempo_at_tick (units::ticks (4800));
  EXPECT_DOUBLE_EQ (tempo, 180.0);
}

// Test linear ramp as last event
TEST_F (TempoMapTest, LinearRampLastEvent)
{
  // Setup:
  // [0, 960): Constant 120 BPM
  // [960, ∞): Linear ramp 120 → ? BPM (should be constant 120 since no end point)
  map->add_tempo_event (units::ticks (960), 120.0, TempoMap::CurveType::Linear);

  // Should be constant after 960 because no endpoint for ramp
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (units::ticks (960)).in (units::seconds), 0.5);
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (units::ticks (1440)).in (units::seconds), 0.5 + 0.25);
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (units::ticks (1920)).in (units::seconds), 0.5 + 0.5);
}

// Test edge cases
TEST_F (TempoMapTest, EdgeCases)
{
  // Zero and negative time
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (units::seconds (0.0)).in (units::ticks), 0.0);
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (units::seconds (-1.0)).in (units::ticks), 0.0);

  // Empty tempo map - default value active
  TempoMap emptyMap (units::sample_rate (960));
  emptyMap.remove_tempo_event (units::ticks (0));
  EXPECT_DOUBLE_EQ (
    emptyMap.tick_to_seconds (units::ticks (960)).in (units::seconds), 0.5);

  // Near-constant ramp
  map->add_tempo_event (
    units::ticks (960), 120.001, TempoMap::CurveType::Linear);
  map->add_tempo_event (
    units::ticks (1920), 120.002, TempoMap::CurveType::Constant);
  EXPECT_NEAR (
    map->tick_to_seconds (units::ticks (1440)).in (units::seconds), 0.5 + 0.25,
    1e-5);

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
    map->musical_position_to_tick ({ 100, 1, 1, 0 }).in (units::ticks), 0);
}

// Test fractional ticks
TEST_F (TempoMapTest, FractionalTicks)
{
  // Test constant tempo
  const auto expectedTime = units::seconds (480.5 * (0.5 / 960.0));
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (units::ticks (480.5)).in (units::seconds),
    expectedTime.in (units::seconds));

  // Test reverse conversion
  EXPECT_NEAR (
    map->seconds_to_tick (expectedTime).in (units::ticks), 480.5, 1e-6);
}

// Test sample rate changes
TEST_F (TempoMapTest, SampleRateChanges)
{
  const double newRate = 48000.0;
  map->set_sample_rate (units::sample_rate (newRate));

  EXPECT_DOUBLE_EQ (
    map->tick_to_samples (units::ticks (960)).in (units::samples),
    0.5 * newRate);
  EXPECT_DOUBLE_EQ (
    map->samples_to_tick (units::samples (0.5 * newRate)).in (units::ticks),
    960.0);
}

// Test complex time signature with different beat units
TEST_F (TempoMapTest, ComplexTimeSignatures)
{
  map->add_time_signature_event (units::ticks (0), 6, 8); // 6/8 time

  // end at 6 beats
  const auto bar1Ticks = units::ticks (6 * (TempoMap::get_ppq () / 2));
  const auto bar1End = bar1Ticks - units::ticks (1);

  auto pos = map->tick_to_musical_position (units::ticks (0));
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);

  pos = map->tick_to_musical_position (bar1End);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 6);

  // Test 7/16 time
  map->add_time_signature_event (bar1Ticks, 7, 16);

  // end at 7 beats
  const auto bar2Ticks = units::ticks (7 * (TempoMap::get_ppq () / 4));
  const auto bar2Start = bar1Ticks;
  const auto bar2End = bar2Start + bar2Ticks - units::ticks (1);

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
  auto ts = map->time_signature_at_tick (units::ticks (0));
  EXPECT_EQ (ts.numerator, 4);
  EXPECT_EQ (ts.denominator, 4);

  // Add time signatures
  map->add_time_signature_event (
    units::ticks (1920), 3, 4); // Bar 3 (assuming 4/4)
  map->add_time_signature_event (units::ticks (3840), 5, 8); // Bar 5 (3/4)

  // Test before first change
  ts = map->time_signature_at_tick (units::ticks (0));
  EXPECT_EQ (ts.numerator, 4);
  EXPECT_EQ (ts.denominator, 4);

  // Test at first change point
  ts = map->time_signature_at_tick (units::ticks (1920));
  EXPECT_EQ (ts.numerator, 3);
  EXPECT_EQ (ts.denominator, 4);

  // Test between changes
  ts = map->time_signature_at_tick (units::ticks (2000));
  EXPECT_EQ (ts.numerator, 3);
  EXPECT_EQ (ts.denominator, 4);

  // Test at second change point
  ts = map->time_signature_at_tick (units::ticks (3840));
  EXPECT_EQ (ts.numerator, 5);
  EXPECT_EQ (ts.denominator, 8);

  // Test after last change
  ts = map->time_signature_at_tick (units::ticks (10000));
  EXPECT_EQ (ts.numerator, 5);
  EXPECT_EQ (ts.denominator, 8);

  // Test with empty map
  TempoMap emptyMap (SAMPLE_RATE);
  emptyMap.remove_time_signature_event (units::ticks (0));
  ts = emptyMap.time_signature_at_tick (units::ticks (0));
  EXPECT_EQ (ts.numerator, 4); // Default
  EXPECT_EQ (ts.denominator, 4);
}

// Test tempo and time signature interaction
TEST_F (TempoMapTest, TempoAndTimeSignatureInteraction)
{
  // Add time signature change at bar 5
  const auto bar5Start = units::ticks (4 * 4 * 960); // Bar 5 start
  map->add_time_signature_event (bar5Start, 3, 4);

  // Add tempo change at bar 3
  const auto bar3Start = units::ticks (2 * 4 * 960); // Bar 3 start
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
    map->tick_to_seconds (bar5Start).in (units::seconds), expectedTime, 1e-5);
}

// Test serialization/deserialization
TEST_F (TempoMapTest, Serialization)
{
  // Add tempo and time signature events
  map->add_time_signature_event (units::ticks (1920), 3, 4);
  map->add_time_signature_event (units::ticks (3840), 5, 8);
  map->add_tempo_event (
    units::ticks (1920), 140.0, TempoMap::CurveType::Constant);
  map->add_tempo_event (units::ticks (3840), 160.0, TempoMap::CurveType::Linear);

  // Serialize to JSON
  nlohmann::json j;
  j = *map;

  // Deserialize to new object
  TempoMap deserialized_map{ SAMPLE_RATE };
  j.get_to (deserialized_map);

  // Verify by checking at various ticks
  std::vector<units::tick_t> test_ticks = {
    units::ticks (0),    units::ticks (960),  units::ticks (1920),
    units::ticks (2880), units::ticks (3840), units::ticks (4800)
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
    map->tick_to_seconds (units::ticks (2880)).in (units::seconds),
    deserialized_map.tick_to_seconds (units::ticks (2880)).in (units::seconds));
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (units::seconds (1.5)).in (units::ticks),
    deserialized_map.seconds_to_tick (units::seconds (1.5)).in (units::ticks));
}

// Test empty serialization
TEST_F (TempoMapTest, EmptySerialization)
{
  // Remove default events
  map->remove_tempo_event (units::ticks (0));
  map->remove_time_signature_event (units::ticks (0));

  // Serialize and deserialize
  nlohmann::json j;
  j = *map;
  TempoMap deserialized_map{ SAMPLE_RATE };
  j.get_to (deserialized_map);

  // Verify empty by checking default tempo and time signature
  EXPECT_DOUBLE_EQ (deserialized_map.tempo_at_tick (units::ticks (0)), 120.0);
  auto ts = deserialized_map.time_signature_at_tick (units::ticks (0));
  EXPECT_EQ (ts.numerator, 4);
  EXPECT_EQ (ts.denominator, 4);
}

// Test samples to musical position conversion
TEST_F (TempoMapTest, SamplesToMusicalPosition)
{
  // Test basic conversion with default 120 BPM, 4/4 time
  auto pos = map->samples_to_musical_position (units::samples (0));
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // Test quarter note at 120 BPM (0.5 seconds = 22050 samples at 44.1kHz)
  const auto quarterNoteSamples = (units::seconds (0.5) * SAMPLE_RATE);
  pos = map->samples_to_musical_position (quarterNoteSamples.as<int64_t> (
    units::samples, ignore (au::TRUNCATION_RISK)));
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 2);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // Test half note (1 second = 44100 samples)
  const auto halfNoteSamples = (units::seconds (1.0) * SAMPLE_RATE);
  pos = map->samples_to_musical_position (
    halfNoteSamples.as<int64_t> (units::samples, ignore (au::TRUNCATION_RISK)));
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 3);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // Test full bar (2 seconds = 88200 samples)
  const auto fullBarSamples = (units::seconds (2.0) * SAMPLE_RATE);
  pos = map->samples_to_musical_position (
    fullBarSamples.as<int64_t> (units::samples, ignore (au::TRUNCATION_RISK)));
  EXPECT_EQ (pos.bar, 2);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  // Test fractional samples (should use floor rounding)
  pos = map->samples_to_musical_position (
    quarterNoteSamples.as<int64_t> (units::samples, ignore (au::TRUNCATION_RISK))
    - units::samples (1));
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 4); // Last sixteenth of beat 1
  EXPECT_EQ (pos.tick, 239);    // 240 ticks per sixteenth - 1

  pos = map->samples_to_musical_position (
    quarterNoteSamples.as<int64_t> (units::samples, ignore (au::TRUNCATION_RISK))
    + units::samples (1));
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 2);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);
}

// Test TimeSignatureEvent utility methods
TEST_F (TempoMapTest, TimeSignatureEventUtilityMethods)
{
  // Test 4/4 time signature
  TempoMap::TimeSignatureEvent ts4_4{ units::ticks (0), 4, 4 };
  EXPECT_EQ (ts4_4.quarters_per_bar (), 4);
  EXPECT_EQ (ts4_4.ticks_per_bar ().in (units::ticks), 4 * 960);
  EXPECT_EQ (ts4_4.ticks_per_beat ().in (units::ticks), 960);

  // Test 3/4 time signature
  TempoMap::TimeSignatureEvent ts3_4{ units::ticks (0), 3, 4 };
  EXPECT_EQ (ts3_4.quarters_per_bar (), 3);
  EXPECT_EQ (ts3_4.ticks_per_bar ().in (units::ticks), 3 * 960);
  EXPECT_EQ (ts3_4.ticks_per_beat ().in (units::ticks), 960);

  // Test 6/8 time signature
  TempoMap::TimeSignatureEvent ts6_8{ units::ticks (0), 6, 8 };
  EXPECT_EQ (ts6_8.quarters_per_bar (), 3); // (6 * 4) / 8 = 3
  EXPECT_EQ (ts6_8.ticks_per_bar ().in (units::ticks), 3 * 960);
  EXPECT_EQ (ts6_8.ticks_per_beat ().in (units::ticks), 480); // 2880 / 6

  // Test 5/8 time signature
  TempoMap::TimeSignatureEvent ts5_8{ units::ticks (0), 5, 8 };
  EXPECT_EQ (
    ts5_8.quarters_per_bar (), 2); // (5 * 4) / 8 = 2.5, but integer division
  EXPECT_EQ (ts5_8.ticks_per_bar ().in (units::ticks), 2 * 960);
  EXPECT_EQ (ts5_8.ticks_per_beat ().in (units::ticks), 384); // 1920 / 5

  // Test 7/16 time signature
  TempoMap::TimeSignatureEvent ts7_16{ units::ticks (0), 7, 16 };
  EXPECT_EQ (
    ts7_16.quarters_per_bar (), 1); // (7 * 4) / 16 = 1.75, but integer division
  EXPECT_EQ (ts7_16.ticks_per_bar ().in (units::ticks), 1 * 960);
  EXPECT_EQ (
    ts7_16.ticks_per_beat ().in (units::ticks), 137); // 960 / 7 (integer
                                                      // division)

  // Test 2/2 time signature (cut time)
  TempoMap::TimeSignatureEvent ts2_2{ units::ticks (0), 2, 2 };
  EXPECT_EQ (ts2_2.quarters_per_bar (), 4); // (2 * 4) / 2 = 4
  EXPECT_EQ (ts2_2.ticks_per_bar ().in (units::ticks), 4 * 960);
  EXPECT_EQ (ts2_2.ticks_per_beat ().in (units::ticks), 1920); // 3840 / 2

  // Test 12/8 time signature
  TempoMap::TimeSignatureEvent ts12_8{ units::ticks (0), 12, 8 };
  EXPECT_EQ (ts12_8.quarters_per_bar (), 6); // (12 * 4) / 8 = 6
  EXPECT_EQ (ts12_8.ticks_per_bar ().in (units::ticks), 6 * 960);
  EXPECT_EQ (ts12_8.ticks_per_beat ().in (units::ticks), 480); // 5760 / 12
}

// Test TimeSignatureEvent utility methods with time_signature_at_tick
TEST_F (TempoMapTest, TimeSignatureEventUtilityMethodsIntegration)
{
  // Add various time signatures to the tempo map
  map->add_time_signature_event (units::ticks (0), 4, 4);        // Bar 1: 4/4
  const auto bar3Start = units::ticks (2 * 4 * 960);             // Bar 3 start
  map->add_time_signature_event (bar3Start, 3, 4);               // Bar 3: 3/4
  const auto bar5Start = bar3Start + units::ticks (2 * 3 * 960); // Bar 5 start
  map->add_time_signature_event (bar5Start, 6, 8);               // Bar 5: 6/8

  // Test 4/4 section
  auto ts = map->time_signature_at_tick (units::ticks (0));
  EXPECT_EQ (ts.quarters_per_bar (), 4);
  EXPECT_EQ (ts.ticks_per_bar ().in (units::ticks), 3840);
  EXPECT_EQ (ts.ticks_per_beat ().in (units::ticks), 960);

  // Test 3/4 section
  ts = map->time_signature_at_tick (bar3Start);
  EXPECT_EQ (ts.quarters_per_bar (), 3);
  EXPECT_EQ (ts.ticks_per_bar ().in (units::ticks), 2880);
  EXPECT_EQ (ts.ticks_per_beat ().in (units::ticks), 960);

  // Test 6/8 section
  ts = map->time_signature_at_tick (bar5Start);
  EXPECT_EQ (ts.quarters_per_bar (), 3); // (6 * 4) / 8 = 3
  EXPECT_EQ (ts.ticks_per_bar ().in (units::ticks), 2880);
  EXPECT_EQ (ts.ticks_per_beat ().in (units::ticks), 480); // 2880 / 6

  // Verify consistency with musical position calculations
  // In 6/8 time, 6 beats should equal one bar
  const auto sixBeatsIn6_8 =
    units::ticks (6 * ts.ticks_per_beat ().in (units::ticks));
  EXPECT_EQ (
    sixBeatsIn6_8.in (units::ticks), ts.ticks_per_bar ().in (units::ticks));

  // In 3/4 time, 3 beats should equal one bar
  ts = map->time_signature_at_tick (bar3Start);
  const auto threeBeatsIn3_4 =
    units::ticks (3 * ts.ticks_per_beat ().in (units::ticks));
  EXPECT_EQ (
    threeBeatsIn3_4.in (units::ticks), ts.ticks_per_bar ().in (units::ticks));
}

// Test edge cases for TimeSignatureEvent utility methods
TEST_F (TempoMapTest, TimeSignatureEventUtilityMethodsEdgeCases)
{
  // Test with numerator = 1
  TempoMap::TimeSignatureEvent ts1_4{ units::ticks (0), 1, 4 };
  EXPECT_EQ (ts1_4.quarters_per_bar (), 1);
  EXPECT_EQ (ts1_4.ticks_per_bar ().in (units::ticks), 960);
  EXPECT_EQ (ts1_4.ticks_per_beat ().in (units::ticks), 960);

  // Test with large numbers
  TempoMap::TimeSignatureEvent tsLarge{ units::ticks (0), 32, 32 };
  EXPECT_EQ (tsLarge.quarters_per_bar (), 4); // (32 * 4) / 32 = 4
  EXPECT_EQ (tsLarge.ticks_per_bar ().in (units::ticks), 3840);
  EXPECT_EQ (tsLarge.ticks_per_beat ().in (units::ticks), 120); // 3840 / 32

  // Test with denominator larger than numerator
  TempoMap::TimeSignatureEvent ts3_16{ units::ticks (0), 3, 16 };
  EXPECT_EQ (
    ts3_16.quarters_per_bar (), 0); // (3 * 4) / 16 = 0 (integer division)
  EXPECT_EQ (ts3_16.ticks_per_bar ().in (units::ticks), 0);
  EXPECT_EQ (ts3_16.ticks_per_beat ().in (units::ticks), 0); // 0 / 3 = 0
}

// Test clearing tempo events
TEST_F (TempoMapTest, ClearTempoEvents)
{
  // Add multiple tempo events
  map->add_tempo_event (
    units::ticks (960), 140.0, TempoMap::CurveType::Constant);
  map->add_tempo_event (units::ticks (1920), 160.0, TempoMap::CurveType::Linear);
  map->add_tempo_event (
    units::ticks (3840), 180.0, TempoMap::CurveType::Constant);

  // Verify events exist
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (units::ticks (960)), 140.0);
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (units::ticks (1920)), 160.0);
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (units::ticks (3840)), 180.0);

  // Clear all tempo events
  map->clear_tempo_events ();

  // Verify all tempo events are cleared (should return default tempo)
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (units::ticks (0)), 120.0);    // Default
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (units::ticks (960)), 120.0);  // Default
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (units::ticks (1920)), 120.0); // Default
  EXPECT_DOUBLE_EQ (map->tempo_at_tick (units::ticks (3840)), 120.0); // Default

  // Test that conversions work correctly after clearing
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (units::ticks (960)).in (units::seconds),
    0.5); // 120 BPM
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (units::ticks (1920)).in (units::seconds),
    1.0); // 120 BPM
}

// Test clearing time signature events
TEST_F (TempoMapTest, ClearTimeSignatureEvents)
{
  // Add multiple time signature events
  map->add_time_signature_event (units::ticks (1920), 3, 4);
  map->add_time_signature_event (units::ticks (3840), 5, 8);
  map->add_time_signature_event (units::ticks (5760), 7, 16);

  // Verify events exist
  auto ts = map->time_signature_at_tick (units::ticks (1920));
  EXPECT_EQ (ts.numerator, 3);
  EXPECT_EQ (ts.denominator, 4);

  ts = map->time_signature_at_tick (units::ticks (3840));
  EXPECT_EQ (ts.numerator, 5);
  EXPECT_EQ (ts.denominator, 8);

  ts = map->time_signature_at_tick (units::ticks (5760));
  EXPECT_EQ (ts.numerator, 7);
  EXPECT_EQ (ts.denominator, 16);

  // Clear all time signature events
  map->clear_time_signature_events ();

  // Verify all time signature events are cleared (should return default 4/4)
  ts = map->time_signature_at_tick (units::ticks (0));
  EXPECT_EQ (ts.numerator, 4);   // Default
  EXPECT_EQ (ts.denominator, 4); // Default

  ts = map->time_signature_at_tick (units::ticks (1920));
  EXPECT_EQ (ts.numerator, 4);   // Default
  EXPECT_EQ (ts.denominator, 4); // Default

  ts = map->time_signature_at_tick (units::ticks (3840));
  EXPECT_EQ (ts.numerator, 4);   // Default
  EXPECT_EQ (ts.denominator, 4); // Default

  ts = map->time_signature_at_tick (units::ticks (5760));
  EXPECT_EQ (ts.numerator, 4);   // Default
  EXPECT_EQ (ts.denominator, 4); // Default

  // Test that musical position calculations work correctly after clearing
  auto pos = map->tick_to_musical_position (units::ticks (0));
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);

  pos = map->tick_to_musical_position (
    units::ticks (3840)); // Should be bar 2 in 4/4
  EXPECT_EQ (pos.bar, 2);
  EXPECT_EQ (pos.beat, 1);
  EXPECT_EQ (pos.sixteenth, 1);
  EXPECT_EQ (pos.tick, 0);
}
}
