// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/position.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::dsp;

class PositionTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    sample_rate_ = 44100;
    frames_per_tick_ = FramesPerTick{ 22.675736961451247 };
    ticks_per_frame_ = to_ticks_per_frame (frames_per_tick_);
    beats_per_bar_ = 4;
    sixteenths_per_beat_ = 4;
    ticks_per_bar_ =
      Position::TICKS_PER_SIXTEENTH_NOTE * sixteenths_per_beat_ * beats_per_bar_;
  }

  sample_rate_t sample_rate_{};
  FramesPerTick frames_per_tick_;
  TicksPerFrame ticks_per_frame_;
  int           beats_per_bar_{};
  int           sixteenths_per_beat_{};
  int           ticks_per_bar_{};
};

TEST_F (PositionTest, Construction)
{
  // Default construction
  Position pos1;
  EXPECT_EQ (pos1.frames_, 0);
  EXPECT_EQ (pos1.ticks_, 0.0);

  // Construction from ticks
  Position pos2 (1920.0, frames_per_tick_);
  EXPECT_EQ (pos2.ticks_, 1920.0);
  EXPECT_GT (pos2.frames_, 0);

  // Construction from frames
  Position pos3 ((signed_frame_t) 44100, ticks_per_frame_);
  EXPECT_GT (pos3.ticks_, 0.0);
  EXPECT_EQ (pos3.frames_, 44100);

  // String construction
  Position pos4 (
    "2.1.1.0", beats_per_bar_, sixteenths_per_beat_, frames_per_tick_);
  EXPECT_GT (pos4.ticks_, 0.0);
  EXPECT_GT (pos4.frames_, 0);
}

TEST_F (PositionTest, TimeConversions)
{
  Position pos;

  // Test seconds conversion
  pos.from_seconds (2.0, sample_rate_, ticks_per_frame_);
  EXPECT_EQ (pos.frames_, 88200);

  // Test milliseconds
  auto ms = pos.to_ms (sample_rate_);
  EXPECT_EQ (ms, 2000);

  // Test frames/ticks conversion
  Position pos2;
  pos2.from_frames (44100, ticks_per_frame_);
  pos2.update_ticks_from_frames (ticks_per_frame_);
  pos2.update_frames_from_ticks (frames_per_tick_);
  EXPECT_GT (pos2.ticks_, 0.0);
}

TEST_F (PositionTest, MusicalPositioning)
{
  Position pos;

  // Test bar positioning
  pos.set_to_bar (4, ticks_per_bar_, frames_per_tick_);
  EXPECT_EQ (pos.get_bars (true, ticks_per_bar_), 4);

  // Test beat additions
  pos.zero ();
  pos.add_beats (2, Position::TICKS_PER_QUARTER_NOTE, frames_per_tick_);
  EXPECT_EQ (
    pos.get_total_beats (
      true, beats_per_bar_, Position::TICKS_PER_QUARTER_NOTE, frames_per_tick_),
    2);

  // Test sixteenths
  pos.zero ();
  pos.add_sixteenths (4, frames_per_tick_);
  EXPECT_EQ (pos.get_total_sixteenths (true, frames_per_tick_), 4);
}

TEST_F (PositionTest, Comparisons)
{
  Position pos1 (1920.0, frames_per_tick_);
  Position pos2 (3840.0, frames_per_tick_);

  EXPECT_LT (pos1, pos2);
  EXPECT_GT (pos2, pos1);

  Position pos3 (1920.0, frames_per_tick_);
  EXPECT_EQ (pos1, pos3);

  EXPECT_TRUE (pos1.is_between_excl_2nd (Position (), pos2));
  EXPECT_TRUE (pos2.is_between_incl_2nd (pos1, pos2));
}

TEST_F (PositionTest, StringFormatting)
{
  Position pos (1920.0, frames_per_tick_);
  auto     str =
    pos.to_string (beats_per_bar_, sixteenths_per_beat_, frames_per_tick_);
  EXPECT_FALSE (str.empty ());

  // Test string parsing back
  Position pos2 (
    str.c_str (), beats_per_bar_, sixteenths_per_beat_, frames_per_tick_);
  EXPECT_EQ (pos.frames_, pos2.frames_);
  EXPECT_STREQ (
    str.c_str (),
    pos2.to_string (beats_per_bar_, sixteenths_per_beat_, frames_per_tick_)
      .c_str ());
}

TEST_F (PositionTest, NegativePositions)
{
  // Test negative position string parsing
  Position pos1 (
    "-6.1.1.0", beats_per_bar_, sixteenths_per_beat_, frames_per_tick_);
  EXPECT_LT (pos1.ticks_, 0.0);
  EXPECT_LT (pos1.frames_, 0);

  // Verify string conversion roundtrip
  auto str =
    pos1.to_string (beats_per_bar_, sixteenths_per_beat_, frames_per_tick_);
  Position pos2 (
    str.c_str (), beats_per_bar_, sixteenths_per_beat_, frames_per_tick_);
  EXPECT_EQ (pos1.frames_, pos2.frames_);
  EXPECT_EQ (pos1.ticks_, pos2.ticks_);
}

TEST_F (PositionTest, RejectsZeroBasedPositions)
{
  EXPECT_ANY_THROW (Position (
    "0.1.1.0", beats_per_bar_, sixteenths_per_beat_, frames_per_tick_));
  EXPECT_ANY_THROW (Position (
    "1.0.1.0", beats_per_bar_, sixteenths_per_beat_, frames_per_tick_));
  EXPECT_ANY_THROW (Position (
    "1.1.0.0", beats_per_bar_, sixteenths_per_beat_, frames_per_tick_));
}

TEST_F (PositionTest, Serialization)
{
  Position pos1;
  pos1.ticks_ = 1920.0;
  pos1.frames_ = 43534;

  // Serialize to JSON
  auto json_str = pos1.serialize_to_json_string ();

  // Create new object and deserialize
  Position pos2;
  pos2.deserialize_from_json_string (json_str.c_str ());

  // Verify all fields match
  EXPECT_EQ (pos1.ticks_, pos2.ticks_);
  EXPECT_EQ (pos1.frames_, pos2.frames_);
}
