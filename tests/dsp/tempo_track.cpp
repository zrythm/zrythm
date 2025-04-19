// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/transport.h"

#include "tests/helpers/project_helper.h"

TEST_F (ZrythmFixture, LoadProjectBpm)
{
  Position pos;
  bpm_t    bpm_before = P_TEMPO_TRACK->get_bpm_at_pos (pos);
  P_TEMPO_TRACK->set_bpm (bpm_before + 20.f, bpm_before, false, false);

  /* save and reload the project */
  test_project_save_and_reload ();

  bpm_t bpm_after = P_TEMPO_TRACK->get_bpm_at_pos (pos);
  ASSERT_NEAR (bpm_after, bpm_before + 20.f, 0.0001f);
}
