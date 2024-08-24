// SPDX-FileCopyrightText: © 2021-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dsp/tempo_track.h"
#include "dsp/track.h"
#include "project.h"
#include "utils/flags.h"
#include "zrythm.h"

#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("zrythm");

TEST_CASE_FIXTURE (ZrythmFixture, "dummy") { }

TEST_CASE ("fetch latest release")
{
  /* TODO below is async now */
#if 0
#  ifdef CHECK_UPDATES
  char * ver = zrythm_fetch_latest_release_ver ();
  REQUIRE_NONNULL (ver);
  REQUIRE_LT (strlen (ver), 20);
#  endif
#endif
}

TEST_SUITE_END;