// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <math.h>

#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

#include "guile/guile.h"

int     num_args = 0;
char ** args = NULL;

static void
test_run_script (void)
{
  test_helper_zrythm_init ();

  guile_init (0, NULL);

  g_message ("running <%s>", args[1]);
  char * content = NULL;
  bool   ret = g_file_get_contents (args[1], &content, NULL, NULL);
  g_assert_true (ret);
  char * res = guile_run_script (content, GUILE_SCRIPT_LANGUAGE_SCHEME);
  g_message ("%s", res);
  g_assert_true (guile_script_succeeded (res));

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

  num_args = argc;
  args = argv;

#define TEST_PREFIX "/guile/script_runner/"

  g_test_add_func (TEST_PREFIX "test run script", (GTestFunc) test_run_script);

  return g_test_run ();
}
