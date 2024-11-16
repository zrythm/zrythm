// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Zrythm settings.
 */

#ifndef __SETTINGS_SETTINGS_H__
#define __SETTINGS_SETTINGS_H__

#include <memory>

#include "gui/backend/backend/settings/user_shortcuts.h"

class PluginSettings;

/**
 * @addtogroup project Settings
 *
 * @{
 */

#define SETTINGS (gZrythm->settings_)

#define S_PLUGIN_SETTINGS (SETTINGS->plugin_settings_)

#define S_USER_SHORTCUTS (SETTINGS->user_shortcuts_)

class Settings
{
public:
  /**
   * Initializes settings.
   */
  void init ();

  std::unique_ptr<PluginSettings> plugin_settings_;

  UserShortcuts user_shortcuts_;
};

/**
 * @}
 */

#endif
