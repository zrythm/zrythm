/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "plugins/carla/carla_discovery.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin_descriptor.h"
#include "settings/plugin_settings.h"
#include "settings/settings.h"
#include "utils/file.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"

#define PLUGIN_SETTINGS_VERSION 2
#define PLUGIN_SETTINGS_FILENAME "plugin-settings.yaml"

/**
 * Creates a plugin setting with the recommended
 * settings for the given plugin descriptor based
 * on the current setup.
 */
PluginSetting *
plugin_setting_new_default (
  PluginDescriptor * descr)
{
  PluginSetting * existing = NULL;
  if (S_PLUGIN_SETTINGS && !ZRYTHM_TESTING)
    {
      existing =
        plugin_settings_find (
          S_PLUGIN_SETTINGS, descr);
    }

  PluginSetting * self = NULL;
  if (existing)
    {
      self = plugin_setting_clone (existing, true);
    }
  else
    {
      self = object_new (PluginSetting);
      self->descr = plugin_descriptor_clone (descr);
      plugin_setting_validate (self);
    }

  return self;
}

PluginSetting *
plugin_setting_clone (
  PluginSetting * src,
  bool            validate)
{
  PluginSetting * new_setting =
    object_new (PluginSetting);

  *new_setting = *src;
  new_setting->descr =
    plugin_descriptor_clone (src->descr);
  new_setting->ui_uri = g_strdup (src->ui_uri);

  if (validate)
    {
      plugin_setting_validate (new_setting);
    }

  return new_setting;
}

void
plugin_setting_print (
  const PluginSetting * self)
{
  g_message (
    "[PluginSetting]\n"
    "descr.uri=%s\n"
    "open_with_carla=%d\n"
    "force_generic_ui=%d\n"
    "bridge_mode=%s\n"
    "ui_uri=%s",
    self->descr->uri, self->open_with_carla,
    self->force_generic_ui,
    carla_bridge_mode_strings[
      self->bridge_mode].str,
    self->ui_uri);
}

/**
 * Makes sure the setting is valid in the current
 * run and changes any fields to make it conform.
 *
 * For example, if the setting is set to not open
 * with carla but the descriptor is for a VST plugin,
 * this will set \ref PluginSetting.open_with_carla
 * to true.
 */
void
plugin_setting_validate (
  PluginSetting * self)
{
  const PluginDescriptor * descr = self->descr;

  if (descr->protocol == PROT_VST ||
      descr->protocol == PROT_VST3 ||
      descr->protocol == PROT_AU ||
      descr->protocol == PROT_SFZ ||
      descr->protocol == PROT_SF2)
    {
      self->open_with_carla = true;
#ifndef HAVE_CARLA
      g_critical (
        "Invalid setting requested: open non-LV2 "
        "plugin, carla not installed");
#endif
    }

#if defined (_WOE32) && defined (HAVE_CARLA)
  /* open all LV2 plugins with custom UIs using
   * carla */
  if (plugin_descriptor_has_custom_ui (
        self->descr) &&
      !global_force_generic)
    )
    {
      self->open_with_carla = true;
    }
#endif

#ifdef HAVE_CARLA
  /* in wayland open all LV2 plugins with custom UIs
   * using carla */
  if (z_gtk_is_wayland () &&
      plugin_descriptor_has_custom_ui (self->descr))
    {
      self->open_with_carla = true;
    }
#endif

#ifdef HAVE_CARLA
  /* if no bridge mode specified, calculate the
   * bridge mode here */
  g_message (
    "%s: recalculating bridge mode...",
    __func__);
  if (self->bridge_mode == CARLA_BRIDGE_NONE)
    {
      self->bridge_mode =
        z_carla_discovery_get_bridge_mode (descr);
      if (self->bridge_mode != CARLA_BRIDGE_NONE)
        {
          self->open_with_carla = true;
        }
    }
  else
    {
      self->open_with_carla = true;
      CarlaBridgeMode mode =
        z_carla_discovery_get_bridge_mode (descr);

      if (mode == CARLA_BRIDGE_FULL)
        {
          self->bridge_mode = mode;
        }
    }
#endif

  /* if no custom UI, force generic */
  if (!plugin_descriptor_has_custom_ui (self->descr))
    {
      self->force_generic_ui = true;
    }

  /* if setting cannot have a UI URI, clear it */
  if (self->descr->protocol != PROT_LV2 ||
      self->open_with_carla ||
      self->force_generic_ui)
    {
      g_free_and_null (self->ui_uri);
    }
  /* otherwise validate it */
  else
    {
      /* if already have UI uri and not supported,
       * clear it */
      if (self->ui_uri &&
          !lv2_plugin_is_ui_supported (
             self->descr->uri, self->ui_uri))
        {
          g_free_and_null (self->ui_uri);
        }

      /* if no UI URI, pick one */
      if (!self->ui_uri)
        {
          char * picked_ui = NULL;
          bool ui_picked =
            lv2_plugin_pick_most_preferable_ui (
              self->descr->uri, &picked_ui,
              NULL, true);

          /* if a suitable UI was found, set the
           * UI URI */
          if (ui_picked)
            {
              g_return_if_fail (picked_ui);
              self->ui_uri = picked_ui;
            }
          /* otherwise force generic UI */
          else
            {
              self->force_generic_ui = true;
            }
        }
    }

  g_debug ("plugin setting validated. new setting:");
  plugin_setting_print (self);
}

/**
 * Frees the plugin setting.
 */
void
plugin_setting_free (
  PluginSetting * self)
{
  object_free_w_func_and_null (
    plugin_descriptor_free, self->descr);

  object_zero_and_free (self);
}

static char *
get_plugin_settings_file_path (void)
{
  char * zrythm_dir =
    zrythm_get_dir (ZRYTHM_DIR_USER_TOP);
  g_return_val_if_fail (zrythm_dir, NULL);

  return
    g_build_filename (
      zrythm_dir, PLUGIN_SETTINGS_FILENAME, NULL);
}

void
plugin_settings_serialize_to_file (
  PluginSettings * self)
{
  self->version = PLUGIN_SETTINGS_VERSION;
  g_message ("Serializing plugin settings...");
  char * yaml = plugin_settings_serialize (self);
  g_return_if_fail (yaml);
  GError *err = NULL;
  char * path =
    get_plugin_settings_file_path ();
  g_return_if_fail (path && strlen (path) > 2);
  g_message (
    "Writing plugin settings to %s...", path);
  g_file_set_contents (path, yaml, -1, &err);
  if (err != NULL)
    {
      g_warning (
        "Unable to write plugin settings "
        "file: %s",
        err->message);
      g_error_free (err);
      g_free (path);
      g_free (yaml);
      g_return_if_reached ();
    }
  g_free (path);
  g_free (yaml);
}

static bool
is_yaml_our_version (
  const char * yaml)
{
  bool same_version = false;
  char version_str[120];
  sprintf (
    version_str, "version: %d\n",
    PLUGIN_SETTINGS_VERSION);
  same_version =
    g_str_has_prefix (yaml, version_str);
  if (!same_version)
    {
      sprintf (
        version_str, "---\nversion: %d\n",
        PLUGIN_SETTINGS_VERSION);
      same_version =
        g_str_has_prefix (yaml, version_str);
    }

  return same_version;
}

/**
 * Reads the file and fills up the object.
 */
PluginSettings *
plugin_settings_new (void)
{
  GError *err = NULL;
  char * path =
    get_plugin_settings_file_path ();
  if (!file_exists (path))
    {
      g_message (
        "Plugin settings file at %s does "
        "not exist", path);
return_new_instance:
      g_free (path);
      return object_new (PluginSettings);
    }
  char * yaml = NULL;
  g_file_get_contents (path, &yaml, NULL, &err);
  if (err != NULL)
    {
      g_critical (
        "Failed to create PluginSettings "
        "from %s", path);
      g_free (err);
      g_free (yaml);
      g_free (path);
      return NULL;
    }

  /* if not same version, purge file and return
   * a new instance */
  if (!is_yaml_our_version (yaml))
    {
      g_message (
        "Found old plugin settings file version. "
        "Purging file and creating a new one.");
      GFile * file =
        g_file_new_for_path (path);
      g_file_delete (file, NULL, NULL);
      g_object_unref (file);
      g_free (yaml);
      goto return_new_instance;
    }

  PluginSettings * self =
    plugin_settings_deserialize (yaml);
  if (!self)
    {
      g_critical (
        "Failed to deserialize "
        "PluginSettings from %s", path);
      g_free (err);
      g_free (yaml);
      g_free (path);
      return NULL;
    }
  g_free (yaml);
  g_free (path);

  return self;
}

/**
 * Finds a setting for the given plugin descriptor.
 *
 * @return The found setting or NULL.
 */
PluginSetting *
plugin_settings_find (
  PluginSettings *         self,
  const PluginDescriptor * descr)
{
  for (int i = 0; i < self->num_settings; i++)
    {
      PluginSetting * cur_setting = self->settings[i];
      PluginDescriptor * cur_descr =
        cur_setting->descr;
      if (plugin_descriptor_is_same_plugin (
            cur_descr, descr))
        {
          return cur_setting;
        }
    }

  return NULL;
}

/**
 * Replaces a setting or appends a setting to the
 * cache.
 *
 * This clones the setting before adding it.
 *
 * @param serialize Whether to serialize the updated
 *   cache now.
 */
void
plugin_settings_set (
  PluginSettings * self,
  PluginSetting *  setting,
  bool             _serialize)
{
  g_message ("Saving plugin setting");

  PluginSetting * own_setting =
    plugin_settings_find (
      S_PLUGIN_SETTINGS, setting->descr);

  if (own_setting)
    {
      own_setting->force_generic_ui =
        setting->force_generic_ui;
      own_setting->open_with_carla =
        setting->open_with_carla;
      own_setting->bridge_mode =
        setting->bridge_mode;
    }
  else
    {
      self->settings[self->num_settings++] =
        plugin_setting_clone (setting, F_VALIDATE);
    }

  if (_serialize)
    {
      plugin_settings_serialize_to_file (self);
    }
}

void
plugin_settings_free (
  PluginSettings * self)
{
  for (int i = 0; i < self->num_settings; i++)
    {
      object_free_w_func_and_null (
        plugin_setting_free,
        self->settings[i]);
    }

  object_zero_and_free (self);
}

SERIALIZE_SRC (
  PluginSettings, plugin_settings);
DESERIALIZE_SRC (
  PluginSettings, plugin_settings);
