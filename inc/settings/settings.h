// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Zrythm settings.
 */

#ifndef __SETTINGS_SETTINGS_H__
#define __SETTINGS_SETTINGS_H__

#include "gtk_wrapper.h"

typedef struct PluginSettings PluginSettings;
typedef struct UserShortcuts  UserShortcuts;

/**
 * @addtogroup project Settings
 *
 * @{
 */

#define SETTINGS (gZrythm->settings)

#define S_PLUGIN_SETTINGS (SETTINGS->plugin_settings)

#define S_USER_SHORTCUTS (SETTINGS->user_shortcuts)

class Settings
{
public:
  Settings (){};
  ~Settings ();

  /**
   * Initializes settings.
   */
  void init ();

  PluginSettings * plugin_settings = nullptr;

  UserShortcuts * user_shortcuts = nullptr;
};

/**
 * @}
 */

#endif
