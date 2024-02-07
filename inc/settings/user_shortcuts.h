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

#include <stdbool.h>

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
  /** Version of the file. */
  int schema_version;

  /** Valid descriptors. */
  UserShortcut * shortcuts[900];
  int            num_shortcuts;
} UserShortcuts;

static const cyaml_schema_field_t user_shortcut_fields_schema[] = {
  YAML_FIELD_STRING_PTR (UserShortcut, action),
  YAML_FIELD_STRING_PTR (UserShortcut, primary),
  YAML_FIELD_STRING_PTR_OPTIONAL (UserShortcut, secondary),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t user_shortcut_schema = {
  YAML_VALUE_PTR (UserShortcut, user_shortcut_fields_schema),
};

static const cyaml_schema_field_t user_shortcuts_fields_schema[] = {
  YAML_FIELD_INT (UserShortcuts, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    UserShortcuts,
    shortcuts,
    user_shortcut_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t user_shortcuts_schema = {
  YAML_VALUE_PTR (UserShortcuts, user_shortcuts_fields_schema),
};

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
