// SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
    utils_get_uint_from_bitfield_val (PORT_FLAG_STEREO_L));
  g_assert_cmpuint (
    14, ==,
    utils_get_uint_from_bitfield_val (PORT_FLAG_NOT_ON_GUI));
  g_assert_cmpuint (
    18, ==,
    utils_get_uint_from_bitfield_val (PORT_FLAG_CHANNEL_FADER));
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
