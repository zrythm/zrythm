// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "tests/helpers/project_helper.h"

TEST_SUITE_BEGIN ("dsp/transport");

TEST_CASE_FIXTURE (ZrythmFixture, "load project BPM")
{
  /* save and reload the project */
  test_project_save_and_reload ();
}

TEST_SUITE_END;