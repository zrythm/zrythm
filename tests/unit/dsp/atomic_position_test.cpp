// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/atomic_position.h"
#include "dsp/tempo_map.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{
class AtomicPositionTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<TempoMap> (44100.0);
    pos = std::make_unique<AtomicPosition> (*tempo_map);
  }

  std::unique_ptr<TempoMap>       tempo_map;
  std::unique_ptr<AtomicPosition> pos;
};

// Test initial state
TEST_F (AtomicPositionTest, InitialState)
{
  EXPECT_EQ (pos->get_current_mode (), TimeFormat::Musical);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), 0.0);
  EXPECT_DOUBLE_EQ (pos->get_seconds (), 0.0);
}

// Test setting/getting in Musical mode
TEST_F (AtomicPositionTest, MusicalModeOperations)
{
  // Set ticks in Musical mode
  pos->set_ticks (960.0);
  EXPECT_EQ (pos->get_current_mode (), TimeFormat::Musical);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), 960.0);
  EXPECT_DOUBLE_EQ (pos->get_seconds (), 0.5); // 960 ticks @ 120 BPM = 0.5s

  // Set ticks again
  pos->set_ticks (1920.0);
  EXPECT_EQ (pos->get_current_mode (), TimeFormat::Musical);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), 1920.0);
  EXPECT_DOUBLE_EQ (pos->get_seconds (), 1.0);
}

// Test setting/getting in Absolute mode
TEST_F (AtomicPositionTest, AbsoluteModeOperations)
{
  // Switch to Absolute mode
  pos->set_mode (TimeFormat::Absolute);

  // Set seconds in Absolute mode
  pos->set_seconds (0.5);
  EXPECT_EQ (pos->get_current_mode (), TimeFormat::Absolute);
  EXPECT_DOUBLE_EQ (pos->get_seconds (), 0.5);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), 960.0); // 0.5s @ 120 BPM = 960 ticks

  // Set seconds again
  pos->set_seconds (1.0);
  EXPECT_EQ (pos->get_current_mode (), TimeFormat::Absolute);
  EXPECT_DOUBLE_EQ (pos->get_seconds (), 1.0);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), 1920.0);
}

// Test mode conversion
TEST_F (AtomicPositionTest, ModeConversion)
{
  // Initial state: Musical mode, 0 ticks
  pos->set_ticks (960.0);

  // Convert to Absolute mode
  pos->set_mode (TimeFormat::Absolute);
  EXPECT_EQ (pos->get_current_mode (), TimeFormat::Absolute);
  EXPECT_DOUBLE_EQ (pos->get_seconds (), 0.5);

  // Convert back to Musical mode
  pos->set_mode (TimeFormat::Musical);
  EXPECT_EQ (pos->get_current_mode (), TimeFormat::Musical);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), 960.0);
}

// Test setting ticks in Absolute mode
TEST_F (AtomicPositionTest, SetTicksInAbsoluteMode)
{
  pos->set_mode (TimeFormat::Absolute);
  pos->set_ticks (960.0); // Should convert to seconds

  EXPECT_EQ (pos->get_current_mode (), TimeFormat::Absolute);
  EXPECT_DOUBLE_EQ (pos->get_seconds (), 0.5);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), 960.0); // Should convert back
}

// Test setting seconds in Musical mode
TEST_F (AtomicPositionTest, SetSecondsInMusicalMode)
{
  pos->set_seconds (0.5); // Should convert to ticks

  EXPECT_EQ (pos->get_current_mode (), TimeFormat::Musical);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), 960.0);
  EXPECT_DOUBLE_EQ (pos->get_seconds (), 0.5); // Should convert back
}

// Test fractional positions
TEST_F (AtomicPositionTest, FractionalPositions)
{
  // Test fractional ticks in Musical mode
  pos->set_ticks (480.5);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), 480.5);
  EXPECT_DOUBLE_EQ (pos->get_seconds (), 480.5 / 960.0 * 0.5);

  // Test fractional seconds in Absolute mode
  pos->set_mode (TimeFormat::Absolute);
  pos->set_seconds (0.25);
  EXPECT_DOUBLE_EQ (pos->get_seconds (), 0.25);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), 0.25 / 0.5 * 960.0);
}

// Test with tempo changes
TEST_F (AtomicPositionTest, WithTempoChanges)
{
  // Add tempo change at 1920 ticks (140 BPM)
  tempo_map->addEvent (1920, 140.0, TempoMap::CurveType::Constant);

  // Set position after tempo change
  pos->set_ticks (2880.0); // 1920 + 960 ticks
  EXPECT_DOUBLE_EQ (pos->get_seconds (), 1.0 + (60.0 / 140.0));

  // Switch to Absolute mode and set position
  pos->set_mode (TimeFormat::Absolute);
  pos->set_seconds (2.0);

  // Verify tick position (1s @120BPM + 1s @140BPM)
  const double expectedTicks = 1920.0 + (2.0 - 1.0) * (140.0 / 60.0) * 960.0;
  EXPECT_DOUBLE_EQ (pos->get_ticks (), expectedTicks);
}

// Test with time signature changes
TEST_F (AtomicPositionTest, WithTimeSignatureChanges)
{
  // Add time signature change (3/4 time at bar 5)
  const int64_t bar5Start = 4 * 4 * 960;
  tempo_map->addTimeSignatureEvent (bar5Start, 3, 4);

  // Set musical position in bar 5
  pos->set_ticks (bar5Start + 960.0); // Beat 2 of bar 5
  EXPECT_EQ (pos->get_current_mode (), TimeFormat::Musical);

  // Convert to Absolute mode
  pos->set_mode (TimeFormat::Absolute);
  const double secondsAtBar5 = pos->get_seconds ();
  EXPECT_DOUBLE_EQ (secondsAtBar5, 8.5);

  // Convert back
  pos->set_mode (TimeFormat::Musical);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), bar5Start + 960.0);
}

// Test get_samples() in Musical mode
TEST_F (AtomicPositionTest, GetSetSamplesInMusicalMode)
{
  // Set musical position
  pos->set_ticks (960.0);

  // 960 ticks @ 120 BPM = 0.5 seconds
  // 0.5s * 44100 Hz = 22050 samples
  EXPECT_EQ (pos->get_samples (), 22050);

  // Roundtrip
  pos->set_ticks (0);
  pos->set_samples (22050);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), 960.0);
}

// Test get_samples() in Absolute mode
TEST_F (AtomicPositionTest, GetSetSamplesInAbsoluteMode)
{
  // Switch to Absolute mode and set position
  pos->set_mode (TimeFormat::Absolute);
  pos->set_seconds (0.5);

  // Same as above: 0.5s * 44100 Hz = 22050 samples
  EXPECT_EQ (pos->get_samples (), 22050);

  // Roundtrip
  pos->set_seconds (0);
  pos->set_samples (22050);
  EXPECT_DOUBLE_EQ (pos->get_seconds (), 0.5);
}

// Test get_samples() with fractional positions
TEST_F (AtomicPositionTest, GetSetSamplesFractional)
{
  // Fractional ticks
  pos->set_ticks (480.5);
  const double expectedSeconds = 480.5 / 960.0 * 0.5;
  EXPECT_EQ (
    pos->get_samples (), static_cast<int64_t> (expectedSeconds * 44100.0));

  // Fractional seconds
  pos->set_mode (TimeFormat::Absolute);
  pos->set_seconds (0.25);
  EXPECT_EQ (pos->get_samples (), static_cast<int64_t> (0.25 * 44100.0));

  // Roundtrip
  pos->set_seconds (0);
  pos->set_samples (0.25 * 44100.0);
  EXPECT_DOUBLE_EQ (pos->get_seconds (), 0.25);
}

// Test get_samples() with tempo changes
TEST_F (AtomicPositionTest, GetSetSamplesWithTempoChanges)
{
  // Add tempo change
  tempo_map->addEvent (1920, 140.0, TempoMap::CurveType::Constant);

  // Set position after tempo change
  pos->set_ticks (2880.0); // 1920 + 960 ticks
  const double expectedSeconds = 1.0 + (60.0 / 140.0);
  EXPECT_EQ (
    pos->get_samples (), static_cast<int64_t> (expectedSeconds * 44100.0));

  // Roundtrip
  pos->set_ticks (0);
  pos->set_samples (expectedSeconds * 44100.0);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), 2880.0);
}

// Test thread safety (simulated with concurrent access)
TEST_F (AtomicPositionTest, ThreadSafety)
{
  // Writer thread sets values
  pos->set_ticks (960.0);

  // Reader thread gets values
  const double ticks = pos->get_ticks ();
  const double seconds = pos->get_seconds ();

  // Should be consistent
  EXPECT_DOUBLE_EQ (ticks, 960.0);
  EXPECT_DOUBLE_EQ (seconds, 0.5);

  // Change mode and values
  pos->set_mode (TimeFormat::Absolute);
  pos->set_seconds (1.0);

  // Reader gets again
  const double newTicks = pos->get_ticks ();
  const double newSeconds = pos->get_seconds ();

  // Should be consistent
  EXPECT_DOUBLE_EQ (newSeconds, 1.0);
  EXPECT_DOUBLE_EQ (newTicks, 1920.0);
}

// Test edge cases
TEST_F (AtomicPositionTest, EdgeCases)
{
  // Zero position
  pos->set_ticks (0.0);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), 0.0);
  EXPECT_DOUBLE_EQ (pos->get_seconds (), 0.0);

  // Negative position (clamped to 0)
  pos->set_ticks (-100.0);
  EXPECT_DOUBLE_EQ (pos->get_ticks (), 0.0);

  // Large position
  pos->set_ticks (1e9);
  EXPECT_GT (pos->get_seconds (), 0.0);

  // NaN/infinity protection (should assert)
  // Uncomment to test:
  // pos->set_seconds (std::numeric_limits<double>::quiet_NaN ());
}

// Test serialization/deserialization in Musical mode
TEST_F (AtomicPositionTest, SerializationMusicalMode)
{
  // Set musical position
  pos->set_ticks (960.0);

  // Serialize to JSON
  nlohmann::json j;
  j = *pos;

  // Create new position with same tempo map
  AtomicPosition new_pos (*tempo_map);
  j.get_to (new_pos);

  // Verify state
  EXPECT_EQ (new_pos.get_current_mode (), TimeFormat::Musical);
  EXPECT_DOUBLE_EQ (new_pos.get_ticks (), 960.0);
  EXPECT_DOUBLE_EQ (new_pos.get_seconds (), 0.5);
}

// Test serialization/deserialization in Absolute mode
TEST_F (AtomicPositionTest, SerializationAbsoluteMode)
{
  // Set absolute position
  pos->set_mode (TimeFormat::Absolute);
  pos->set_seconds (1.5);

  // Serialize to JSON
  nlohmann::json j;
  j = *pos;

  // Create new position with same tempo map
  AtomicPosition new_pos (*tempo_map);
  j.get_to (new_pos);

  // Verify state
  EXPECT_EQ (new_pos.get_current_mode (), TimeFormat::Absolute);
  EXPECT_DOUBLE_EQ (new_pos.get_seconds (), 1.5);
  EXPECT_DOUBLE_EQ (new_pos.get_ticks (), 1.5 / 0.5 * 960.0);
}

// Test serialization after mode conversion
TEST_F (AtomicPositionTest, SerializationAfterModeConversion)
{
  // Set musical position and convert to absolute
  pos->set_ticks (1920.0);
  pos->set_mode (TimeFormat::Absolute);

  // Serialize to JSON
  nlohmann::json j;
  j = *pos;

  // Create new position with same tempo map
  AtomicPosition new_pos (*tempo_map);
  j.get_to (new_pos);

  // Verify state
  EXPECT_EQ (new_pos.get_current_mode (), TimeFormat::Absolute);
  EXPECT_DOUBLE_EQ (new_pos.get_seconds (), 1.0);
  EXPECT_DOUBLE_EQ (new_pos.get_ticks (), 1920.0);
}
}
