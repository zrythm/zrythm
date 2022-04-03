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

#include "audio/port_identifier.h"
#include "utils/general.h"

#include <glib.h>

static void
test_uint_from_bitfield (void)
{
  g_assert_cmpuint (
    0, ==,
    utils_get_uint_from_bitfield_val (
      PORT_FLAG_STEREO_L));
  g_assert_cmpuint (
    14, ==,
    utils_get_uint_from_bitfield_val (
      PORT_FLAG_NOT_ON_GUI));
  g_assert_cmpuint (
    18, ==,
    utils_get_uint_from_bitfield_val (
      PORT_FLAG_CHANNEL_FADER));
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/utils/general/"

  g_test_add_func (
    TEST_PREFIX "test uint from bitfield",
    (GTestFunc) test_uint_from_bitfield);

  return g_test_run ();
}
