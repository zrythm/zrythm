// SPDX-FileCopyrightText: © 2020, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "utils/gtest_wrapper.h"
#include "utils/ui.h"

TEST_SUITE_BEGIN ("utils/ui");

TEST_CASE ("overlay action strings")
{
  /* verify that actions were not added/removed without matching strings */
  ASSERT_EQ (
    UiOverlayAction_to_string (UiOverlayAction::NUM_UI_OVERLAY_ACTIONS),
    "--INVALID--");
}

TEST_SUITE_END;