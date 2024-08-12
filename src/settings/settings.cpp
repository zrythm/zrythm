// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "settings/plugin_settings.h"
#include "settings/settings.h"
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

  plugin_settings_ = PluginSettings::read_or_new ();
  z_return_if_fail (plugin_settings_);
}