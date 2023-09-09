// SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "dsp/curve.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/zrythm.h"

static void
test_curve_algorithms (void)
{
  CurveOptions opts;

  double epsilon = 0.0001;
  double val;

  /* ---- EXPONENT ---- */

  opts.algo = CURVE_ALGORITHM_EXPONENT;

  /* -- curviness -1 -- */
  opts.curviness = -0.95;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1 - 0.93465, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1 - 0.93465, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness -0.5 -- */
  opts.curviness = -0.5;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1 - 0.69496, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1 - 0.69496, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness 0 -- */
  opts.curviness = 0;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.5, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.5, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness 0.5 -- */
  opts.curviness = 0.5;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.69496, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.69496, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness 1 -- */
  opts.curviness = 0.95;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.93465, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.93465, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* ---- SUPERELLIPSE ---- */

  opts.algo = CURVE_ALGORITHM_SUPERELLIPSE;

  /* -- curviness - 0.7 -- */
  opts.curviness = -0.7;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1 - 0.9593, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1 - 0.9593, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness 0 -- */
  opts.curviness = 0;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.5, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.5, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness 0.7 -- */
  opts.curviness = 0.7;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.9593, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.9593, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* ---- VITAL ---- */

  opts.algo = CURVE_ALGORITHM_VITAL;

  /* -- curviness -1 -- */
  opts.curviness = -1;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1 - 0.9933, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1 - 0.9933, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness -0.5 -- */
  opts.curviness = -0.5;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1 - 0.9241, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1 - 0.9241, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness 0 -- */
  opts.curviness = 0;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.5, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.5, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness 0.5 -- */
  opts.curviness = 0.5;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.9241, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.9241, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness 1 -- */
  opts.curviness = 1;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.9933, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.9933, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* ---- PULSE ---- */

  opts.algo = CURVE_ALGORITHM_PULSE;

  /* -- curviness -1 -- */
  opts.curviness = -1;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness -0.5 -- */
  opts.curviness = -0.5;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness 0 -- */
  opts.curviness = 0;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness 0.5 -- */
  opts.curviness = 0.5;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness 1 -- */
  opts.curviness = 1;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* ---- LOGARITHMIC ---- */

  opts.algo = CURVE_ALGORITHM_LOGARITHMIC;

  /* -- curviness -1 -- */
  opts.curviness = -0.95;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1 - 0.968689501, epsilon);

  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1 - 0.968689501, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness -0.5 -- */
  opts.curviness = -0.5;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1 - 0.893168449, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1 - 0.893168449, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness 0 -- */
  opts.curviness = 0;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.511909664, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.511909664, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness 0.5 -- */
  opts.curviness = 0.5;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.893168449, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.893168449, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);

  /* -- curviness 1 -- */
  opts.curviness = 0.95;
  val = curve_get_normalized_y (0.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 0.968689501, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 0);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 1.0, epsilon);
  val = curve_get_normalized_y (0.5, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.968689501, epsilon);
  val = curve_get_normalized_y (1.0, &opts, 1);
  g_assert_cmpfloat_with_epsilon (val, 0.0, epsilon);
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

  test_helper_zrythm_init ();

#define TEST_PREFIX "/audio/curve/"

  g_test_add_func (
    TEST_PREFIX "test_curve_algorithms", (GTestFunc) test_curve_algorithms);

  return g_test_run ();
}
