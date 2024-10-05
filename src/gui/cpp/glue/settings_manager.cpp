// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/glue/settings_manager.h"
#include "gui/cpp/zrythm_application.h"

SettingsManager::SettingsManager (QObject * parent) : QObject (parent) { }

void
SettingsManager::reset_and_sync ()
{
  settings_.clear ();
  settings_.sync ();
}

SettingsManager *
SettingsManager::get_instance ()
{
  return dynamic_cast<ZrythmApplication *> (qApp)->get_settings_manager ();
}