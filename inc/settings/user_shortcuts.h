/*
 * SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * User shortcuts.
 */

#ifndef __SETTINGS_USER_SHORTCUTS_H__
#define __SETTINGS_USER_SHORTCUTS_H__

#include "utils/yaml.h"

/**
 * @addtogroup settings
 *
 * @{
 */

#define USER_SHORTCUTS_SCHEMA_VERSION 2

typedef struct UserShortcut
{
  char * action;
  char * primary;
  char * secondary;
} UserShortcut;

/**
 * User shortcuts read from yaml.
 */
typedef struct UserShortcuts
{
  /** Valid descriptors. */
  UserShortcut * shortcuts[900];
  int            num_shortcuts;
} UserShortcuts;

void
user_shortcut_free (UserShortcut * shortcut);

/**
 * Reads the file and fills up the object.
 */
UserShortcuts *
user_shortcuts_new (void);

/**
 * Returns a shortcut for the given action, or @p
 * default_shortcut if not found.
 *
 * @param primary Whether to get the primary
 *   shortcut, otherwise the secondary.
 */
const char *
user_shortcuts_get (
  UserShortcuts * self,
  bool            primary,
  const char *    action,
  const char *    default_shortcut);

void
user_shortcuts_free (UserShortcuts * self);

/**
 * @}
 */

#endif
