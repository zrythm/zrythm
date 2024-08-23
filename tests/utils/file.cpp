// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "utils/file.h"
#include "utils/io.h"

#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("utils/file");

TEST_CASE ("symlink")
{
  auto filepath =
    Glib::build_filename (TESTS_SRCDIR, "test_start_with_signal.mp3");

  char * tmp_dir = g_dir_make_tmp ("zrythm_symlink_dir_XXXXXX", nullptr);
  REQUIRE_NONNULL (tmp_dir);
  auto target = Glib::build_filename (tmp_dir, "target.mp3");
  z_info ("target {}", target);

  REQUIRE_EQ (file_symlink (filepath.c_str (), target.c_str ()), 0);
  io_remove (target);
  io_rmdir (tmp_dir, false);
}

TEST_SUITE_END;