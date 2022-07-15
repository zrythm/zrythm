// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "audio/position.h"
#include "utils/math.h"
#include "utils/string.h"

#include "tests/helpers/zrythm.h"

static void
test_conversions (void)
{
  test_helper_zrythm_init ();

  Position pos;
  position_init (&pos);
  position_from_ticks (&pos, 10000);
  g_assert_cmpint (pos.frames, >, 0);

  position_init (&pos);
  position_from_frames (&pos, 10000);
  g_assert_cmpint ((int) pos.ticks, >, 0);

  test_helper_zrythm_cleanup ();
}

static void
test_get_totals (void)
{
  test_helper_zrythm_init ();

  Position pos;
  position_init (&pos);
  int ret = position_get_total_beats (&pos, false);
  g_assert_cmpint (ret, ==, 0);

  ret = position_get_total_beats (&pos, false);
  g_assert_cmpint (ret, ==, 0);

  ret = position_get_total_sixteenths (&pos, false);
  g_assert_cmpint (ret, ==, 0);

  position_add_sixteenths (&pos, 1);

  ret = position_get_total_bars (&pos, false);
  g_assert_cmpint (ret, ==, 0);
  ret = position_get_total_bars (&pos, true);
  g_assert_cmpint (ret, ==, 0);

  ret = position_get_total_beats (&pos, false);
  g_assert_cmpint (ret, ==, 0);
  ret = position_get_total_beats (&pos, true);
  g_assert_cmpint (ret, ==, 0);

  ret = position_get_total_sixteenths (&pos, false);
  g_assert_cmpint (ret, ==, 0);
  ret = position_get_total_sixteenths (&pos, true);
  g_assert_cmpint (ret, ==, 1);

  position_init (&pos);
  position_add_beats (&pos, 1);

  ret = position_get_total_bars (&pos, false);
  g_assert_cmpint (ret, ==, 0);
  ret = position_get_total_bars (&pos, true);
  g_assert_cmpint (ret, ==, 0);

  ret = position_get_total_beats (&pos, false);
  g_assert_cmpint (ret, ==, 0);
  ret = position_get_total_beats (&pos, true);
  g_assert_cmpint (ret, ==, 1);

  ret = position_get_total_sixteenths (&pos, false);
  g_assert_cmpint (
    ret, ==,
    position_get_total_beats (&pos, true)
        * (int) TRANSPORT->sixteenths_per_beat
      - 1);
  ret = position_get_total_sixteenths (&pos, true);
  g_assert_cmpint (
    ret, ==,
    position_get_total_beats (&pos, true)
      * (int) TRANSPORT->sixteenths_per_beat);

  position_init (&pos);
  position_add_bars (&pos, 1);

  ret = position_get_total_bars (&pos, false);
  g_assert_cmpint (ret, ==, 0);
  ret = position_get_total_bars (&pos, true);
  g_assert_cmpint (ret, ==, 1);

  ret = position_get_total_beats (&pos, false);
  g_assert_cmpint (ret, ==, 3);
  ret = position_get_total_beats (&pos, true);
  g_assert_cmpint (ret, ==, 4);

  ret = position_get_total_sixteenths (&pos, false);
  g_assert_cmpint (
    ret, ==,
    position_get_total_beats (&pos, true)
        * (int) TRANSPORT->sixteenths_per_beat
      - 1);
  ret = position_get_total_sixteenths (&pos, true);
  g_assert_cmpint (
    ret, ==,
    position_get_total_beats (&pos, true)
      * (int) TRANSPORT->sixteenths_per_beat);

  test_helper_zrythm_cleanup ();
}

static void
test_set_to (void)
{
  test_helper_zrythm_init ();

  Position     pos;
  const char * expect = "4.1.1.0";
  char         res[400];

  position_set_to_bar (&pos, 4);
  position_to_string (&pos, res);
  g_assert_true (string_contains_substr (res, expect));

  expect = "1.1.1.0";
  position_set_to_bar (&pos, 1);
  position_to_string (&pos, res);
  g_assert_true (string_contains_substr (res, expect));

  test_helper_zrythm_cleanup ();
}

static void
test_print_position (void)
{
  test_helper_zrythm_init ();

  g_debug ("---");

  Position pos;
  position_init (&pos);
  for (int i = 0; i < 2000; i++)
    {
      position_add_ticks (&pos, 2.1);
      position_print (&pos);
    }

  g_debug ("---");

  for (int i = 0; i < 2000; i++)
    {
      position_add_ticks (&pos, -4.1);
      position_print (&pos);
    }

  g_debug ("---");

  test_helper_zrythm_cleanup ();
}

static void
test_position_from_ticks (void)
{
  test_helper_zrythm_init ();

  Position pos;
  double   ticks = 50000.0;

  /* assert values are correct */
  position_from_ticks (&pos, ticks);
  g_assert_cmpint (
    position_get_bars (&pos, true), ==,
    math_round_double_to_signed_32 (
      ticks / TRANSPORT->ticks_per_bar + 1));
  g_assert_cmpint (position_get_bars (&pos, true), >, 0);

  test_helper_zrythm_cleanup ();
}

static void
test_position_to_frames (void)
{
  test_helper_zrythm_init ();

  Position pos;
  double   ticks = 50000.0;

  /* assert values are correct */
  position_from_ticks (&pos, ticks);
  long frames = position_to_frames (&pos);
  g_assert_cmpint (
    frames, ==,
    math_round_double_to_signed_64 (
      AUDIO_ENGINE->frames_per_tick * ticks));

  test_helper_zrythm_cleanup ();
}

static void
test_get_total_beats (void)
{
  test_helper_zrythm_init ();

  Position start_pos = {
    .ticks = 4782.3818594104323, .frames = 69376
  };
  Position end_pos = {
    .ticks = 4800.0290249433119, .frames = 69632
  };
  position_from_ticks (&start_pos, 4782.3818594104323);
  position_from_ticks (&end_pos, 4800);

  int beats = 0;
  beats = position_get_total_beats (&start_pos, false);
  g_assert_cmpint (beats, ==, 4);
  beats = position_get_total_beats (&end_pos, false);
  g_assert_cmpint (beats, ==, 4);

  position_from_ticks (&end_pos, 4800.0290249433119);
  beats = position_get_total_beats (&end_pos, false);
  g_assert_cmpint (beats, ==, 5);

  test_helper_zrythm_cleanup ();
}

static void
test_position_benchmarks (void)
{
  test_helper_zrythm_init ();

  double   ticks = 50000.0;
  gint64   loop_times = 5;
  gint     total_time;
  Position pos;
  position_from_ticks (&pos, ticks);

  g_message ("add frames");
  total_time = 0;
  for (int j = 0; j < loop_times; j++)
    {
      gint64 before = g_get_monotonic_time ();
      for (int i = 0; i < 100000; i++)
        {
          position_add_frames (&pos, 1000);
        }
      gint64 after = g_get_monotonic_time ();
      total_time += after - before;
    }
  g_message ("time: %ld", total_time / loop_times);

  test_helper_zrythm_cleanup ();
}

static void
test_snap (void)
{
  test_helper_zrythm_init ();

  g_assert_cmpint (
    SNAP_GRID_TIMELINE->snap_note_length, ==, NOTE_LENGTH_BAR);

  /* test without keep offset */
  Position start_pos;
  position_set_to_bar (&start_pos, 4);
  Position cur_pos;
  position_set_to_bar (&cur_pos, 4);
  position_add_ticks (
    &cur_pos, 2 * TICKS_PER_QUARTER_NOTE + 10);
  Position test_pos;
  position_set_to_bar (&test_pos, 5);
  position_snap (
    &start_pos, &cur_pos, NULL, NULL, SNAP_GRID_TIMELINE);
  g_assert_cmppos (&cur_pos, &test_pos);

  position_set_to_bar (&cur_pos, 4);
  position_set_to_bar (&test_pos, 4);
  position_add_ticks (
    &cur_pos, 2 * TICKS_PER_QUARTER_NOTE - 10);
  position_snap (
    &start_pos, &cur_pos, NULL, NULL, SNAP_GRID_TIMELINE);
  g_assert_cmppos (&cur_pos, &test_pos);

  g_message ("-----");

  /* test keep offset */
  SNAP_GRID_TIMELINE->snap_to_grid_keep_offset = true;
  position_set_to_bar (&start_pos, 4);
  position_add_ticks (&start_pos, 2 * TICKS_PER_QUARTER_NOTE);
  position_set_to_pos (&cur_pos, &start_pos);
  position_add_ticks (&cur_pos, 10);
  position_set_to_pos (&test_pos, &start_pos);
  position_snap (
    &start_pos, &cur_pos, NULL, NULL, SNAP_GRID_TIMELINE);
  char buf1[100];
  char buf2[100];
  position_to_string (&cur_pos, buf1);
  position_to_string (&test_pos, buf2);
  g_message ("cur pos %s test pos %s", buf1, buf2);
  g_assert_cmppos (&cur_pos, &test_pos);

  position_set_to_bar (&start_pos, 4);
  position_add_ticks (&start_pos, 2 * TICKS_PER_QUARTER_NOTE);
  position_set_to_pos (&cur_pos, &start_pos);
  position_add_ticks (&cur_pos, -10);
  position_set_to_pos (&test_pos, &start_pos);
  position_snap (
    &start_pos, &cur_pos, NULL, NULL, SNAP_GRID_TIMELINE);
  position_to_string (&cur_pos, buf1);
  position_to_string (&test_pos, buf2);
  g_message ("cur pos %s test pos %s", buf1, buf2);
  g_assert_cmppos (&cur_pos, &test_pos);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/position/"

  g_test_add_func (
    TEST_PREFIX "test snap", (GTestFunc) test_snap);
  g_test_add_func (
    TEST_PREFIX "test print position",
    (GTestFunc) test_print_position);
  g_test_add_func (
    TEST_PREFIX "test conversions",
    (GTestFunc) test_conversions);
  g_test_add_func (
    TEST_PREFIX "test get totals",
    (GTestFunc) test_get_totals);
  g_test_add_func (
    TEST_PREFIX "test set to", (GTestFunc) test_set_to);
  g_test_add_func (
    TEST_PREFIX "test position from ticks",
    (GTestFunc) test_position_from_ticks);
  g_test_add_func (
    TEST_PREFIX "test position to frames",
    (GTestFunc) test_position_to_frames);
  g_test_add_func (
    TEST_PREFIX "test get total beats",
    (GTestFunc) test_get_total_beats);
  g_test_add_func (
    TEST_PREFIX "test position benchmarks",
    (GTestFunc) test_position_benchmarks);

  return g_test_run ();
}
