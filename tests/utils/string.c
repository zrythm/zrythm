/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "utils/string.h"

#include <glib.h>

static void
test_get_int_after_last_space ()
{
  const char * strs[] = {
    "helloЧитатьハロー・ワールド 1",
    "testハロー・ワールドest 22",
    "testハロー22",
    "",
    "testハロー 34 56",
  };

  int ret =
    string_get_int_after_last_space (
      strs[0], NULL);
  g_assert_cmpint (ret, ==, 1);
  ret =
    string_get_int_after_last_space (
      strs[1], NULL);
  g_assert_cmpint (ret, ==, 22);
  ret =
    string_get_int_after_last_space (
      strs[2], NULL);
  g_assert_cmpint (ret, ==, -1);
  ret =
    string_get_int_after_last_space (
      strs[4], NULL);
  g_assert_cmpint (ret, ==, 56);
}

static void
test_contains_substr ()
{
  const char * strs[] = {
    "helloЧитатьハロー・ワールド 1",
    "testハロー・ワールドABc 22",
    "testハロー22",
    "",
    "testハロー 34 56",
  };

  g_assert_true (
    string_contains_substr (strs[0], "Читать"));
  g_assert_true (
    string_contains_substr_case_insensitive (
      strs[1], "abc"));
  g_assert_false (
    string_contains_substr (strs[0], "Чатитать"));
  g_assert_false (
    string_contains_substr_case_insensitive (
      strs[1], "abd"));
}

static void
test_is_equal ()
{
  g_assert_true (
    string_is_equal ("", ""));
  g_assert_true (
    string_is_equal (NULL, NULL));
  g_assert_true (
    string_is_equal ("abc", "abc"));
  g_assert_false (
    string_is_equal ("abc", "aabc"));
  g_assert_false (
    string_is_equal ("", "aabc"));
  g_assert_false (
    string_is_equal (NULL, ""));
}

static void
test_copy_w_realloc ()
{
  char * str = g_strdup ("aa");
  string_copy_w_realloc (&str, "");
  g_assert_true (
    string_is_equal (str, ""));
  string_copy_w_realloc (&str, "aa");
  g_assert_true (
    string_is_equal (str, "aa"));
  string_copy_w_realloc (&str, "あああ");
  g_assert_true (
    string_is_equal (str, "あああ"));
  string_copy_w_realloc (&str, "あああ");
  g_assert_true (
    string_is_equal (str, "あああ"));
  string_copy_w_realloc (&str, "aa");
  g_assert_true (
    string_is_equal (str, "aa"));
  string_copy_w_realloc (&str, "");
  g_assert_nonnull (str);
  g_assert_true (
    string_is_equal (str, ""));

  string_copy_w_realloc (&str, NULL);
  g_assert_null (str);
  string_copy_w_realloc (&str, NULL);
  g_assert_null (str);
  string_copy_w_realloc (&str, "");
  g_assert_nonnull (str);
  g_assert_true (
    string_is_equal (str, ""));
  string_copy_w_realloc (&str, "aaa");
  g_assert_true (
    string_is_equal (str, "aaa"));
  string_copy_w_realloc (&str, "aaa");
  g_assert_true (
    string_is_equal (str, "aaa"));
  string_copy_w_realloc (&str, "aa");
  g_assert_true (
    string_is_equal (str, "aa"));

  string_copy_w_realloc (
    &str,
    "[Zrythm] Enabled");
  g_assert_true (
    string_is_equal (str, "[Zrythm] Enabled"));
}

static void
test_replace_regex (void)
{
  const char * replace_str, * regex;
  char * src_str;

  replace_str = "---$1---";
  regex = "(abc)+\\1";
  src_str = g_strdup ("abcabc");
  string_replace_regex (
    &src_str, regex, replace_str);
  g_assert_cmpstr (
    src_str, ==, "---abc---");

  replace_str = "$1";
  regex = "(\\?\\?\\?\n)+\\1";
  src_str =
    g_strdup ("???\n???\n???\n???\n??? abc");
  string_replace_regex (
    &src_str, regex, replace_str);
  g_assert_cmpstr (
    src_str, ==, "???\n??? abc");

  replace_str = "??? ...\n";
  regex = "(\\?\\?\\?\n)+\\1";
  src_str =
    g_strdup ("???\n???\n???\n???\n??? abc\n???\n???\n??? test");
  string_replace_regex (
    &src_str, regex, replace_str);
  g_assert_cmpstr (
    src_str, ==, "??? ...\n??? abc\n??? ...\n??? test");
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/utils/string/"

  g_test_add_func (
    TEST_PREFIX "test replace regex",
    (GTestFunc) test_replace_regex);
  g_test_add_func (
    TEST_PREFIX "test get int after last space",
    (GTestFunc) test_get_int_after_last_space);
  g_test_add_func (
    TEST_PREFIX "test contains substr",
    (GTestFunc) test_contains_substr);
  g_test_add_func (
    TEST_PREFIX "test is equal",
    (GTestFunc) test_is_equal);
  g_test_add_func (
    TEST_PREFIX "test copy w realloc",
    (GTestFunc) test_copy_w_realloc);

  return g_test_run ();
}
