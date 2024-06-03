// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * GSettings Manager.
 */

#ifndef __SETTINGS_G_SETTINGS_MANAGER_H__
#define __SETTINGS_G_SETTINGS_MANAGER_H__

#include "ext/juce/juce.h"
#include "gtk_wrapper.h"

/**
 * @addtogroup project Settings
 *
 * @{
 */

/**
 * @brief A wrapper class for a GSettings object.
 *
 * This class provides a convenient way to work with GSettings objects. It
 * handles the creation, lifetime, and ownership of the GSettings object.
 *
 * The class provides copy and move semantics, as well as a swap operation.
 * When the object is destroyed, the underlying GSettings object is
 * automatically unreferenced.
 */
struct GSettingsWrapper
{
  GSettingsWrapper () = default;

  GSettingsWrapper (std::string schema_id)
      : schema_id_ (std::move (schema_id)),
        settings_ (g_settings_new (schema_id_.c_str ()))
  {
    if (!settings_)
      {
        g_critical ("invalid settings schema id: %s", schema_id_.c_str ());
      }
  }

  ~GSettingsWrapper () { reset (); }

  GSettingsWrapper (const GSettingsWrapper &other)
      : GSettingsWrapper (other.schema_id_)
  {
  }

  GSettingsWrapper &operator= (const GSettingsWrapper &other)
  {
    GSettingsWrapper tmp (other);
    swap (tmp);
    return *this;
  }

  GSettingsWrapper (GSettingsWrapper &&other) noexcept
      : schema_id_ (std::move (other.schema_id_)), settings_ (other.settings_)
  {
    other.settings_ = nullptr;
  }

  GSettingsWrapper &operator= (GSettingsWrapper &&other) noexcept
  {
    swap (other);
    return *this;
  }

  void swap (GSettingsWrapper &other) noexcept
  {
    using std::swap;
    swap (schema_id_, other.schema_id_);
    swap (settings_, other.settings_);
  }

  void reset ()
  {
    if (settings_)
      {
        g_object_unref (settings_);
        settings_ = nullptr;
      }
  }

  std::string schema_id_;
  GSettings * settings_ = nullptr;
};

class GSettingsManager
{
public:
  GSettingsManager ();
  ~GSettingsManager ();

  std::unordered_map<std::string, GSettingsWrapper> settings_;

  /**
   * @brief Get the GSettings instance for the given schema id.
   *
   * @param schema_id
   * @return A GSettings instance owned by this class that must not be unref'd.
   */
  GSettings * get_settings (const std::string &schema_id);

  /**
   * Resets settings to defaults.
   *
   * @param confirm Show command line confirmation option.
   * @param exit_on_finish Exit with a code on finish.
   */
  static void reset_to_factory (bool confirm, bool exit_on_finish);

  /**
   * Prints the current settings.
   */
  static void print_all_settings (bool pretty_print);

  /**
   * Returns whether the "as" key contains the given
   * string.
   */
  NONNULL static bool
  strv_contains_str (GSettings * settings, const char * key, const char * val);

  /**
   * Appends the given string to a key of type "as".
   */
  NONNULL static void append_to_strv (
    GSettings *  settings,
    const char * key,
    const char * val,
    bool         ignore_if_duplicate);

  static GVariant * get_range (const char * schema, const char * key);

  static void get_range_double (
    const char * schema,
    const char * key,
    double *     lower,
    double *     upper);

  static GVariant * get_default_value (const char * schema, const char * key);

  double static get_default_value_double (const char * schema, const char * key);

  /**
   * Returns the localized summary as a newly allocated string.
   */
  static char * get_summary (GSettings * settings, const char * key);

  /**
   * Returns the localized description as a newly allocated string.
   */
  static char * get_description (GSettings * settings, const char * key);

public:
  JUCE_DECLARE_SINGLETON_SINGLETHREADED (GSettingsManager, true)
};

#define GSETTINGS_ZRYTHM_PREFIX "org.zrythm.Zrythm"

/* ---- Standard settings ---- */

#define S_GENERAL_SCHEMA (GSETTINGS_ZRYTHM_PREFIX ".general")
#define S_MONITOR_SCHEMA (GSETTINGS_ZRYTHM_PREFIX ".monitor")
#define S_UI_SCHEMA (GSETTINGS_ZRYTHM_PREFIX ".ui")
#define S_EXPORT_AUDIO_SCHEMA (GSETTINGS_ZRYTHM_PREFIX ".export.audio")
#define S_EXPORT_MIDI_SCHEMA (GSETTINGS_ZRYTHM_PREFIX ".export.midi")
#define S_UI_INSPECTOR_SCHEMA (GSETTINGS_ZRYTHM_PREFIX ".ui.inspector")
#define S_UI_MIXER_SCHEMA (GSETTINGS_ZRYTHM_PREFIX ".ui.mixer")
#define S_UI_PANELS_SCHEMA (GSETTINGS_ZRYTHM_PREFIX ".ui.panels")
#define S_UI_PLUGIN_BROWSER_SCHEMA \
  (GSETTINGS_ZRYTHM_PREFIX ".ui.plugin-browser")
#define S_UI_FILE_BROWSER_SCHEMA (GSETTINGS_ZRYTHM_PREFIX ".ui.file-browser")
#define S_TRANSPORT_SCHEMA (GSETTINGS_ZRYTHM_PREFIX ".transport")

#define GET_SETTINGS_FOR(schema) \
  GSettingsManager::getInstance ()->get_settings (schema)

#define S_GENERAL GET_SETTINGS_FOR (S_GENERAL_SCHEMA)
#define S_MONITOR GET_SETTINGS_FOR (S_MONITOR_SCHEMA)
#define S_UI GET_SETTINGS_FOR (S_UI_SCHEMA)
#define S_EXPORT_AUDIO GET_SETTINGS_FOR (S_EXPORT_AUDIO_SCHEMA)
#define S_EXPORT_MIDI GET_SETTINGS_FOR (S_EXPORT_MIDI_SCHEMA)
#define S_UI_INSPECTOR GET_SETTINGS_FOR (S_UI_INSPECTOR_SCHEMA)
#define S_UI_MIXER GET_SETTINGS_FOR (S_UI_MIXER_SCHEMA)
#define S_UI_PANELS GET_SETTINGS_FOR (S_UI_PANELS_SCHEMA)
#define S_UI_PLUGIN_BROWSER GET_SETTINGS_FOR (S_UI_PLUGIN_BROWSER_SCHEMA)
#define S_UI_FILE_BROWSER GET_SETTINGS_FOR (S_UI_FILE_BROWSER_SCHEMA)
#define S_TRANSPORT GET_SETTINGS_FOR (S_TRANSPORT_SCHEMA)

/* ---- end standard settings ---- */

/* ---- Preferences ---- */

#define GSETTINGS_PREFERENCES_PREFIX GSETTINGS_ZRYTHM_PREFIX ".preferences"

#define S_P_DSP_PAN_SCHEMA (GSETTINGS_PREFERENCES_PREFIX ".dsp.pan")
#define S_P_EDITING_AUDIO_SCHEMA (GSETTINGS_PREFERENCES_PREFIX ".editing.audio")
#define S_P_EDITING_AUTOMATION_SCHEMA \
  (GSETTINGS_PREFERENCES_PREFIX ".editing.automation")
#define S_P_EDITING_UNDO_SCHEMA (GSETTINGS_PREFERENCES_PREFIX ".editing.undo")
#define S_P_GENERAL_ENGINE_SCHEMA \
  (GSETTINGS_PREFERENCES_PREFIX ".general.engine")
#define S_P_GENERAL_PATHS_SCHEMA (GSETTINGS_PREFERENCES_PREFIX ".general.paths")
#define S_P_GENERAL_UPDATES_SCHEMA \
  (GSETTINGS_PREFERENCES_PREFIX ".general.updates")
#define S_P_PLUGINS_UIS_SCHEMA (GSETTINGS_PREFERENCES_PREFIX ".plugins.uis")
#define S_P_PLUGINS_PATHS_SCHEMA (GSETTINGS_PREFERENCES_PREFIX ".plugins.paths")
#define S_P_PROJECTS_GENERAL_SCHEMA \
  (GSETTINGS_PREFERENCES_PREFIX ".projects.general")
#define S_P_UI_GENERAL_SCHEMA (GSETTINGS_PREFERENCES_PREFIX ".ui.general")
#define S_P_SCRIPTING_GENERAL_SCHEMA \
  (GSETTINGS_PREFERENCES_PREFIX ".scripting.general")

#define S_P_DSP_PAN GET_SETTINGS_FOR (S_P_DSP_PAN_SCHEMA)
#define S_P_EDITING_AUDIO GET_SETTINGS_FOR (S_P_EDITING_AUDIO_SCHEMA)
#define S_P_EDITING_AUTOMATION GET_SETTINGS_FOR (S_P_EDITING_AUTOMATION_SCHEMA)
#define S_P_EDITING_UNDO GET_SETTINGS_FOR (S_P_EDITING_UNDO_SCHEMA)
#define S_P_GENERAL_ENGINE GET_SETTINGS_FOR (S_P_GENERAL_ENGINE_SCHEMA)
#define S_P_GENERAL_PATHS GET_SETTINGS_FOR (S_P_GENERAL_PATHS_SCHEMA)
#define S_P_GENERAL_UPDATES GET_SETTINGS_FOR (S_P_GENERAL_UPDATES_SCHEMA)
#define S_P_PLUGINS_UIS GET_SETTINGS_FOR (S_P_PLUGINS_UIS_SCHEMA)
#define S_P_PLUGINS_PATHS GET_SETTINGS_FOR (S_P_PLUGINS_PATHS_SCHEMA)
#define S_P_PROJECTS_GENERAL GET_SETTINGS_FOR (S_P_PROJECTS_GENERAL_SCHEMA)
#define S_P_UI_GENERAL GET_SETTINGS_FOR (S_P_UI_GENERAL_SCHEMA)
#define S_P_SCRIPTING_GENERAL GET_SETTINGS_FOR (S_P_SCRIPTING_GENERAL_SCHEMA)

/* ---- end preferences ---- */

/**
 * @}
 */

#endif // __SETTINGS_G_SETTINGS_MANAGER_H__