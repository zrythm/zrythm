// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "settings/g_settings_manager.h"
#include "settings/settings.h"

#include "tests/helpers/zrythm_helper.h"

static void
test_append_to_strv (void)
{
  test_helper_zrythm_init ();

  GSettings settings;
  GSettingsManager::append_to_strv (&settings, "test-key", "test-val", false);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/settings/settings/"

  g_test_add_func (
    TEST_PREFIX "test append to strv", (GTestFunc) test_append_to_strv);

  return g_test_run ();
}
