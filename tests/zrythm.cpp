// SPDX-FileCopyrightText: © 2021-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "common/dsp/tempo_track.h"
#include "common/dsp/track.h"
#include "common/utils/flags.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/zrythm.h"

#include "tests/helpers/zrythm_helper.h"

TEST_F (ZrythmFixture, Dummy) { }

TEST (ZrythmTest, FetchLatestRelease)
{
  /* TODO below is async now */
#if 0
#  ifdef CHECK_UPDATES
  char * ver = zrythm_fetch_latest_release_ver ();
  ASSERT_NONNULL (ver);
  ASSERT_LT (strlen (ver), 20);
#  endif
#endif
}