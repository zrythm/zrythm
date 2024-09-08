// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "dsp/transport.h"
#include "project.h"
#include "zrythm.h"

#include "tests/helpers/project_helper.h"

TEST_SUITE_BEGIN ("dsp/tempo track");

TEST_CASE_FIXTURE (ZrythmFixture, "load project bpm")
{
  Position pos;
  bpm_t    bpm_before = P_TEMPO_TRACK->get_bpm_at_pos (pos);
  P_TEMPO_TRACK->set_bpm (bpm_before + 20.f, bpm_before, false, false);

  /* save and reload the project */
  test_project_save_and_reload ();

  bpm_t bpm_after = P_TEMPO_TRACK->get_bpm_at_pos (pos);
  ASSERT_NEAR (bpm_after, bpm_before + 20.f, 0.0001f);
}

TEST_SUITE_END;