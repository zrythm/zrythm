/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Application settings.
 */

#include <stdio.h>
#include <stdlib.h>

#include "settings/settings.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

void
settings_init (Settings * self)
{
  self->root =
    g_settings_new (GSETTINGS_ZRYTHM_PREFIX);
  self->general =
    g_settings_new (
      GSETTINGS_ZRYTHM_PREFIX ".general");
  self->preferences =
    g_settings_new (
      GSETTINGS_ZRYTHM_PREFIX ".preferences");
  self->ui =
    g_settings_new (
      GSETTINGS_ZRYTHM_PREFIX ".ui");
}

static int
reset_schema (
  GSettingsSchema * schema,
  const char *      schema_str)
{
  GSettings * settings =
    g_settings_new (schema_str);
  char ** keys =
    g_settings_schema_list_keys (schema);

  printf (_("Resetting schema %s...\n"), schema_str);

  int i = 0;
  char * key;
  while ((key = keys[i++]))
    {
      GSettingsSchemaKey * schema_key =
        g_settings_schema_get_key (schema, key);
      g_return_val_if_fail (schema_key, -1);
      GVariant * val =
        g_settings_schema_key_get_default_value (
          schema_key);
      g_return_val_if_fail (val, -1);
      char * var_str = g_variant_print (val, 0);
      printf (
        /* TRANSLATORS: please keep the spaces */
        _("  resetting %s to %s\n"), key, var_str);
      int ret =
        g_settings_set_value (settings, key, val);
      if (!ret)
        {
          g_warning ("Failed to set value");
          return -1;
        }
    }

  return 0;
}

/**
 * Resets settings to defaults.
 *
 * @param print_stdout Print the result to
 *   stdout.
 * @param exit_on_finish Exit with a code on
 *   finish.
 */
void
settings_reset_to_factory (
  int print_stdout,
  int exit_on_finish)
{
  printf (
    _("This will reset Zrythm to factory settings. "
    "You will lose all your preferences. Type 'y' "
    "to continue: "));
  char c = getchar ();
  if (c != 'y')
    {
      if (print_stdout)
        printf (_("Aborting...\n"));
      if (exit_on_finish)
        exit (0);

      return;
    }

  GSettingsSchemaSource * source =
    g_settings_schema_source_get_default ();
  char ** non_relocatable;
  g_settings_schema_source_list_schemas (
    source, 1, &non_relocatable, NULL);
  int i = 0;
  char * schema_str;
  while ((schema_str = non_relocatable[i++]))
    {
      if (string_contains_substr (
            schema_str, GSETTINGS_ZRYTHM_PREFIX, 0))
        {
          GSettingsSchema * schema =
            g_settings_schema_source_lookup (
              source, schema_str, 1);
          if (!schema)
            {
              if (print_stdout)
                {
                  fprintf (
                    stderr,
                    _("Failed to find schema %s. Zrythm is likely not installed"),
                    schema_str);
                }
              if (exit_on_finish)
                {
                  exit (1);
                }
              g_return_if_reached ();
            }
          if (reset_schema (schema, schema_str))
            {
              if (print_stdout)
                {
                  fprintf (
                    stderr,
                    _("Failed to reset schema %s"),
                    schema_str);
                }
              if (exit_on_finish)
                {
                  exit (1);
                }
              g_return_if_reached ();
            }
        }
    }

  g_settings_sync ();

  if (print_stdout)
    {
      printf (
        _("Reset to factory settings successful\n"));
    }

  if (exit_on_finish)
    {
      exit (0);
    }
}

/**
 * Frees settings.
 */
void
settings_free_members (Settings * self)
{
  if (self->root)
    g_object_unref_and_null (self->root);
  if (self->general)
    g_object_unref_and_null (self->general);
  if (self->preferences)
    g_object_unref_and_null (self->preferences);
  if (self->ui)
    g_object_unref_and_null (self->ui);
}
