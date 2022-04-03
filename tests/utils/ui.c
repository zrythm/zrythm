/*
 * Copyright (C) 2020, 2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "utils/ui.h"

#include <glib.h>

static void
test_overlay_action_strings (void)
{
  /* verify that actions were not added/removed
   * without matching strings */
  g_assert_cmpstr (
    ui_overlay_strings[NUM_UI_OVERLAY_ACTIONS], ==,
    "INVALID");
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
