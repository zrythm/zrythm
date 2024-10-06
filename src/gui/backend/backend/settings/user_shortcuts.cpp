// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/settings/user_shortcuts.h"

const std::string &
UserShortcuts::get (
  bool               primary,
  const std::string &action,
  const std::string &default_shortcut) const
{
  for (const auto &shortcut : shortcuts)
    {
      if (shortcut.action == action)
        {
          return primary ? shortcut.primary : shortcut.secondary;
        }
    }

  return default_shortcut;
}