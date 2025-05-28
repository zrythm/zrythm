// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "gui/backend/backend/settings/plugin_configuration_manager.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/zrythm.h"
#include "utils/gtest_wrapper.h"

void
Settings::init ()
{
  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      return;
    }

  plugin_settings_ = PluginConfigurationManager::read_or_new ();
  z_return_if_fail (plugin_settings_);
}
