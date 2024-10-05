// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "gui/cpp/backend/settings/plugin_settings.h"
#include "gui/cpp/backend/settings/settings.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include <glib/gi18n.h>

#include "common/utils/gtest_wrapper.h"

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

void
Settings::init ()
{
  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      return;
    }

  plugin_settings_ = PluginSettings::read_or_new ();
  z_return_if_fail (plugin_settings_);
}