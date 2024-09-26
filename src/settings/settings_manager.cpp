// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "settings/settings_manager.h"

JUCE_IMPLEMENT_SINGLETON (SettingsManager)

SettingsManager::~SettingsManager ()
{
  clearSingletonInstance ();
}