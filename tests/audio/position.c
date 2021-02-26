/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-test-config.h"

#include "audio/position.h"
#include "utils/math.h"

#include "tests/helpers/zrythm.h"

static void
test_position_from_ticks ()
{
  Position pos = {
    4, 4, 4, 4, 4, 4
  };
  double ticks = 50000.0;

  /* assert values are correct */
  position_from_ticks (&pos, ticks);
  g_assert_cmpint (
    pos.bars, ==,
    math_round_double_to_int (
      ticks / TRANSPORT->ticks_per_bar + 1));
  g_assert_cmpint (
    pos.bars, >, 0);
}

static void
test_get_total_beats ()
{
  Position start_pos = {
    .bars = 2,
    .beats = 1,
    .sixteenths = 4,
    .ticks = 222,
    .sub_tick = 0.38185941043198568,
    .total_ticks = 4782.3818594104323,
    .frames = 69376
  };
  Position end_pos = {
    .bars = 2,
    .beats = 2,
    .sixteenths = 1,
    .ticks = 0,
    .sub_tick = 0.029024943311810603,
    .total_ticks = 4800.0290249433119,
    .frames = 69632
  };
  position_from_ticks (
    &start_pos, 4782.3818594104323);
  position_from_ticks (
    &end_pos, 4800);

  int beats = 0;
  beats =
    position_get_total_beats (&start_pos);
  g_assert_cmpint (beats, ==, 4);
  beats =
    position_get_total_beats (&end_pos);
  g_assert_cmpint (beats, ==, 4);

  position_from_ticks (
    &end_pos, 4800.0290249433119);
  beats =
    position_get_total_beats (&end_pos);
  g_assert_cmpint (beats, ==, 5);
}

static void
test_position_benchmarks ()
{
  double ticks = 50000.0;
  Position pos;
  position_from_ticks (&pos, ticks);

  g_message ("position to frames:");
  for (int j = 0; j < 5; j++)
    {
      gint64 before = g_get_monotonic_time ();
      for (int i = 0; i < 100000; i++)
        {
          position_to_frames (&pos);
        }
      gint64 after = g_get_monotonic_time ();
      g_message ("time: %ld", after - before);
    }

  g_message ("add frames");
  for (int j = 0; j < 5; j++)
    {
      gint64 before = g_get_monotonic_time ();
      for (int i = 0; i < 100000; i++)
        {
          position_add_frames (&pos, 1000);
        }
      gint64 after = g_get_monotonic_time ();
      g_message ("time: %ld", after - before);
    }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/audio/position/"

  g_test_add_func (
    TEST_PREFIX "test position from ticks",
    (GTestFunc) test_position_from_ticks);
  g_test_add_func (
    TEST_PREFIX "test get total beats",
    (GTestFunc) test_get_total_beats);
  g_test_add_func (
    TEST_PREFIX "test position benchmarks",
    (GTestFunc) test_position_benchmarks);

  return g_test_run ();
}
