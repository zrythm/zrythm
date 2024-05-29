// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <cstdlib>

#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/io.h"

#include <glib.h>

#include "tests/helpers/zrythm_helper.h"

static void
test_get_parent_dir (void)
{
  char * parent;

#ifdef _WIN32
  parent = io_path_get_parent_dir ("C:\\ab\\cd\\ef\\gh");
  g_assert_cmpstr (parent, ==, "C:\\ab\\cd\\ef");
  parent = io_path_get_parent_dir ("C:\\ab\\cd\\ef\\gh\\");
  g_assert_cmpstr (parent, ==, "C:\\ab\\cd\\ef");
  parent = io_path_get_parent_dir ("C:\\ab");
  g_assert_cmpstr (parent, ==, "C:\\");
  parent = io_path_get_parent_dir ("C:\\");
  g_assert_cmpstr (parent, ==, "C:\\");
#else
  parent = io_path_get_parent_dir ("/ab/cd/ef/gh");
  g_assert_cmpstr (parent, ==, "/ab/cd/ef");
  parent = io_path_get_parent_dir ("/ab/cd/ef/gh/");
  g_assert_cmpstr (parent, ==, "/ab/cd/ef");
  parent = io_path_get_parent_dir ("/ab");
  g_assert_cmpstr (parent, ==, "/");
  parent = io_path_get_parent_dir ("/");
  g_assert_cmpstr (parent, ==, "/");
#endif
}

static void
test_get_ext (void)
{
  const char * res;

  res = io_file_get_ext ("abc.wav");
  g_assert_cmpstr (res, ==, "wav");
  res = io_file_get_ext ("abc.test.wav");
  g_assert_cmpstr (res, ==, "wav");
  res = io_file_get_ext ("abctestwav");
  g_assert_cmpstr (res, ==, "");
  res = io_file_get_ext ("abctestwav.");
  g_assert_cmpstr (res, ==, "");
  res = io_file_get_ext ("...");
  g_assert_cmpstr (res, ==, "");
}

static void
test_strip_ext (void)
{
  const char * res;

  res = io_file_strip_ext ("abc.wav");
  g_assert_cmpstr (res, ==, "abc");
  res = io_file_strip_ext ("abc.test.wav");
  g_assert_cmpstr (res, ==, "abc.test");
  res = io_file_strip_ext ("abctestwav");
  g_assert_cmpstr (res, ==, "abctestwav");
  res = io_file_strip_ext ("abctestwav.");
  g_assert_cmpstr (res, ==, "abctestwav");
  res = io_file_strip_ext ("...");
  g_assert_cmpstr (res, ==, "..");
}

static void
test_get_files_in_dir (void)
{
  test_helper_zrythm_init ();

#ifdef __linux__
  char ** files;

  files =
    io_get_files_in_dir_ending_in (TESTS_SRCDIR, F_NO_RECURSIVE, ".wav", false);
  g_assert_nonnull (files);
  g_strfreev (files);

  files = NULL;

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*reached*");
  files =
    io_get_files_in_dir_ending_in ("/non-existent", F_RECURSIVE, ".wav", false);
  g_test_assert_expected_messages ();
  g_assert_null (files);

  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*reached*");
  files =
    io_get_files_in_dir_ending_in ("/non-existent", F_RECURSIVE, ".wav", true);
  g_test_assert_expected_messages ();
  g_assert_nonnull (files);
  g_assert_null (files[0]);
  g_strfreev (files);
#endif

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/utils/io/"

  g_test_add_func (
    TEST_PREFIX "test get parent dir", (GTestFunc) test_get_parent_dir);
  g_test_add_func (TEST_PREFIX "test get ext", (GTestFunc) test_get_ext);
  g_test_add_func (
    TEST_PREFIX "test get files in dir", (GTestFunc) test_get_files_in_dir);
  g_test_add_func (TEST_PREFIX "test strip ext", (GTestFunc) test_strip_ext);

  return g_test_run ();
}
