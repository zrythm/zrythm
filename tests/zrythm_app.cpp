// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "dsp/midi_event.h"
#include "engine/session/graph_dispatcher.h"
#include "structure/tracks/fader.h"
#include "utils/math.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/zrythm_helper.h"

#include "whereami/whereami.h"

TEST (ZrythmApp, version)
{
  char * exe_path = NULL;
  int    dirname_length, length;
  length = wai_getExecutablePath (nullptr, 0, &dirname_length);
  if (length > 0)
    {
      exe_path = (char *) malloc ((size_t) length + 1);
      wai_getExecutablePath (exe_path, length, &dirname_length);
      exe_path[length] = '\0';
    }
  ASSERT_NONNULL (exe_path);

  const char * arg1 = "--version";
  int          argc = 2;
  char *       argv[] = { exe_path, (char *) arg1 };

  ZrythmApp app (argc, (const char **) argv);
  int       ret = app.run (argc, argv);
  ASSERT_EQ (ret, 0);
}
