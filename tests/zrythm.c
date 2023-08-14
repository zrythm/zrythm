// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "audio/tempo_track.h"
#include "audio/track.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "helpers/zrythm.h"

#include <locale.h>

static void
test_dummy (void)
{
  test_helper_zrythm_init ();
  test_helper_zrythm_cleanup ();
}

static void
test_fetch_latest_release (void)
{
#ifdef CHECK_UPDATES
  char * ver = zrythm_fetch_latest_release_ver ();
  g_assert_nonnull (ver);
  g_assert_cmpuint (strlen (ver), <, 20);
#endif
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/zrythm/"

  g_test_add_func (
    TEST_PREFIX "test dummy", (GTestFunc) test_dummy);
  g_test_add_func (
    TEST_PREFIX "test fetch latest release",
    (GTestFunc) test_fetch_latest_release);

  return g_test_run ();
}
