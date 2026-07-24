// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "gui/backend/backend/settings/plugin_configuration_manager.h"
#include "gui/backend/backend/settings/settings.h"
#include "utils/logger.h"

void
Settings::init ()
{
  plugin_settings_ = PluginConfigurationManager::read_or_new ();
  z_return_if_fail (plugin_settings_);
}
