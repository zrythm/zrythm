/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "utils/io.h"

#include <glib.h>

static void
test_get_parent_dir ()
{
  char * parent;

#ifdef _WOE32
  parent =
    io_path_get_parent_dir (
      "C:\\ab\\cd\\ef\\gh");
  g_assert_cmpstr (parent, ==, "C:\\ab\\cd\\ef");
  parent =
    io_path_get_parent_dir (
      "C:\\ab\\cd\\ef\\gh\\");
  g_assert_cmpstr (parent, ==, "C:\\ab\\cd\\ef");
  parent =
    io_path_get_parent_dir (
      "C:\\ab");
  g_assert_cmpstr (parent, ==, "C:\\");
  parent =
    io_path_get_parent_dir (
      "C:\\");
  g_assert_cmpstr (parent, ==, "C:\\");
#else
  parent =
    io_path_get_parent_dir (
      "/ab/cd/ef/gh");
  g_assert_cmpstr (parent, ==, "/ab/cd/ef");
  parent =
    io_path_get_parent_dir (
      "/ab/cd/ef/gh/");
  g_assert_cmpstr (parent, ==, "/ab/cd/ef");
  parent =
    io_path_get_parent_dir (
      "/ab");
  g_assert_cmpstr (parent, ==, "/");
  parent =
    io_path_get_parent_dir (
      "/");
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

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/utils/io/"

  g_test_add_func (
    TEST_PREFIX "test get parent dir",
    (GTestFunc) test_get_parent_dir);
  g_test_add_func (
    TEST_PREFIX "test get ext",
    (GTestFunc) test_get_ext);

  return g_test_run ();
}

