// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/backend/settings/settings.h"

#include "tests/helpers/zrythm_helper.h"

TEST_F (ZrythmFixture, AppendToStrv)
{
  GSettings settings;
  GSettingsManager::append_to_strv (&settings, "test-key", "test-val", false);
}