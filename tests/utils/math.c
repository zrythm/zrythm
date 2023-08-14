// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <stdlib.h>

#include "utils/math.h"

#include <glib.h>

static void
test_nans_in_conversions (void)
{
  float ret;
  ret = math_get_fader_val_from_amp (1);
  math_assert_nonnann (ret);
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/utils/math/"

  g_test_add_func (
    TEST_PREFIX "test nans in conversions",
    (GTestFunc) test_nans_in_conversions);

  return g_test_run ();
}
