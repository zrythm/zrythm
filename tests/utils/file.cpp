// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <cstdlib>

#include "utils/file.h"
#include "utils/flags.h"
#include "utils/io.h"

#include <glib.h>

static void
test_symlink (void)
{
  char * filepath =
    g_build_filename (TESTS_SRCDIR, "test_start_with_signal.mp3", NULL);

  char * tmp_dir = g_dir_make_tmp ("zrythm_symlink_dir_XXXXXX", NULL);
  g_assert_nonnull (tmp_dir);
  char * target = g_build_filename (tmp_dir, "target.mp3", NULL);
  g_message ("target %s", target);

  g_assert_cmpint (file_symlink (filepath, target), ==, 0);
  io_remove (target);
  io_rmdir (tmp_dir, Z_F_NO_FORCE);
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/utils/file/"

  g_test_add_func (TEST_PREFIX "test symlink", (GTestFunc) test_symlink);

  return g_test_run ();
}
