// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "dsp/transport.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"

static void
test_load_project_bpm (void)
{
  test_helper_zrythm_init ();

  Position pos;
  position_init (&pos);
  bpm_t bpm_before = tempo_track_get_bpm_at_pos (P_TEMPO_TRACK, &pos);
  tempo_track_set_bpm (
    P_TEMPO_TRACK, bpm_before + 20.f, bpm_before, false, F_NO_PUBLISH_EVENTS);

  /* save and reload the project */
  test_project_save_and_reload ();

  bpm_t bpm_after = tempo_track_get_bpm_at_pos (P_TEMPO_TRACK, &pos);
  g_assert_cmpfloat_with_epsilon (bpm_after, bpm_before + 20.f, 0.0001f);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/transport/"

  g_test_add_func (
    TEST_PREFIX "test load project bpm", (GTestFunc) test_load_project_bpm);

  return g_test_run ();
}
