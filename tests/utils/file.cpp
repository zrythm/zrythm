// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "tests/helpers/zrythm_helper.h"

#include "common/utils/file.h"
#include "common/utils/io.h"

TEST (File, SymLink)
{
  auto filepath =
    Glib::build_filename (TESTS_SRCDIR, "test_start_with_signal.mp3");

  char * tmp_dir = g_dir_make_tmp ("zrythm_symlink_dir_XXXXXX", nullptr);
  ASSERT_NONNULL (tmp_dir);
  auto target = Glib::build_filename (tmp_dir, "target.mp3");
  z_info ("target {}", target);

  ASSERT_TRUE (file_symlink (filepath.c_str (), target.c_str ()));
  io_remove (target);
  io_rmdir (tmp_dir, false);
}