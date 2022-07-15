// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <math.h>

#include "audio/audio_track.h"
#include "audio/engine_dummy.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/zrythm.h"

#include <locale.h>

static void
test_update_snap_points (void)
{
  test_helper_zrythm_init ();

  SnapGrid sg;
  gint64   before, after;

  snap_grid_init (
    &sg, SNAP_GRID_TYPE_TIMELINE, NOTE_LENGTH_1_128, false);

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
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/snap grid/"

  g_test_add_func (
    TEST_PREFIX "test update snap points",
    (GTestFunc) test_update_snap_points);

  return g_test_run ();
}
