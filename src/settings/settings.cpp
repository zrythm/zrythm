// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Application settings.
 */

#include "zrythm-config.h"

#include <stdio.h>
#include <stdlib.h>

#include "settings/plugin_settings.h"
#include "settings/settings.h"
#include "settings/user_shortcuts.h"
#include "utils/gtk.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/terminal.h"
#include "zrythm.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

void
Settings::init ()
{
  if (ZRYTHM_TESTING)
    {
      return;
    }

  this->general = g_settings_new (GSETTINGS_ZRYTHM_PREFIX ".general");
  this->export_audio = g_settings_new (GSETTINGS_ZRYTHM_PREFIX ".export.audio");
  this->export_midi = g_settings_new (GSETTINGS_ZRYTHM_PREFIX ".export.midi");
  this->monitor = g_settings_new (GSETTINGS_ZRYTHM_PREFIX ".monitor");
  this->ui = g_settings_new (GSETTINGS_ZRYTHM_PREFIX ".ui");
  this->ui_inspector = g_settings_new (GSETTINGS_ZRYTHM_PREFIX ".ui.inspector");
  this->ui_mixer = g_settings_new (GSETTINGS_ZRYTHM_PREFIX ".ui.mixer");
  this->ui_plugin_browser =
    g_settings_new (GSETTINGS_ZRYTHM_PREFIX ".ui.plugin-browser");
  this->ui_panels = g_settings_new (GSETTINGS_ZRYTHM_PREFIX ".ui.panels");
  this->ui_file_browser =
    g_settings_new (GSETTINGS_ZRYTHM_PREFIX ".ui.file-browser");
  this->transport = g_settings_new (GSETTINGS_ZRYTHM_PREFIX ".transport");

#define PREFERENCES_PREFIX GSETTINGS_ZRYTHM_PREFIX ".preferences."
#define NEW_PREFERENCES_SETTINGS(a, b) \
  this->preferences_##a##_##b = g_settings_new (PREFERENCES_PREFIX #a "." #b); \
  g_return_if_fail (this->preferences_##a##_##b)

  NEW_PREFERENCES_SETTINGS (dsp, pan);
  NEW_PREFERENCES_SETTINGS (editing, audio);
  NEW_PREFERENCES_SETTINGS (editing, automation);
  NEW_PREFERENCES_SETTINGS (editing, undo);
  NEW_PREFERENCES_SETTINGS (general, engine);
  NEW_PREFERENCES_SETTINGS (general, paths);
  NEW_PREFERENCES_SETTINGS (general, updates);
  NEW_PREFERENCES_SETTINGS (plugins, uis);
  NEW_PREFERENCES_SETTINGS (plugins, paths);
  NEW_PREFERENCES_SETTINGS (projects, general);
  NEW_PREFERENCES_SETTINGS (ui, general);

#undef PREFERENCES_PREFIX

  g_return_if_fail (
    this->general && this->export_audio && this->ui && this->ui_inspector);

#if 0
  /* plugin settings */
  GSettingsBackend * psettings_backend =
    g_keyfile_settings_backend_new (
      "/filesystem/path/plugin-settings.ini",
      "/",
      NULL);
  this->plugin_settings =
    g_settings_new_with_backend (
      GSETTINGS_ZRYTHM_PREFIX ".plugin-settings",
      psettings_backend);
  g_settings_set_boolean (
    this->plugin_settings, "loop", 1);
#endif

  this->plugin_settings = plugin_settings_read_or_new ();
  g_return_if_fail (this->plugin_settings);

  this->user_shortcuts = user_shortcuts_new ();
  if (!this->user_shortcuts)
    {
      g_warning ("failed to parse user shortcuts");
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
      g_return_val_if_fail (schema_key, -1);
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
          g_return_val_if_fail (val, -1);
          char * var_str = g_variant_print (val, 0);
          sprintf (tmp, _ ("resetting %s to %s"), key, var_str);
          printf ("  %s\n", tmp);
          int ret = g_settings_set_value (settings, key, val);
          if (!ret)
            {
              g_warning ("Failed to set value");
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
  g_settings_schema_source_list_schemas (source, 1, &non_relocatable, NULL);
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

void
settings_reset_to_factory (bool confirm, bool exit_on_finish)
{
  if (confirm)
    {
      printf (
        "%s ",
        _ ("This will reset Zrythm to factory settings. You will lose all "
           "your preferences. Type 'y' to continue:"));
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

/**
 * Prints the current settings.
 */
void
settings_print (int pretty_print)
{
  print_or_reset (1, pretty_print, 0);
}

GVariant *
settings_get_range (const char * schema, const char * key)
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
settings_get_default_value (const char * schema, const char * key)
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
settings_get_default_value_double (const char * schema, const char * key)
{
  GVariant * def_val = settings_get_default_value (schema, key);

  double ret = g_variant_get_double (def_val);

  g_variant_unref (def_val);

  return ret;
}

void
settings_get_range_double (
  const char * schema,
  const char * key,
  double *     lower,
  double *     upper)
{
  GVariant * range = settings_get_range (schema, key);

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

/**
 * Returns the localized summary as a newly
 * allocated string.
 */
char *
settings_get_summary (GSettings * settings, const char * key)
{
  GSettingsSchema * settings_schema;
  g_object_get (settings, "settings-schema", &settings_schema, NULL);
  GSettingsSchemaKey * schema_key =
    g_settings_schema_get_key (settings_schema, key);
  char * descr = g_strdup (g_settings_schema_key_get_summary (schema_key));
  g_settings_schema_key_unref (schema_key);
  g_settings_schema_unref (settings_schema);

  return descr;
}

/**
 * Returns the localized description as a newly
 * allocated string.
 */
char *
settings_get_description (GSettings * settings, const char * key)
{
  GSettingsSchema * settings_schema;
  g_object_get (settings, "settings-schema", &settings_schema, NULL);
  GSettingsSchemaKey * schema_key =
    g_settings_schema_get_key (settings_schema, key);
  char * descr = g_strdup (g_settings_schema_key_get_description (schema_key));
  g_settings_schema_key_unref (schema_key);
  g_settings_schema_unref (settings_schema);

  return descr;
}

/**
 * Returns whether the "as" key contains the given
 * string.
 */
bool
settings_strv_contains_str (
  GSettings *  settings,
  const char * key,
  const char * val)
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

/**
 * Appends the given string to a key of type "as".
 */
void
settings_append_to_strv (
  GSettings *  settings,
  const char * key,
  const char * val,
  bool         ignore_if_duplicate)
{
  if (
    ignore_if_duplicate && !ZRYTHM_TESTING
    && settings_strv_contains_str (settings, key, val))
    {
      return;
    }

  g_debug (
    "%s: key %s val %s ignore if duplicate %d", __func__, key, val,
    ignore_if_duplicate);

  GStrvBuilder * builder = g_strv_builder_new ();
  char ** strv = ZRYTHM_TESTING ? NULL : g_settings_get_strv (settings, key);
  if (strv)
    g_strv_builder_addv (builder, (const char **) strv);
  g_strv_builder_add (builder, val);

  char ** new_strv = g_strv_builder_end (builder);

  if (ZRYTHM_TESTING)
    {
      for (size_t i = 0; new_strv[i] != NULL; i++)
        {
          char * tmp = new_strv[i];
          g_message ("setting [%zu]: %s", i, tmp);
        }
    }
  else
    {
      g_settings_set_strv (settings, key, (const char * const *) new_strv);
    }

  g_strfreev (strv);
  g_strfreev (new_strv);
}

/**
 * Frees settings.
 */
Settings::~Settings ()
{
#define FREE_SETTING(x) g_object_unref_and_null (this->x)

  FREE_SETTING (general);
  FREE_SETTING (preferences_dsp_pan);
  FREE_SETTING (preferences_editing_audio);
  FREE_SETTING (preferences_editing_automation);
  FREE_SETTING (preferences_editing_undo);
  FREE_SETTING (preferences_general_engine);
  FREE_SETTING (preferences_general_paths);
  FREE_SETTING (preferences_general_updates);
  FREE_SETTING (preferences_plugins_uis);
  FREE_SETTING (preferences_plugins_paths);
  FREE_SETTING (preferences_projects_general);
  FREE_SETTING (preferences_ui_general);
  FREE_SETTING (monitor);
  FREE_SETTING (ui);
  FREE_SETTING (transport);
  FREE_SETTING (export_audio);
  FREE_SETTING (export_midi);
  FREE_SETTING (ui_mixer);
  FREE_SETTING (ui_inspector);
  FREE_SETTING (ui_panels);
  FREE_SETTING (ui_plugin_browser);
  FREE_SETTING (ui_file_browser);

#undef FREE_SETTING

  object_free_w_func_and_null (plugin_settings_free, this->plugin_settings);
  object_free_w_func_and_null (user_shortcuts_free, this->user_shortcuts);
}
