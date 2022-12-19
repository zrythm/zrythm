// SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <stdlib.h>

#include "utils/ui.h"

#include <glib.h>

static void
test_overlay_action_strings (void)
{
  /* verify that actions were not added/removed
   * without matching strings */
  g_assert_cmpstr (
    ui_overlay_strings[NUM_UI_OVERLAY_ACTIONS], ==,
    "--INVALID--");
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/utils/ui/"

  g_test_add_func (
    TEST_PREFIX "test overlay action strings",
    (GTestFunc) test_overlay_action_strings);

  return g_test_run ();
}
