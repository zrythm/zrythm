// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "dsp/fader.h"
#include "dsp/midi_event.h"
#include "dsp/router.h"
#include "utils/math.h"

#include <glib.h>

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/zrythm.h"

#include "ext/whereami/whereami.h"

static void
test_version (void)
{
  test_helper_zrythm_init ();

  char * exe_path = NULL;
  int    dirname_length, length;
  length = wai_getExecutablePath (NULL, 0, &dirname_length);
  if (length > 0)
    {
      exe_path = (char *) malloc ((size_t) length + 1);
      wai_getExecutablePath (exe_path, length, &dirname_length);
      exe_path[length] = '\0';
    }
  g_assert_nonnull (exe_path);

  const char * arg1 = "--version";
  int          argc = 2;
  char *       argv[] = { exe_path, (char *) arg1 };

  ZrythmApp * app = zrythm_app_new (argc, (const char **) argv);
  int         ret = g_application_run (G_APPLICATION (app), argc, argv);
  g_assert_cmpint (ret, ==, 0);
  g_object_unref (app);

  test_helper_zrythm_cleanup ();
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/zrythm_app/"

  g_test_add_func (TEST_PREFIX "test version", (GTestFunc) test_version);

  return g_test_run ();
}
