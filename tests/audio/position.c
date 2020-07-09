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

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/audio/position/"

  g_test_add_func (
    TEST_PREFIX "test position from ticks",
    (GTestFunc) test_position_from_ticks);

  return g_test_run ();
}
