// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Application settings.
 */

#include "zrythm-config.h"

#include <cstdio>
#include <cstdlib>

#include "settings/g_settings_manager.h"
#include "settings/plugin_settings.h"
#include "settings/settings.h"
#include "settings/user_shortcuts.h"
#include "utils/gtk.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/terminal.h"
#include "zrythm.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

void
Settings::init ()
{
  if (ZRYTHM_TESTING)
    {
      return;
    }

  this->plugin_settings = plugin_settings_read_or_new ();
  g_return_if_fail (this->plugin_settings);

  this->user_shortcuts = user_shortcuts_new ();
  if (!this->user_shortcuts)
    {
      g_warning ("failed to parse user shortcuts");
    }
}

/**
 * Frees settings.
 */
Settings::~Settings ()
{
  object_free_w_func_and_null (plugin_settings_free, this->plugin_settings);
  object_free_w_func_and_null (user_shortcuts_free, this->user_shortcuts);
}
