// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/zrythm_application.h"

using namespace zrythm::gui;

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