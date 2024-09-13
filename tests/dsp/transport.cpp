// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "tests/helpers/project_helper.h"

TEST_F (ZrythmFixture, LoadProjectBPM)
{
  /* save and reload the project */
  test_project_save_and_reload ();
}