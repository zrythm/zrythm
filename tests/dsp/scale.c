// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "dsp/scale.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "zrythm.h"

#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

static void
test_scale_contains_note (void)
{
  MusicalScale * scale =
    musical_scale_new (SCALE_MINOR, NOTE_D);

  g_assert_true (musical_scale_contains_note (scale, NOTE_C));
  g_assert_false (
    musical_scale_contains_note (scale, NOTE_CS));
  g_assert_true (musical_scale_contains_note (scale, NOTE_D));
  g_assert_false (
    musical_scale_contains_note (scale, NOTE_DS));
  g_assert_true (musical_scale_contains_note (scale, NOTE_E));
  g_assert_true (musical_scale_contains_note (scale, NOTE_F));
  g_assert_false (
    musical_scale_contains_note (scale, NOTE_FS));
  g_assert_true (musical_scale_contains_note (scale, NOTE_G));
  g_assert_false (
    musical_scale_contains_note (scale, NOTE_GS));
  g_assert_true (musical_scale_contains_note (scale, NOTE_A));
  g_assert_true (musical_scale_contains_note (scale, NOTE_AS));
  g_assert_false (musical_scale_contains_note (scale, NOTE_B));

  musical_scale_free (scale);
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/audio/scale/"

  g_test_add_func (
    TEST_PREFIX "test scale contains note",
    (GTestFunc) test_scale_contains_note);

  return g_test_run ();
}
