// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dsp/position.h"
#include "utils/math.h"
#include "utils/string.h"

#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("dsp/position");

TEST_CASE_FIXTURE (ZrythmFixture, "conversions")
{
  Position pos;
  pos.from_ticks (10'000);
  REQUIRE_GT (pos.frames_, 0);

  pos.zero();
  pos.from_frames (10'000);
  REQUIRE_GT (pos.ticks_, 0);
}

TEST_CASE_FIXTURE (ZrythmFixture, "get totals")
{
  Position pos;
  REQUIRE_EQ (pos.get_total_beats (false), 0);

  REQUIRE_EQ (pos.get_total_beats (false), 0);

  REQUIRE_EQ (pos.get_total_sixteenths (false), 0);

  pos.add_sixteenths (1);

  REQUIRE_EQ (pos.get_total_bars (false), 0);
  REQUIRE_EQ (pos.get_total_bars (true), 0);

  REQUIRE_EQ (pos.get_total_beats (false), 0);
  REQUIRE_EQ (pos.get_total_beats (true), 0);

  REQUIRE_EQ (pos.get_total_sixteenths (false), 0);
  REQUIRE_EQ (pos.get_total_sixteenths (true), 1);

  pos.zero ();
  pos.add_beats (1);

  REQUIRE_EQ (pos.get_total_bars (false), 0);
  REQUIRE_EQ (pos.get_total_bars (true), 0);

  REQUIRE_EQ (pos.get_total_beats (false), 0);
  REQUIRE_EQ (pos.get_total_beats (true), 1);

  REQUIRE_EQ (
    pos.get_total_sixteenths (false),
    pos.get_total_beats (true) * (int) TRANSPORT->sixteenths_per_beat_ - 1);
  REQUIRE_EQ (
    pos.get_total_sixteenths (true),
    pos.get_total_beats (true) * (int) TRANSPORT->sixteenths_per_beat_);

  pos.zero ();
  pos.add_bars (1);

  REQUIRE_EQ (pos.get_total_bars (false), 0);
  REQUIRE_EQ (pos.get_total_bars (true), 1);

  REQUIRE_EQ (pos.get_total_beats (false), 3);
  REQUIRE_EQ (pos.get_total_beats (true), 4);

  REQUIRE_EQ (
    pos.get_total_sixteenths (false),
    pos.get_total_beats (true) * (int) TRANSPORT->sixteenths_per_beat_ - 1);
  REQUIRE_EQ (
    pos.get_total_sixteenths (true),
    pos.get_total_beats (true) * (int) TRANSPORT->sixteenths_per_beat_);
}

TEST_CASE_FIXTURE (ZrythmFixture, "set to x")
{
  auto check_after_set_to_bar = [] (const int bar, const char * expect) {
    Position pos;
    pos.set_to_bar (bar);
    auto res = pos.to_string ();
    REQUIRE (string_contains_substr (res.c_str (), expect));
  };

  check_after_set_to_bar (4, "4.1.1.0");
  check_after_set_to_bar (1, "1.1.1.0");
}

TEST_CASE_FIXTURE (ZrythmFixture, "print position")
{
  z_debug ("---");

  Position pos;
  for (int i = 0; i < 2000; i++)
    {
      pos.add_ticks (2.1);
      pos.print ();
    }

  z_debug ("---");

  for (int i = 0; i < 2000; i++)
    {
      pos.add_ticks (-4.1);
      pos.print ();
    }

  z_debug ("---");
}

TEST_CASE_FIXTURE (ZrythmFixture, "position from ticks")
{
  Position pos;
  double   ticks =
    50'000.0;

    /* assert values are correct */
    pos.from_ticks (ticks);
  REQUIRE_EQ (
    pos.get_bars (true),
    math_round_double_to_signed_32 (ticks / TRANSPORT->ticks_per_bar_ + 1));
  REQUIRE_GT (pos.get_bars (true), 0);
}

TEST_CASE_FIXTURE (ZrythmFixture, "position to frames")
{
  Position pos;
  double   ticks = 50000.0;

  /* assert values are correct */
  pos.from_ticks (ticks);
  auto frames = pos.frames_;
  REQUIRE_EQ (
    frames,
    math_round_double_to_signed_64 (AUDIO_ENGINE->frames_per_tick_ * ticks));
}

TEST_CASE_FIXTURE (ZrythmFixture, "get total beats")
{
  Position start_pos{ 4782.3818594104323 };
  Position end_pos{ 4800.0290249433119 };
  end_pos.frames_ = 69632;
  start_pos.from_ticks (4782.3818594104323);
  end_pos.from_ticks (4800);

  int beats = 0;
  beats = start_pos.get_total_beats (false);
  REQUIRE_EQ (beats, 4);
  beats = end_pos.get_total_beats (false);
  REQUIRE_EQ (beats, 4);

  end_pos.from_ticks (4800.0290249433119);
  beats = end_pos.get_total_beats (false);
  REQUIRE_EQ (beats, 5);
}

TEST_CASE_FIXTURE (ZrythmFixture, "position benchmarks")
{
  double   ticks = 50000.0;
  gint64   loop_times = 5;
  gint     total_time;
  Position pos{ ticks };

  z_info ("add frames");
  total_time = 0;
  for (int j = 0; j < loop_times; j++)
    {
      gint64 before = g_get_monotonic_time ();
      for (int i = 0; i < 100000; i++)
        {
          pos.add_frames (1000);
        }
      gint64 after = g_get_monotonic_time ();
      total_time += after - before;
    }
  z_info ("time: {}", total_time / loop_times);
}

TEST_CASE_FIXTURE (ZrythmFixture, "snap")
{
  REQUIRE_EQ (
    SNAP_GRID_TIMELINE->snap_note_length_, NoteLength::NOTE_LENGTH_BAR);

  /* test without keep offset */
  Position start_pos;
  start_pos.set_to_bar (4);
  Position cur_pos;
  cur_pos.set_to_bar (4);
  cur_pos.add_ticks (2 * TICKS_PER_QUARTER_NOTE + 10);
  Position test_pos;
  test_pos.set_to_bar (5);
  cur_pos.snap (&start_pos, nullptr, nullptr, *SNAP_GRID_TIMELINE);
  REQUIRE_POSITION_EQ (cur_pos, test_pos);

  cur_pos.set_to_bar (4);
  test_pos.set_to_bar (4);
  cur_pos.add_ticks (2 * TICKS_PER_QUARTER_NOTE - 10);
  cur_pos.snap (&start_pos, nullptr, nullptr, *SNAP_GRID_TIMELINE);
  REQUIRE_POSITION_EQ (cur_pos, test_pos);

  z_info ("-----");

  /* test keep offset */
  SNAP_GRID_TIMELINE->snap_to_grid_keep_offset_ = true;
  start_pos.set_to_bar (4);
  start_pos.add_ticks (2 * TICKS_PER_QUARTER_NOTE);
  cur_pos = start_pos;
  cur_pos.add_ticks (10);
  test_pos = start_pos;
  cur_pos.snap (&start_pos, nullptr, nullptr, *SNAP_GRID_TIMELINE);
  z_info ("cur pos {} test pos {}", cur_pos.to_string (), test_pos.to_string ());
  REQUIRE_POSITION_EQ (cur_pos, test_pos);

  start_pos.set_to_bar (4);
  start_pos.add_ticks (2 * TICKS_PER_QUARTER_NOTE);
  cur_pos = start_pos;
  cur_pos.add_ticks (-10);
  test_pos = start_pos;
  cur_pos.snap (&start_pos, nullptr, nullptr, *SNAP_GRID_TIMELINE);
  z_info ("cur pos {} test pos {}", cur_pos, test_pos);
  REQUIRE_POSITION_EQ (cur_pos, test_pos);
}

TEST_SUITE_END;