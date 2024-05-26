// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Zrythm settings.
 */

#ifndef __SETTINGS_SETTINGS_H__
#define __SETTINGS_SETTINGS_H__

#include <memory>

#include <gtk/gtk.h>

typedef struct PluginSettings PluginSettings;
typedef struct UserShortcuts  UserShortcuts;

/**
 * @addtogroup project Settings
 *
 * @{
 */

#define GSETTINGS_ZRYTHM_PREFIX "org.zrythm.Zrythm"
#define SETTINGS (gZrythm->settings)

/* ---- Standard settings ---- */
#define S_MONITOR SETTINGS->monitor
#define S_UI SETTINGS->ui
#define S_EXPORT_AUDIO SETTINGS->export_audio
#define S_EXPORT_MIDI SETTINGS->export_midi
#define S_GENERAL SETTINGS->general
#define S_UI_INSPECTOR SETTINGS->ui_inspector
#define S_UI_MIXER SETTINGS->ui_mixer
#define S_UI_PANELS SETTINGS->ui_panels
#define S_UI_PLUGIN_BROWSER SETTINGS->ui_plugin_browser
#define S_UI_FILE_BROWSER SETTINGS->ui_file_browser
#define S_TRANSPORT SETTINGS->transport

/* ---- end standard settings ---- */

/* ---- Preferences ---- */
#define S_P_DSP_PAN SETTINGS->preferences_dsp_pan
#define S_P_EDITING_AUDIO SETTINGS->preferences_editing_audio
#define S_P_EDITING_AUTOMATION SETTINGS->preferences_editing_automation
#define S_P_EDITING_UNDO SETTINGS->preferences_editing_undo
#define S_P_GENERAL_ENGINE SETTINGS->preferences_general_engine
#define S_P_GENERAL_PATHS SETTINGS->preferences_general_paths
#define S_P_GENERAL_UPDATES SETTINGS->preferences_general_updates
#define S_P_PLUGINS_UIS SETTINGS->preferences_plugins_uis
#define S_P_PLUGINS_PATHS SETTINGS->preferences_plugins_paths
#define S_P_PROJECTS_GENERAL SETTINGS->preferences_projects_general
#define S_P_UI_GENERAL SETTINGS->preferences_ui_general
#define S_P_SCRIPTING_GENERAL SETTINGS->preferences_scripting_general

/* ---- end preferences ---- */

#define S_IS_DEBUG (g_settings_get_int (S_GENERAL, "debug"))

#define S_SET_ENUM(settings, key, val) g_settings_set_enum (settings, key, val)

#define S_GET_ENUM(settings, key) g_settings_get_enum (settings, key)

#define S_UI_SET_ENUM(key, val) S_SET_ENUM (S_UI, key, val)

#define S_UI_GET_ENUM(key) S_GET_ENUM (S_UI, key)

#define S_PLUGIN_SETTINGS SETTINGS->plugin_settings

#define S_USER_SHORTCUTS SETTINGS->user_shortcuts

class Settings
{
public:
  Settings (){};
  ~Settings ();

  /**
   * Initializes settings.
   */
  void init ();

  /**
   * General settings, like recent projects list.
   */
  GSettings * general = nullptr;

  /** All preferences_* settings are to be shown in
   * the preferences dialog. */
  GSettings * preferences_dsp_pan = nullptr;
  GSettings * preferences_editing_audio = nullptr;
  GSettings * preferences_editing_automation = nullptr;
  GSettings * preferences_editing_undo = nullptr;
  GSettings * preferences_general_engine = nullptr;
  GSettings * preferences_general_paths = nullptr;
  GSettings * preferences_general_updates = nullptr;
  GSettings * preferences_plugins_uis = nullptr;
  GSettings * preferences_plugins_paths = nullptr;
  GSettings * preferences_projects_general = nullptr;
  GSettings * preferences_ui_general = nullptr;
  GSettings * preferences_scripting_general = nullptr;

  /** Monitor settings. */
  GSettings * monitor = nullptr;

  /**
   * UI memory.
   *
   * This is for storing things like last selections, etc.,
   * that do not appear in the preferences but are "silently"
   * remembered.
   */
  GSettings * ui = nullptr;

  /** Transport settings. */
  GSettings * transport = nullptr;

  GSettings * export_audio = nullptr;
  GSettings * export_midi = nullptr;

  GSettings * ui_mixer = nullptr;
  GSettings * ui_inspector = nullptr;
  GSettings * ui_panels = nullptr;
  GSettings * ui_plugin_browser = nullptr;
  GSettings * ui_file_browser = nullptr;

  PluginSettings * plugin_settings = nullptr;

  UserShortcuts * user_shortcuts = nullptr;
};

/**
 * Resets settings to defaults.
 *
 * @param confirm Show command line confirmation option.
 * @param exit_on_finish Exit with a code on finish.
 */
void
settings_reset_to_factory (bool confirm, bool exit_on_finish);

/**
 * Prints the current settings.
 */
void
settings_print (int pretty_print);

/**
 * Returns whether the "as" key contains the given
 * string.
 */
NONNULL bool
settings_strv_contains_str (
  GSettings *  settings,
  const char * key,
  const char * val);

/**
 * Appends the given string to a key of type "as".
 */
NONNULL void
settings_append_to_strv (
  GSettings *  settings,
  const char * key,
  const char * val,
  bool         ignore_if_duplicate);

GVariant *
settings_get_range (const char * schema, const char * key);

void
settings_get_range_double (
  const char * schema,
  const char * key,
  double *     lower,
  double *     upper);

GVariant *
settings_get_default_value (const char * schema, const char * key);

double
settings_get_default_value_double (const char * schema, const char * key);

/**
 * Returns the localized summary as a newly
 * allocated string.
 */
char *
settings_get_summary (GSettings * settings, const char * key);

/**
 * Returns the localized description as a newly
 * allocated string.
 */
char *
settings_get_description (GSettings * settings, const char * key);

/**
 * @}
 */

#endif
