/*
 * Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include <math.h>

#include "audio/engine_dummy.h"
#include "audio/audio_track.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/zrythm.h"

#include <glib.h>
#include <locale.h>

static void
test_update_snap_points (void)
{
  test_helper_zrythm_init ();

  SnapGrid sg;
  gint64 before, after;

  snap_grid_init (
    &sg, SNAP_GRID_TYPE_TIMELINE,
    NOTE_LENGTH_1_128, false);

#define TEST_WITH_MAX_BARS(x) \
  before = g_get_monotonic_time (); \
  after = g_get_monotonic_time (); \
  g_message ("time %ld", after - before)

  TEST_WITH_MAX_BARS (100);
  TEST_WITH_MAX_BARS (1000);
#if 0
  TEST_WITH_MAX_BARS (10000);
  TEST_WITH_MAX_BARS (100000);
#endif

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/snap grid/"

  g_test_add_func (
    TEST_PREFIX "test update snap points",
    (GTestFunc) test_update_snap_points);

  return g_test_run ();
}
