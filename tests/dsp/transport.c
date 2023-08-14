// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "dsp/transport.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"

static void
test_load_project_bpm (void)
{
  test_helper_zrythm_init ();

  /* save and reload the project */
  test_project_save_and_reload ();

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/transport/"

  g_test_add_func (
    TEST_PREFIX "test load project bpm",
    (GTestFunc) test_load_project_bpm);

  return g_test_run ();
}
