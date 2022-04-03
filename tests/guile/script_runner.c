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
  bool   ret = g_file_get_contents (
      args[1], &content, NULL, NULL);
  g_assert_true (ret);
  char * res = guile_run_script (content);
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

  g_test_add_func (
    TEST_PREFIX "test run script",
    (GTestFunc) test_run_script);

  return g_test_run ();
}
