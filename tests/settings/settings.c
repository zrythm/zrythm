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

#include "settings/settings.h"

#include "tests/helpers/zrythm.h"

static void
test_append_to_strv (void)
{
  test_helper_zrythm_init ();

  GSettings settings;
  settings_append_to_strv (
    &settings, "test-key", "test-val", false);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/settings/settings/"

  g_test_add_func (
    TEST_PREFIX "test append to strv",
    (GTestFunc) test_append_to_strv);

  return g_test_run ();
}
