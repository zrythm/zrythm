// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "settings/g_settings_manager.h"
#include "settings/settings.h"

#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("settings/settings");

TEST_CASE_FIXTURE (ZrythmFixture, "append to strv")
{
  GSettings settings;
  GSettingsManager::append_to_strv (&settings, "test-key", "test-val", false);
}

TEST_SUITE_END;