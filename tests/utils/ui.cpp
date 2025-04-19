// SPDX-FileCopyrightText: © 2020, 2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "gui/backend/ui.h"
#include "utils/gtest_wrapper.h"

TEST (UI, OverlayActionStrings)
{
  /* verify that actions were not added/removed without matching strings */
  ASSERT_EQ (
    UiOverlayAction_to_string (UiOverlayAction::NUM_UI_OVERLAY_ACTIONS),
    "--INVALID--");
}
