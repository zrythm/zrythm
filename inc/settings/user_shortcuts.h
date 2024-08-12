// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * User shortcuts.
 */

#ifndef __SETTINGS_USER_SHORTCUTS_H__
#define __SETTINGS_USER_SHORTCUTS_H__

#include <string>
#include <vector>

/**
 * @addtogroup settings
 *
 * @{
 */

/**
 * Represents a user-defined keyboard shortcut.
 *
 * This struct contains the action name and the primary and secondary
 * keyboard shortcuts associated with that action.
 */
struct UserShortcut
{
  std::string action;
  std::string primary;
  std::string secondary;
};

/**
 * Manages user-defined keyboard shortcuts.
 *
 * This class provides methods to read and retrieve user-defined keyboard
 * shortcuts for various actions in the application.
 *
 * @warning Unimplemented.
 */
class UserShortcuts
{
public:
  /**
   * Returns a shortcut for the given action, or @p
   * default_shortcut if not found.
   *
   * @param primary Whether to get the primary
   *   shortcut, otherwise the secondary.
   */
  const std::string &get (
    bool               primary,
    const std::string &action,
    const std::string &default_shortcut) const;

private:
  std::vector<UserShortcut> shortcuts;
};

/**
 * @}
 */

#endif