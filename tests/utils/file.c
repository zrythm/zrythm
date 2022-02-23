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

#include <stdlib.h>

#include "utils/file.h"
#include "utils/flags.h"
#include "utils/io.h"

#include <glib.h>

static void
test_symlink (void)
{
  char * filepath =
    g_build_filename (
      TESTS_SRCDIR,
      "test_start_with_signal.mp3", NULL);

  char * tmp_dir =
    g_dir_make_tmp ("zrythm_symlink_dir_XXXXXX", NULL);
  g_assert_nonnull (tmp_dir);
  char * target =
    g_build_filename (tmp_dir, "target.mp3", NULL);
  g_message ("target %s", target);

  g_assert_cmpint (
    file_symlink (filepath, target), ==, 0);
  io_remove (target);
  io_rmdir (tmp_dir, Z_F_NO_FORCE);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/utils/file/"

  g_test_add_func (
    TEST_PREFIX "test symlink",
    (GTestFunc) test_symlink);

  return g_test_run ();
}

