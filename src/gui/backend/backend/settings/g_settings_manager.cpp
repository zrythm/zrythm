// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * GSettings Manager implementation.
 */

#include "zrythm-config.h"

#include <cstdio>
#include <cstdlib>

#include "common/utils/gtest_wrapper.h"
#include "common/utils/terminal.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#include <glib/gi18n.h>

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

JUCE_IMPLEMENT_SINGLETON (GSettingsManager)

GSettingsManager::GSettingsManager ()
{
  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      return;
    }
}

GSettings *
GSettingsManager::get_settings (const std::string &schema_id)
{
  auto it = settings_.find (schema_id);
  if (it == settings_.end ())
    {
      settings_[schema_id] = GSettingsWrapper (schema_id);
      return settings_[schema_id].settings_;
    }
  else
    {
      return it->second.settings_;
    }
}

static int
print_or_reset_schema (
  GSettingsSchema * schema,
  const char *      schema_str,
  int               print_only,
  int               pretty_print)
{
  GSettings * settings = g_settings_new (schema_str);
  char **     keys = g_settings_schema_list_keys (schema);

  char tmp[8000];
  if (print_only)
    {
      sprintf (
        tmp, _ ("Schema %s%s%s"), pretty_print ? TERMINAL_BOLD : "", schema_str,
        pretty_print ? TERMINAL_RESET : "");
      printf ("%s%s\n", pretty_print ? TERMINAL_UNDERLINE : "", tmp);
    }
  else
    {
      printf (_ ("Resetting schema %s...\n"), schema_str);
    }

  int    i = 0;
  char * key;
  while ((key = keys[i++]))
    {
      GSettingsSchemaKey * schema_key = g_settings_schema_get_key (schema, key);
      z_return_val_if_fail (schema_key, -1);
      if (print_only)
        {
          GVariant * val = g_settings_get_value (settings, key);
          char *     var_str = g_variant_print (val, 0);
          printf (
            "  %s%s%s=%s%s%s\n",
            pretty_print ? TERMINAL_COLOR_MAGENTA TERMINAL_BOLD : "", key,
            pretty_print ? TERMINAL_RESET : "",
            pretty_print ? TERMINAL_COLOR_YELLOW : "", var_str,
            pretty_print ? TERMINAL_RESET : "");
          if (pretty_print)
            {
              const char * description =
                g_settings_schema_key_get_description (schema_key);
              printf ("    %s\n", _ (description));
            }
        }
      else if (
        /* don't reset install-dir */
        string_is_equal (key, "install-dir"))
        {
          sprintf (tmp, _ ("skipping %s"), key);
          printf ("  %s\n", tmp);
        }
      else
        {
          GVariant * val = g_settings_schema_key_get_default_value (schema_key);
          z_return_val_if_fail (val, -1);
          char * var_str = g_variant_print (val, 0);
          sprintf (tmp, _ ("resetting %s to %s"), key, var_str);
          printf ("  %s\n", tmp);
          int ret = g_settings_set_value (settings, key, val);
          if (!ret)
            {
              z_warning ("Failed to set value");
              return -1;
            }
        }
    }

  return 0;
}

/**
 * @param print_only Only print the settings without
 *   resetting.
 */
static int
print_or_reset (int print_only, int pretty_print, int exit_on_finish)
{
  GSettingsSchemaSource * source = g_settings_schema_source_get_default ();
  char **                 non_relocatable;
  g_settings_schema_source_list_schemas (source, 1, &non_relocatable, nullptr);
  int    i = 0;
  char * schema_str;
  while ((schema_str = non_relocatable[i++]))
    {
      if (string_contains_substr (schema_str, GSETTINGS_ZRYTHM_PREFIX))
        {
          GSettingsSchema * schema =
            g_settings_schema_source_lookup (source, schema_str, 1);
          if (!schema)
            {
              fprintf (
                stderr,
                _ ("Failed to find schema %s. %s is likely not installed"),
                schema_str, PROGRAM_NAME);
              if (exit_on_finish)
                exit (1);
              else
                return -1;
            }
          if (
            print_or_reset_schema (schema, schema_str, print_only, pretty_print))
            {
              if (print_only)
                {
                  fprintf (stderr, _ ("Failed to view schema %s"), schema_str);
                }
              else
                {
                  fprintf (stderr, _ ("Failed to reset schema %s"), schema_str);
                }
              if (exit_on_finish)
                exit (1);
              else
                return -1;
            }
        }
    }

  if (!print_only)
    {
      g_settings_sync ();
    }

  return 0;
}

GVariant *
GSettingsManager::get_range (const char * schema, const char * key)
{
  GSettingsSchemaSource * source = g_settings_schema_source_get_default ();
  GSettingsSchema *       g_settings_schema =
    g_settings_schema_source_lookup (source, schema, 1);
  GSettingsSchemaKey * schema_key =
    g_settings_schema_get_key (g_settings_schema, key);
  GVariant * range = g_settings_schema_key_get_range (schema_key);

  g_settings_schema_key_unref (schema_key);
  g_settings_schema_unref (g_settings_schema);

  return range;
}

GVariant *
GSettingsManager::get_default_value (const char * schema, const char * key)
{
  GSettingsSchemaSource * source = g_settings_schema_source_get_default ();
  GSettingsSchema *       g_settings_schema =
    g_settings_schema_source_lookup (source, schema, 1);
  GSettingsSchemaKey * schema_key =
    g_settings_schema_get_key (g_settings_schema, key);
  GVariant * def_val = g_settings_schema_key_get_default_value (schema_key);

  g_settings_schema_key_unref (schema_key);
  g_settings_schema_unref (g_settings_schema);

  return def_val;
}

double
GSettingsManager::get_default_value_double (const char * schema, const char * key)
{
  GVariant * def_val = get_default_value (schema, key);

  double ret = g_variant_get_double (def_val);

  g_variant_unref (def_val);

  return ret;
}

void
GSettingsManager::get_range_double (
  const char * schema,
  const char * key,
  double *     lower,
  double *     upper)
{
  GVariant * range = get_range (schema, key);

  GVariant * range_vals = g_variant_get_child_value (range, 1);
  range_vals = g_variant_get_child_value (range_vals, 0);
  GVariant * lower_var = g_variant_get_child_value (range_vals, 0);
  GVariant * upper_var = g_variant_get_child_value (range_vals, 1);

  *lower = g_variant_get_double (lower_var);
  *upper = g_variant_get_double (upper_var);

  g_variant_unref (range_vals);
  g_variant_unref (lower_var);
  g_variant_unref (upper_var);
}

char *
GSettingsManager::get_summary (GSettings * settings, const char * key)
{
  GSettingsSchema * settings_schema;
  g_object_get (settings, "settings-schema", &settings_schema, nullptr);
  GSettingsSchemaKey * schema_key =
    g_settings_schema_get_key (settings_schema, key);
  char * descr = g_strdup (g_settings_schema_key_get_summary (schema_key));
  g_settings_schema_key_unref (schema_key);
  g_settings_schema_unref (settings_schema);

  return descr;
}

char *
GSettingsManager::get_description (GSettings * settings, const char * key)
{
  GSettingsSchema * settings_schema;
  g_object_get (settings, "settings-schema", &settings_schema, nullptr);
  GSettingsSchemaKey * schema_key =
    g_settings_schema_get_key (settings_schema, key);
  char * descr = g_strdup (g_settings_schema_key_get_description (schema_key));
  g_settings_schema_key_unref (schema_key);
  g_settings_schema_unref (settings_schema);

  return descr;
}

bool
GSettingsManager::
  strv_contains_str (GSettings * settings, const char * key, const char * val)
{
  char ** strv = g_settings_get_strv (settings, key);

  for (int i = 0;; i++)
    {
      char * str = strv[i];
      if (!str)
        break;

      if (string_is_equal (str, val))
        {
          g_strfreev (strv);
          return true;
        }
    }

  g_strfreev (strv);

  return false;
}

void
GSettingsManager::append_to_strv (
  GSettings *  settings,
  const char * key,
  const char * val,
  bool         ignore_if_duplicate)
{
  if (
    ignore_if_duplicate && !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING
    && strv_contains_str (settings, key, val))
    {
      return;
    }

  z_debug (
    "%s: key %s val %s ignore if duplicate %d", __func__, key, val,
    ignore_if_duplicate);

  GStrvBuilder * builder = g_strv_builder_new ();
  char **        strv =
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
             ? nullptr
             : g_settings_get_strv (settings, key);
  if (strv)
    g_strv_builder_addv (builder, (const char **) strv);
  g_strv_builder_add (builder, val);

  char ** new_strv = g_strv_builder_end (builder);

  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      for (size_t i = 0; new_strv[i] != NULL; i++)
        {
          char * tmp = new_strv[i];
          z_info ("setting [{}]: {}", i, tmp);
        }
    }
  else
    {
      g_settings_set_strv (settings, key, (const char * const *) new_strv);
    }

  g_strfreev (strv);
  g_strfreev (new_strv);
}

void
GSettingsManager::reset_to_factory (bool confirm, bool exit_on_finish)
{
  if (confirm)
    {
      printf (
        "%s ",
        _ ("This will reset Zrythm to factory settings. You will lose all your preferences. Type 'y' to continue:"));
      char c = getchar ();
      if (c != 'y')
        {
          printf (_ ("Aborting...\n"));
          if (exit_on_finish)
            exit (0);

          return;
        }
    }

  print_or_reset (0, 0, exit_on_finish);

  printf (_ ("Reset to factory settings successful\n"));
}

void
GSettingsManager::print_all_settings (bool pretty_print)
{
  print_or_reset (true, pretty_print, 0);
}

GSettingsManager::~GSettingsManager ()
{
  clearSingletonInstance ();
}