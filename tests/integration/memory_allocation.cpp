/*
 * SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "zrythm-test-config.h"

#include "dsp/engine_dummy.h"
#include "dsp/midi_event.h"
#include "dsp/midi_track.h"
#include "dsp/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "zrythm.h"

#include <glib.h>

#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

#include <locale.h>

/**
 * Verify that memory allocated by init() is free'd
 * by cleanup().
 */
static void
test_memory_allocation (void)
{
  test_helper_zrythm_init ();

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/integration/memory_allocation/"

  g_test_add_func (
    TEST_PREFIX "test memory allocation", (GTestFunc) test_memory_allocation);

  return g_test_run ();
}
