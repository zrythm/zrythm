// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include "tests/helpers/zrythm_helper.h"

TEST_F (ZrythmFixture, UpdateSnapPoints)
{
  SnapGrid sg (SnapGrid::Type::Timeline, NoteLength::NOTE_LENGTH_1_128, false);

  auto TEST_WITH_MAX_BARS = [] (auto x) {
    auto before = Zrythm::getInstance ()->get_monotonic_time_usecs ();
    auto after = Zrythm::getInstance ()->get_monotonic_time_usecs ();
    z_info ("time {}", after - before);
  };

  // ????
  TEST_WITH_MAX_BARS (100);
  TEST_WITH_MAX_BARS (1000);
#if 0
  TEST_WITH_MAX_BARS (10000);
  TEST_WITH_MAX_BARS (100000);
#endif
}