/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "tests/helpers/zrythm.h"

#include "guile/guile.h"
#include "guile/project_generator.h"

static void
test_gen_project_from_string ()
{
  /*LOG = log_new ();*/

  ZRYTHM = zrythm_new (NULL, false, true, false);
  guile_init (0, NULL);

  char * tmp_prj_path = g_dir_make_tmp (
    "zrythm_test_project_generator_XXXXXX", NULL);
  char * script_path = g_build_filename (
    TESTS_SRC_ROOT_DIR, "tests", "scripts",
    "gen-test-project.scm", NULL);
  int res =
    guile_project_generator_generate_project_from_file (
      script_path, tmp_prj_path);
  g_assert_cmpint (res, ==, 0);

  char * prj_file = g_build_filename (
    tmp_prj_path, PROJECT_FILE, NULL);
  g_assert_true (
    g_file_test (prj_file, G_FILE_TEST_EXISTS));
  g_free (tmp_prj_path);
  g_free (prj_file);
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/guile/project_generator/"

  g_test_add_func (
    TEST_PREFIX "test gen project from string",
    (GTestFunc) test_gen_project_from_string);

  return g_test_run ();
}
