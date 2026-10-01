// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"
#include "utils/units.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

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
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (0)).in (units::bpm), 120.0);
  EXPECT_EQ (
    map->time_signature_at_tick (units::ticks (0)).time_signature.numerator, 4);
  EXPECT_EQ (
    map->time_signature_at_tick (units::ticks (0)).time_signature.denominator,
    4);
}

// Test tempo event management
TEST_F (TempoMapTest, TempoEventManagement)
{
  // Add constant tempo event
  map->add_tempo_event (
    units::ticks (1920), units::bpm (140.0), TempoMap::CurveType::Constant);
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (1920)).in (units::bpm), 140.0);
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (1919)).in (units::bpm),
    120.0); // before the event

  // Add linear tempo event at 3840
  map->add_tempo_event (
    units::ticks (3840), units::bpm (160.0), TempoMap::CurveType::Linear);
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (3840)).in (units::bpm), 160.0);

  // Update existing event at 1920
  map->add_tempo_event (
    units::ticks (1920), units::bpm (150.0), TempoMap::CurveType::Linear);
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (1920)).in (units::bpm), 150.0);
  // Check the linear ramp: at the midpoint between 1920 and 3840, tempo should
  // be 155.0
  EXPECT_NEAR (
    map->tempo_at_tick (units::ticks (2880)).in (units::bpm), 155.0, 1e-8);

  // Remove event at 3840
  map->remove_tempo_event (units::ticks (3840));
  // Now the tempo at 3840 should be the same as the previous event (150.0)
  // because the event was removed
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (3840)).in (units::bpm), 150.0);
  // Also, the segment from 1920 onward should be constant, so at 2880 it should
  // be 150.0 (not ramping)
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (2880)).in (units::bpm), 150.0);
}

// Test time signature management
TEST_F (TempoMapTest, TimeSignatureManagement)
{
  // Add time signature
  map->add_time_signature_event (units::ticks (1920), TimeSignature{ 3, 4 });
  auto ts = map->time_signature_at_tick (units::ticks (1920));
  EXPECT_EQ (ts.time_signature.numerator, 3);
  EXPECT_EQ (ts.time_signature.denominator, 4);
  // Before the event, it should be 4/4
  ts = map->time_signature_at_tick (units::ticks (1919));
  EXPECT_EQ (ts.time_signature.numerator, 4);
  EXPECT_EQ (ts.time_signature.denominator, 4);

  // Update existing
  map->add_time_signature_event (units::ticks (1920), TimeSignature{ 5, 8 });
  ts = map->time_signature_at_tick (units::ticks (1920));
  EXPECT_EQ (ts.time_signature.numerator, 5);
  EXPECT_EQ (ts.time_signature.denominator, 8);

  // Remove
  map->remove_time_signature_event (units::ticks (1920));
  ts = map->time_signature_at_tick (units::ticks (1920));
  EXPECT_EQ (ts.time_signature.numerator, 4);
  EXPECT_EQ (ts.time_signature.denominator, 4);
}

// Test constant tempo conversions
TEST_F (TempoMapTest, ConstantTempoConversions)
{
  // 120 BPM = 0.5 seconds per beat
  // 1 beat = 960 ticks -> 1 tick = 0.5/960 seconds

  // Test tick to seconds
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (TimelineTick{ units::ticks (0) }).in (units::seconds),
    0.0);
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (TimelineTick{ units::ticks (960) }).in (units::seconds),
    0.5);
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (TimelineTick{ units::ticks (1920) })
      .in (units::seconds),
    1.0);

  // Test seconds to tick
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (units::seconds (0.0)).asDouble (), 0.0);
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (units::seconds (0.5)).asDouble (), 960.0);
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (units::seconds (1.0)).asDouble (), 1920.0);

  // Test samples conversion
  EXPECT_DOUBLE_EQ (
    map->tick_to_samples (TimelineTick{ units::ticks (960) }).in (units::samples),
    0.5 * 44100.0);
  EXPECT_DOUBLE_EQ (
    map->samples_to_tick (units::samples (0.5 * 44100.0)).asDouble (), 960.0);
}

// Test linear tempo ramp conversions
TEST_F (TempoMapTest, LinearTempoRamp)
{
  // Create a linear ramp segment from 120 to 180 BPM over 4 beats
  const auto startRamp = units::ticks (0);
  const auto endRamp = units::ticks (4 * 960); // 4 beats
  map->add_tempo_event (
    startRamp, units::bpm (120.0), TempoMap::CurveType::Linear);
  map->add_tempo_event (
    endRamp, units::bpm (180.0), TempoMap::CurveType::Constant);

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
    map->tick_to_seconds (TimelineTick{ midTick }).in (units::seconds),
    expectedTime, 1e-8);

  // Test reverse conversion
  EXPECT_NEAR (
    map->seconds_to_tick (units::seconds (expectedTime)).asDouble (),
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
  EXPECT_EQ (map->musical_position_to_tick ({ 1, 1, 1, 0 }).asDouble (), 0);
  EXPECT_EQ (map->musical_position_to_tick ({ 1, 2, 1, 0 }).asDouble (), 960);
  EXPECT_EQ (map->musical_position_to_tick ({ 1, 1, 2, 0 }).asDouble (), 240);
  EXPECT_EQ (map->musical_position_to_tick ({ 1, 1, 2, 1 }).asDouble (), 241);
}

// Test time signature changes
TEST_F (TempoMapTest, TimeSignatureChanges)
{
  // Add time signatures
  map->add_time_signature_event (
    units::ticks (0), TimeSignature{ 4, 4 });        // Bar 1: 4/4
  const auto bar5Start = units::ticks (4 * 4 * 960); // Bar 5 start
  map->add_time_signature_event (bar5Start, TimeSignature{ 3, 4 }); // Bar 5: 3/4
  const auto bar8Start =
    bar5Start + units::ticks (3 * 3 * 960); // Bar 8 start (3 bars of 3/4)
  map->add_time_signature_event (bar8Start, TimeSignature{ 7, 8 }); // Bar 8: 7/8

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
  EXPECT_EQ (
    map->musical_position_to_tick ({ 5, 1, 1, 0 }).asQuantity (), bar5Start);
  EXPECT_EQ (
    map->musical_position_to_tick ({ 5, 3, 1, 0 }).asQuantity (),
    bar5Start + units::ticks (2 * 960));
  EXPECT_EQ (
    map->musical_position_to_tick ({ 8, 1, 1, 0 }).asQuantity (), bar8Start);
  EXPECT_EQ (
    map->musical_position_to_tick ({ 8, 3, 2, 1 }).asQuantity (),
    bar8Start + units::ticks (960) + units::ticks (960 / 4) + units::ticks (1));
}

// MultiSegmentLinearRamp test
TEST_F (TempoMapTest, MultiSegmentLinearRamp)
{
  // Setup tempo events
  map->add_tempo_event (
    units::ticks (960), units::bpm (120.0), TempoMap::CurveType::Linear);
  map->add_tempo_event (
    units::ticks (1920), units::bpm (180.0), TempoMap::CurveType::Constant);

  // Test before ramp
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (TimelineTick{ units::ticks (480) }).in (units::seconds),
    0.25);

  // Test start of ramp
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (TimelineTick{ units::ticks (960) }).in (units::seconds),
    0.5);

  // Test midpoint of ramp (150 BPM)
  const auto midTime =
    map->tick_to_seconds (TimelineTick{ units::ticks (1440) });
  const double expectedMidTime =
    0.5 + (60.0 * 960) / (960.0 * 60.0) * std::log (150.0 / 120.0);
  EXPECT_NEAR (midTime.in (units::seconds), expectedMidTime, 1e-8);

  // Test end of ramp
  const auto endTime =
    map->tick_to_seconds (TimelineTick{ units::ticks (1920) });
  const double expectedEndTime =
    0.5 + (60.0 * 960) / (960.0 * 60.0) * std::log (180.0 / 120.0);
  EXPECT_NEAR (endTime.in (units::seconds), expectedEndTime, 1e-8);

  // Test after ramp (480 ticks at 180 BPM)
  const double afterRampTime =
    endTime.in (units::seconds) + (480.0 / 960.0) * (60.0 / 180.0);
  EXPECT_NEAR (
    map->tick_to_seconds (TimelineTick{ units::ticks (2400) })
      .in (units::seconds),
    afterRampTime, 1e-8);
}

// Test tempo lookups
TEST_F (TempoMapTest, TempoLookup)
{
  // Setup linear ramp from 120 to 180 BPM over 4 beats (3840 ticks)
  map->add_tempo_event (
    units::ticks (0), units::bpm (120.0), TempoMap::CurveType::Linear);
  map->add_tempo_event (
    units::ticks (3840), units::bpm (180.0), TempoMap::CurveType::Constant);

  // Test start of ramp
  auto tempo = map->tempo_at_tick (units::ticks (0));
  EXPECT_DOUBLE_EQ (tempo.in (units::bpm), 120.0);

  // Test 1/4 through ramp
  tempo = map->tempo_at_tick (units::ticks (960));
  EXPECT_NEAR (tempo.in (units::bpm), 135.0, 1e-8);

  // Test midpoint of ramp
  tempo = map->tempo_at_tick (units::ticks (1920));
  EXPECT_NEAR (tempo.in (units::bpm), 150.0, 1e-8);

  // Test 3/4 through ramp
  tempo = map->tempo_at_tick (units::ticks (2880));
  EXPECT_NEAR (tempo.in (units::bpm), 165.0, 1e-8);

  // Test end of ramp
  tempo = map->tempo_at_tick (units::ticks (3840));
  EXPECT_DOUBLE_EQ (tempo.in (units::bpm), 180.0);

  // Test after ramp
  tempo = map->tempo_at_tick (units::ticks (4800));
  EXPECT_DOUBLE_EQ (tempo.in (units::bpm), 180.0);
}

// Test linear ramp as last event
TEST_F (TempoMapTest, LinearRampLastEvent)
{
  // Setup:
  // [0, 960): Constant 120 BPM
  // [960, ∞): Linear ramp 120 → ? BPM (should be constant 120 since no end point)
  map->add_tempo_event (
    units::ticks (960), units::bpm (120.0), TempoMap::CurveType::Linear);

  // Should be constant after 960 because no endpoint for ramp
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (TimelineTick{ units::ticks (960) }).in (units::seconds),
    0.5);
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (TimelineTick{ units::ticks (1440) })
      .in (units::seconds),
    0.5 + 0.25);
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (TimelineTick{ units::ticks (1920) })
      .in (units::seconds),
    0.5 + 0.5);
}

// Test edge cases
TEST_F (TempoMapTest, EdgeCases)
{
  // Zero and negative time
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (units::seconds (0.0)).asDouble (), 0.0);
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (units::seconds (-1.0)).asDouble (), 0.0);

  // Empty tempo map - default value active
  TempoMap emptyMap (units::sample_rate (960));
  emptyMap.remove_tempo_event (units::ticks (0));
  EXPECT_DOUBLE_EQ (
    emptyMap.tick_to_seconds (TimelineTick{ units::ticks (960) })
      .in (units::seconds),
    0.5);

  // Near-constant ramp
  map->add_tempo_event (
    units::ticks (960), units::bpm (120.001), TempoMap::CurveType::Linear);
  map->add_tempo_event (
    units::ticks (1920), units::bpm (120.002), TempoMap::CurveType::Constant);
  EXPECT_NEAR (
    map->tick_to_seconds (TimelineTick{ units::ticks (1440) })
      .in (units::seconds),
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
  EXPECT_GT (map->musical_position_to_tick ({ 100, 1, 1, 0 }).asDouble (), 0);
}

// Test fractional ticks
TEST_F (TempoMapTest, FractionalTicks)
{
  // Test constant tempo
  const auto expectedTime = units::seconds (480.5 * (0.5 / 960.0));
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (TimelineTick{ units::ticks (480.5) })
      .in (units::seconds),
    expectedTime.in (units::seconds));

  // Test reverse conversion
  EXPECT_NEAR (map->seconds_to_tick (expectedTime).asDouble (), 480.5, 1e-6);
}

// Test sample rate changes
TEST_F (TempoMapTest, SampleRateChanges)
{
  const double newRate = 48000.0;
  map->set_sample_rate (units::sample_rate (newRate));

  EXPECT_DOUBLE_EQ (
    map->tick_to_samples (TimelineTick{ units::ticks (960) }).in (units::samples),
    0.5 * newRate);
  EXPECT_DOUBLE_EQ (
    map->samples_to_tick (units::samples (0.5 * newRate)).asDouble (), 960.0);
}

// Test complex time signature with different beat units
TEST_F (TempoMapTest, ComplexTimeSignatures)
{
  map->add_time_signature_event (
    units::ticks (0), TimeSignature{ 6, 8 }); // 6/8 time

  // end at 6 beats
  const auto bar1Ticks = 6 * (TempoMap::get_ppq () / 2);
  const auto bar1End = bar1Ticks - units::ticks (1);

  auto pos = map->tick_to_musical_position (units::ticks (0));
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 1);

  pos = map->tick_to_musical_position (bar1End);
  EXPECT_EQ (pos.bar, 1);
  EXPECT_EQ (pos.beat, 6);

  // Test 7/16 time
  map->add_time_signature_event (bar1Ticks, TimeSignature{ 7, 16 });

  // end at 7 beats
  const auto bar2Ticks = 7 * (TempoMap::get_ppq () / 4);
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
  EXPECT_EQ (ts.time_signature.numerator, 4);
  EXPECT_EQ (ts.time_signature.denominator, 4);

  // Add time signatures
  map->add_time_signature_event (
    units::ticks (1920), TimeSignature{ 3, 4 }); // Bar 3 (assuming 4/4)
  map->add_time_signature_event (
    units::ticks (3840), TimeSignature{ 5, 8 }); // Bar 5 (3/4)

  // Test before first change
  ts = map->time_signature_at_tick (units::ticks (0));
  EXPECT_EQ (ts.time_signature.numerator, 4);
  EXPECT_EQ (ts.time_signature.denominator, 4);

  // Test at first change point
  ts = map->time_signature_at_tick (units::ticks (1920));
  EXPECT_EQ (ts.time_signature.numerator, 3);
  EXPECT_EQ (ts.time_signature.denominator, 4);

  // Test between changes
  ts = map->time_signature_at_tick (units::ticks (2000));
  EXPECT_EQ (ts.time_signature.numerator, 3);
  EXPECT_EQ (ts.time_signature.denominator, 4);

  // Test at second change point
  ts = map->time_signature_at_tick (units::ticks (3840));
  EXPECT_EQ (ts.time_signature.numerator, 5);
  EXPECT_EQ (ts.time_signature.denominator, 8);

  // Test after last change
  ts = map->time_signature_at_tick (units::ticks (10000));
  EXPECT_EQ (ts.time_signature.numerator, 5);
  EXPECT_EQ (ts.time_signature.denominator, 8);

  // Test with empty map
  TempoMap emptyMap (SAMPLE_RATE);
  emptyMap.remove_time_signature_event (units::ticks (0));
  ts = emptyMap.time_signature_at_tick (units::ticks (0));
  EXPECT_EQ (ts.time_signature.numerator, 4); // Default
  EXPECT_EQ (ts.time_signature.denominator, 4);
}

// Test tempo and time signature interaction
TEST_F (TempoMapTest, TempoAndTimeSignatureInteraction)
{
  // Add time signature change at bar 5
  const auto bar5Start = units::ticks (4 * 4 * 960); // Bar 5 start
  map->add_time_signature_event (bar5Start, TimeSignature{ 3, 4 });

  // Add tempo change at bar 3
  const auto bar3Start = units::ticks (2 * 4 * 960); // Bar 3 start
  map->add_tempo_event (
    bar3Start, units::bpm (140.0), TempoMap::CurveType::Constant);

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
    map->tick_to_seconds (TimelineTick{ bar5Start }).in (units::seconds),
    expectedTime, 1e-5);
}

// Test serialization/deserialization
TEST_F (TempoMapTest, Serialization)
{
  // Add tempo and time signature events
  map->add_time_signature_event (units::ticks (1920), TimeSignature{ 3, 4 });
  map->add_time_signature_event (units::ticks (3840), TimeSignature{ 5, 8 });
  map->add_tempo_event (
    units::ticks (1920), units::bpm (140.0), TempoMap::CurveType::Constant);
  map->add_tempo_event (
    units::ticks (3840), units::bpm (160.0), TempoMap::CurveType::Linear);

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
        map->tempo_at_tick (tick).in (units::bpm),
        deserialized_map.tempo_at_tick (tick).in (units::bpm));
      auto ts1 = map->time_signature_at_tick (tick);
      auto ts2 = deserialized_map.time_signature_at_tick (tick);
      EXPECT_EQ (ts1.time_signature.numerator, ts2.time_signature.numerator);
      EXPECT_EQ (ts1.time_signature.denominator, ts2.time_signature.denominator);
    }

  // Also test a conversion to be sure
  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (TimelineTick{ units::ticks (2880) })
      .in (units::seconds),
    deserialized_map.tick_to_seconds (TimelineTick{ units::ticks (2880) })
      .in (units::seconds));
  EXPECT_DOUBLE_EQ (
    map->seconds_to_tick (units::seconds (1.5)).asDouble (),
    deserialized_map.seconds_to_tick (units::seconds (1.5)).asDouble ());
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
  EXPECT_DOUBLE_EQ (
    deserialized_map.tempo_at_tick (units::ticks (0)).in (units::bpm), 120.0);
  auto ts = deserialized_map.time_signature_at_tick (units::ticks (0));
  EXPECT_EQ (ts.time_signature.numerator, 4);
  EXPECT_EQ (ts.time_signature.denominator, 4);
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
  TempoMap::TimeSignatureEvent ts4_4{
    units::ticks (0), TimeSignature{ 4, 4 }
  };
  EXPECT_EQ (ts4_4.quarters_per_bar (), 4);
  EXPECT_EQ (ts4_4.ticks_per_bar ().in (units::ticks), 4 * 960);
  EXPECT_EQ (ts4_4.ticks_per_beat ().in (units::ticks), 960);

  // Test 3/4 time signature
  TempoMap::TimeSignatureEvent ts3_4{
    units::ticks (0), TimeSignature{ 3, 4 }
  };
  EXPECT_EQ (ts3_4.quarters_per_bar (), 3);
  EXPECT_EQ (ts3_4.ticks_per_bar ().in (units::ticks), 3 * 960);
  EXPECT_EQ (ts3_4.ticks_per_beat ().in (units::ticks), 960);

  // Test 6/8 time signature
  TempoMap::TimeSignatureEvent ts6_8{
    units::ticks (0), TimeSignature{ 6, 8 }
  };
  EXPECT_EQ (ts6_8.quarters_per_bar (), 3); // (6 * 4) / 8 = 3
  EXPECT_EQ (ts6_8.ticks_per_bar ().in (units::ticks), 3 * 960);
  EXPECT_EQ (ts6_8.ticks_per_beat ().in (units::ticks), 480); // 2880 / 6

  // Test 5/8 time signature
  TempoMap::TimeSignatureEvent ts5_8{
    units::ticks (0), TimeSignature{ 5, 8 }
  };
  EXPECT_EQ (
    ts5_8.quarters_per_bar (), 2); // (5 * 4) / 8 = 2.5, but integer division
  EXPECT_EQ (ts5_8.ticks_per_bar ().in (units::ticks), 2 * 960);
  EXPECT_EQ (ts5_8.ticks_per_beat ().in (units::ticks), 384); // 1920 / 5

  // Test 7/16 time signature
  TempoMap::TimeSignatureEvent ts7_16{
    units::ticks (0), TimeSignature{ 7, 16 }
  };
  EXPECT_EQ (
    ts7_16.quarters_per_bar (), 1); // (7 * 4) / 16 = 1.75, but integer division
  EXPECT_EQ (ts7_16.ticks_per_bar ().in (units::ticks), 1 * 960);
  EXPECT_EQ (
    ts7_16.ticks_per_beat ().in (units::ticks), 137); // 960 / 7 (integer
                                                      // division)

  // Test 2/2 time signature (cut time)
  TempoMap::TimeSignatureEvent ts2_2{
    units::ticks (0), TimeSignature{ 2, 2 }
  };
  EXPECT_EQ (ts2_2.quarters_per_bar (), 4); // (2 * 4) / 2 = 4
  EXPECT_EQ (ts2_2.ticks_per_bar ().in (units::ticks), 4 * 960);
  EXPECT_EQ (ts2_2.ticks_per_beat ().in (units::ticks), 1920); // 3840 / 2

  // Test 12/8 time signature
  TempoMap::TimeSignatureEvent ts12_8{
    units::ticks (0), TimeSignature{ 12, 8 }
  };
  EXPECT_EQ (ts12_8.quarters_per_bar (), 6); // (12 * 4) / 8 = 6
  EXPECT_EQ (ts12_8.ticks_per_bar ().in (units::ticks), 6 * 960);
  EXPECT_EQ (ts12_8.ticks_per_beat ().in (units::ticks), 480); // 5760 / 12
}

// Test TimeSignatureEvent utility methods with time_signature_at_tick
TEST_F (TempoMapTest, TimeSignatureEventUtilityMethodsIntegration)
{
  // Add various time signatures to the tempo map
  map->add_time_signature_event (
    units::ticks (0), TimeSignature{ 4, 4 });        // Bar 1: 4/4
  const auto bar3Start = units::ticks (2 * 4 * 960); // Bar 3 start
  map->add_time_signature_event (bar3Start, TimeSignature{ 3, 4 }); // Bar 3: 3/4
  const auto bar5Start = bar3Start + units::ticks (2 * 3 * 960); // Bar 5 start
  map->add_time_signature_event (bar5Start, TimeSignature{ 6, 8 }); // Bar 5: 6/8

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
  TempoMap::TimeSignatureEvent ts1_4{
    units::ticks (0), TimeSignature{ 1, 4 }
  };
  EXPECT_EQ (ts1_4.quarters_per_bar (), 1);
  EXPECT_EQ (ts1_4.ticks_per_bar ().in (units::ticks), 960);
  EXPECT_EQ (ts1_4.ticks_per_beat ().in (units::ticks), 960);

  // Test with large numbers
  TempoMap::TimeSignatureEvent tsLarge{
    units::ticks (0), TimeSignature{ 32, 32 }
  };
  EXPECT_EQ (tsLarge.quarters_per_bar (), 4); // (32 * 4) / 32 = 4
  EXPECT_EQ (tsLarge.ticks_per_bar ().in (units::ticks), 3840);
  EXPECT_EQ (tsLarge.ticks_per_beat ().in (units::ticks), 120); // 3840 / 32

  // Test with denominator larger than numerator
  TempoMap::TimeSignatureEvent ts3_16{
    units::ticks (0), TimeSignature{ 3, 16 }
  };
  EXPECT_EQ (
    ts3_16.quarters_per_bar (), 0); // (3 * 4) / 16 = 0 (integer division)
  EXPECT_EQ (ts3_16.ticks_per_bar ().in (units::ticks), 0);
  EXPECT_EQ (ts3_16.ticks_per_beat ().in (units::ticks), 0); // 0 / 3 = 0
}

// ---------------------------------------------------------------------------
// Base tempo / base time signature at tick 0 (intrinsic anchor).
// ---------------------------------------------------------------------------

// No inserted events: the base tempo (120 BPM) governs the whole timeline as a
// constant, and tick<->seconds is linear with slope 1/base_bpm.
TEST_F (TempoMapTest, BaseTempoNoEvents)
{
  const int64_t ppq = TempoMap::get_ppq ().in (units::ticks);
  const double  base_bpm = 120.0;
  const double  sec_per_tick = 60.0 / (base_bpm * ppq);
  const int64_t one_beat = ppq; // 1 quarter note = ppq ticks

  EXPECT_DOUBLE_EQ (map->base_bpm ().in (units::bpm), base_bpm);
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (0)).in (units::bpm), base_bpm);
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (5000)).in (units::bpm), base_bpm);

  EXPECT_DOUBLE_EQ (
    map->tick_to_seconds (TimelineTick{ units::ticks (0) }).in (units::seconds),
    0.0);
  // One beat at the base tempo equals 60/base_bpm seconds.
  EXPECT_NEAR (
    map->tick_to_seconds (TimelineTick{ units::ticks (one_beat) })
      .in (units::seconds),
    one_beat * sec_per_tick, 1e-9);
  EXPECT_NEAR (
    map->seconds_to_tick (units::seconds (one_beat * sec_per_tick)).asDouble (),
    one_beat, 1e-6);
}

// First inserted event past tick 0: base tempo (constant) governs [0, event),
// the event governs from its tick onward, and the seconds curve is continuous
// at the boundary.
TEST_F (TempoMapTest, BaseTempoLeadSegmentBeforeFirstEvent)
{
  const int64_t ppq = TempoMap::get_ppq ().in (units::ticks);
  const double  base_bpm = 120.0;
  const double  event_bpm = 140.0;
  const double  sec_per_tick_at_base = 60.0 / (base_bpm * ppq);
  const double  sec_per_tick_at_event = 60.0 / (event_bpm * ppq);
  // Bar 2 in 4/4 = 4 quarters * ppq ticks.
  const int64_t boundary = 4 * ppq;

  map->add_tempo_event (
    units::ticks (boundary), units::bpm (event_bpm),
    TempoMap::CurveType::Constant);

  // Before the event: base tempo.
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (0)).in (units::bpm), base_bpm);
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (boundary - 1)).in (units::bpm), base_bpm);
  // At/after the event: the inserted tempo.
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (boundary)).in (units::bpm), event_bpm);
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (boundary + ppq)).in (units::bpm),
    event_bpm);

  // Lead segment [0, boundary) at the base tempo.
  const double boundary_sec = boundary * sec_per_tick_at_base;
  EXPECT_NEAR (
    map->tick_to_seconds (TimelineTick{ units::ticks (boundary) })
      .in (units::seconds),
    boundary_sec, 1e-9);
  // Continuity just before the boundary.
  EXPECT_NEAR (
    map->tick_to_seconds (TimelineTick{ units::ticks (boundary - 1) })
      .in (units::seconds),
    (boundary - 1) * sec_per_tick_at_base, 1e-9);
  // One tick past the boundary at the event tempo.
  EXPECT_NEAR (
    map->tick_to_seconds (TimelineTick{ units::ticks (boundary + 1) })
      .in (units::seconds),
    boundary_sec + sec_per_tick_at_event, 1e-9);

  // Inverse: the boundary time maps back to the boundary tick.
  EXPECT_NEAR (
    map->seconds_to_tick (units::seconds (boundary_sec)).asDouble (), boundary,
    1e-6);
}

// Editing the base tempo rescales the lead segment.
TEST_F (TempoMapTest, SetBaseBpmRescalesLeadSegment)
{
  const int64_t ppq = TempoMap::get_ppq ().in (units::ticks);
  const double  new_base = 60.0;
  const int64_t boundary = 4 * ppq;
  const double  sec_per_tick_at_base = 60.0 / (new_base * ppq);

  map->add_tempo_event (
    units::ticks (boundary), units::bpm (140.0), TempoMap::CurveType::Constant);
  map->set_base_bpm (units::bpm (new_base));

  EXPECT_DOUBLE_EQ (map->base_bpm ().in (units::bpm), new_base);
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (0)).in (units::bpm), new_base);
  EXPECT_NEAR (
    map->tick_to_seconds (TimelineTick{ units::ticks (boundary) })
      .in (units::seconds),
    boundary * sec_per_tick_at_base, 1e-9);
}

// An inserted event at tick 0 shadows the base tempo for the region it covers.
TEST_F (TempoMapTest, EventAtTickZeroShadowsBase)
{
  const int64_t ppq = TempoMap::get_ppq ().in (units::ticks);
  map->set_base_bpm (units::bpm (100.0));
  map->add_tempo_event (
    units::ticks (0), units::bpm (140.0), TempoMap::CurveType::Constant);

  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (0)).in (units::bpm), 140.0);
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (ppq)).in (units::bpm), 140.0);
  // Base is not consulted; only one event exists.
  EXPECT_EQ (map->tempo_events ().size (), 1u);
}

// The base time signature governs bar numbering over [0, first event).
TEST_F (TempoMapTest, BaseTimeSignatureGovernsLeadRegion)
{
  const int64_t ppq = TempoMap::get_ppq ().in (units::ticks);
  // Base 3/4: denominator 4 -> beat unit is a quarter note (ppq ticks/beat);
  // 3 beats per bar -> 3 * ppq ticks per bar.
  constexpr int num = 3;
  constexpr int den = 4;
  const int64_t ticks_per_bar = (num * 4 / den) * ppq;

  map->set_base_time_signature (TimeSignature{ num, den });

  auto at_tick0 = map->tick_to_musical_position (units::ticks (0));
  EXPECT_EQ (at_tick0.bar, 1);
  EXPECT_EQ (at_tick0.beat, 1);

  // One bar later -> bar 2, beat 1.
  auto at_bar2 = map->tick_to_musical_position (units::ticks (ticks_per_bar));
  EXPECT_EQ (at_bar2.bar, 2);
  EXPECT_EQ (at_bar2.beat, 1);

  // One beat into bar 1 -> beat 2.
  auto at_beat2 = map->tick_to_musical_position (units::ticks (ppq));
  EXPECT_EQ (at_beat2.bar, 1);
  EXPECT_EQ (at_beat2.beat, 2);
}

// Base tempo and base time signature survive serialization roundtrip.
TEST_F (TempoMapTest, BaseValuesSerialization)
{
  map->set_base_bpm (units::bpm (150.0));
  map->set_base_time_signature (TimeSignature{ 6, 8 });

  nlohmann::json j = *map;
  TempoMap       deserialized{ SAMPLE_RATE };
  j.get_to (deserialized);

  // Scalar roundtrip.
  EXPECT_DOUBLE_EQ (deserialized.base_bpm ().in (units::bpm), 150.0);
  EXPECT_EQ (deserialized.base_time_signature ().time_signature.numerator, 6);
  EXPECT_EQ (deserialized.base_time_signature ().time_signature.denominator, 8);

  // Behavioral: base_bpm_ actually drives tempo lookup after load.
  EXPECT_DOUBLE_EQ (
    deserialized.tempo_at_tick (units::ticks (0)).in (units::bpm), 150.0);

  // Behavioral: the rebuilt effective-time-signature cache drives bar
  // numbering after load. 6/8 -> ticks_per_bar = (6 * 4 / 8) * ppq = 3 * ppq,
  // so one bar later lands on bar 2, beat 1. This would catch a regression
  // where rebuild_time_signature_cache() were removed from from_json.
  const int64_t ppq = TempoMap::get_ppq ().in (units::ticks);
  const auto    at_bar2 =
    deserialized.tick_to_musical_position (units::ticks (3 * ppq));
  EXPECT_EQ (at_bar2.bar, 2);
  EXPECT_EQ (at_bar2.beat, 1);
}

// Old project format (no baseBpm/baseTimeSignature keys) loads with default
// base values (120 BPM, 4/4).
TEST_F (TempoMapTest, OldFormatLoadUsesDefaultBase)
{
  nlohmann::json j = R"({
    "timeSignatures": [],
    "tempoChanges": []
  })"_json;

  TempoMap deserialized{ SAMPLE_RATE };
  j.get_to (deserialized);

  EXPECT_DOUBLE_EQ (deserialized.base_bpm ().in (units::bpm), 120.0);
  EXPECT_EQ (deserialized.base_time_signature ().time_signature.numerator, 4);
  EXPECT_EQ (deserialized.base_time_signature ().time_signature.denominator, 4);
  EXPECT_DOUBLE_EQ (
    deserialized.tempo_at_tick (units::ticks (0)).in (units::bpm), 120.0);
}

// An inserted time-signature event at tick 0 shadows the base signature: the
// base must not be prepended into the effective view, and bar numbering follows
// the inserted signature.
TEST_F (TempoMapTest, TimeSignatureAtTickZeroShadowsBase)
{
  const int64_t ppq = TempoMap::get_ppq ().in (units::ticks);
  // Inserted 3/4 at tick 0 shadows the default base 4/4.
  map->add_time_signature_event (units::ticks (0), TimeSignature{ 3, 4 });

  ASSERT_EQ (map->effective_time_signature_events ().size (), 1u);
  const auto &eff0 = map->effective_time_signature_events ()[0];
  EXPECT_EQ (eff0.tick, units::ticks (0));
  EXPECT_EQ (eff0.time_signature.numerator, 3);
  EXPECT_EQ (eff0.time_signature.denominator, 4);

  // 3/4 -> ticks_per_bar = 3 * ppq, so one bar later lands on bar 2.
  const auto at_bar2 = map->tick_to_musical_position (units::ticks (3 * ppq));
  EXPECT_EQ (at_bar2.bar, 2);
  EXPECT_EQ (at_bar2.beat, 1);
}

// Base and inserted time-signature events: bar numbering accumulates correctly
// across the boundary at the inserted event.
TEST_F (TempoMapTest, BaseAndInsertedTimeSignatureBoundary)
{
  const int64_t ppq = TempoMap::get_ppq ().in (units::ticks);
  // Base 4/4 (default). Insert 3/4 at 8 * ppq ticks (2 bars under base 4/4).
  constexpr int64_t boundary_bars = 2;
  const int64_t     boundary = boundary_bars * 4 * ppq;
  map->add_time_signature_event (units::ticks (boundary), TimeSignature{ 3, 4 });

  // Strictly before the boundary: base 4/4 governs.
  EXPECT_EQ (map->tick_to_musical_position (units::ticks (0)).bar, 1);
  EXPECT_EQ (map->tick_to_musical_position (units::ticks (4 * ppq)).bar, 2);

  // At the boundary: 2 bars elapsed under base 4/4, so inserted 3/4 begins at
  // bar 3.
  const auto at_boundary =
    map->tick_to_musical_position (units::ticks (boundary));
  EXPECT_EQ (at_boundary.bar, 3);
  EXPECT_EQ (at_boundary.beat, 1);

  // One bar past the boundary under 3/4 (3 * ppq ticks/bar) -> bar 4.
  const auto after_boundary =
    map->tick_to_musical_position (units::ticks (boundary + 3 * ppq));
  EXPECT_EQ (after_boundary.bar, 4);
  EXPECT_EQ (after_boundary.beat, 1);
}

// Forward musical_position_to_tick with a non-default base signature, verified
// as a round-trip against tick_to_musical_position.
TEST_F (TempoMapTest, MusicalPositionToTickForwardWithBaseSignature)
{
  const int64_t ppq = TempoMap::get_ppq ().in (units::ticks);
  // Base 6/8: quarters_per_bar = 6 * (4/8) = 3 -> ticks_per_bar = 3 * ppq.
  map->set_base_time_signature (TimeSignature{ 6, 8 });

  const TempoMap::MusicalPosition pos{ 2, 1, 1, 0 };
  const auto forward_tick = map->musical_position_to_tick (pos);
  // Bar 2 starts one bar past tick 0 -> 3 * ppq ticks.
  EXPECT_DOUBLE_EQ (forward_tick.asDouble (), 3.0 * ppq);

  // Inverse round-trip back to the original musical position.
  const auto back = map->tick_to_musical_position (units::ticks (3 * ppq));
  EXPECT_EQ (back, pos);
}

// After removing the only inserted tempo and time-signature events, lookups
// must fall back to the base values across the timeline.
TEST_F (TempoMapTest, RemoveEventRestoresBaseGovernance)
{
  const int64_t ppq = TempoMap::get_ppq ().in (units::ticks);

  map->set_base_bpm (units::bpm (100.0));
  map->set_base_time_signature (TimeSignature{ 3, 4 });
  // Time signatures must be added before tempo events.
  map->add_time_signature_event (units::ticks (4 * ppq), TimeSignature{ 5, 8 });
  map->add_tempo_event (
    units::ticks (4 * ppq), units::bpm (140.0), TempoMap::CurveType::Constant);

  // Sanity: at the event tick the inserted values govern.
  EXPECT_DOUBLE_EQ (
    map->tempo_at_tick (units::ticks (4 * ppq)).in (units::bpm), 140.0);
  EXPECT_EQ (
    map->time_signature_at_tick (units::ticks (4 * ppq)).time_signature.numerator,
    5);
  EXPECT_EQ (
    map->time_signature_at_tick (units::ticks (4 * ppq))
      .time_signature.denominator,
    8);

  // Remove both -> base values must govern everywhere again.
  map->remove_tempo_event (units::ticks (4 * ppq));
  map->remove_time_signature_event (units::ticks (4 * ppq));

  EXPECT_DOUBLE_EQ (map->base_bpm ().in (units::bpm), 100.0);
  for (const int64_t t : { int64_t{ 0 }, ppq, 4 * ppq, 8 * ppq })
    {
      SCOPED_TRACE (t);
      EXPECT_DOUBLE_EQ (
        map->tempo_at_tick (units::ticks (t)).in (units::bpm), 100.0);
      EXPECT_EQ (
        map->time_signature_at_tick (units::ticks (t)).time_signature.numerator,
        3);
      EXPECT_EQ (
        map->time_signature_at_tick (units::ticks (t)).time_signature.denominator,
        4);
    }
}
}
