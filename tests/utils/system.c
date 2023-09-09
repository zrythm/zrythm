// SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <stdlib.h>

#include "utils/system.h"

#include <glib.h>

static void
test_run_cmd_w_args (void)
{
  char * output;
  int    ret;

  const char * args[] = {
    "bash",
    "-c",
    "sleep 6 && echo hello",
    NULL,
  };
  const char * args_instant[] = {
    "bash",
    "-c",
    "echo hello",
    NULL,
  };
  const char * args_stderr[] = {
    "bash",
    "-c",
    "sleep 6 && >&2 echo hello",
    NULL,
  };

  /* try no output */
  ret = system_run_cmd_w_args (args_instant, 100, NULL, NULL, false);
  g_assert_cmpint (ret, ==, 0);

  ret = system_run_cmd_w_args (args, 1, &output, NULL, false);
  g_assert_cmpint (ret, !=, 0);
  ret = system_run_cmd_w_args (args, 1, NULL, &output, false);
  g_assert_cmpint (ret, !=, 0);
  ret = system_run_cmd_w_args (args_stderr, 1, &output, NULL, false);
  g_assert_cmpint (ret, !=, 0);
  ret = system_run_cmd_w_args (args_stderr, 1, NULL, &output, false);
  g_assert_cmpint (ret, !=, 0);

  ret = system_run_cmd_w_args (args, 2000, &output, NULL, false);
  g_assert_cmpint (ret, !=, 0);
  ret = system_run_cmd_w_args (args, 2000, NULL, &output, false);
  g_assert_cmpint (ret, !=, 0);
  ret = system_run_cmd_w_args (args_stderr, 2000, &output, NULL, false);
  g_assert_cmpint (ret, !=, 0);
  ret = system_run_cmd_w_args (args_stderr, 2000, NULL, &output, false);
  g_assert_cmpint (ret, !=, 0);

  ret = system_run_cmd_w_args (args, 8000, &output, NULL, false);
  g_assert_cmpint (ret, ==, 0);
  g_assert_cmpstr (output, ==, "hello\n");
  output[0] = '\0';
#if 0
  ret =
    system_run_cmd_w_args (
      args, 8000, false, output, false);
  g_assert_cmpint (ret, ==, 0);
  g_assert_cmpstr (output, ==, "");
  output[0] = '\0';

  ret =
    system_run_cmd_w_args (
      args_stderr, 8000, true, output, false);
  g_assert_cmpint (ret, ==, 0);
  g_assert_cmpstr (output, ==, "");
  output[0] = '\0';
  ret =
    system_run_cmd_w_args (
      args_stderr, 8000, false, output, false);
  g_assert_cmpint (ret, ==, 0);
  g_assert_cmpstr (output, ==, "hello\n");
#endif
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/utils/general/"

  g_test_add_func (
    TEST_PREFIX "test run cmd w args", (GTestFunc) test_run_cmd_w_args);

  return g_test_run ();
}
