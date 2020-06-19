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
 * Zrythm settings.
 */

#ifndef __SETTINGS_SETTINGS_H__
#define __SETTINGS_SETTINGS_H__

#include <gtk/gtk.h>

/**
 * @addtogroup project Settings
 *
 * @{
 */

#define GSETTINGS_ZRYTHM_PREFIX "org.zrythm.Zrythm"
#define SETTINGS (ZRYTHM->settings)
#define S_UI SETTINGS->ui
#define S_EXPORT SETTINGS->export
#define S_P_DSP_PAN SETTINGS->preferences_dsp_pan
#define S_P_EDITING_AUDIO \
  SETTINGS->preferences_editing_audio
#define S_P_EDITING_AUTOMATION \
  SETTINGS->preferences_editing_automation
#define S_P_EDITING_UNDO \
  SETTINGS->preferences_editing_undo
#define S_P_GENERAL_ENGINE \
  SETTINGS->preferences_general_engine
#define S_P_GENERAL_PATHS \
  SETTINGS->preferences_general_paths
#define S_P_PLUGINS_UIS \
  SETTINGS->preferences_plugins_uis
#define S_P_PLUGINS_PATHS \
  SETTINGS->preferences_plugins_paths
#define S_P_PROJECTS_GENERAL \
  SETTINGS->preferences_projects_general
#define S_P_UI_GENERAL \
  SETTINGS->preferences_ui_general
#define S_GENERAL SETTINGS->general
#define S_UI_INSPECTOR_SETTINGS \
  SETTINGS->ui_inspector
#define S_TRANSPORT \
  SETTINGS->transport
#define S_IS_DEBUG (g_settings_get_int ( \
  S_GENERAL, "debug"))

#define S_UI_SET_ENUM(key,val) \
  g_settings_set_enum ( \
    S_UI, key, val)

#define S_UI_GET_ENUM(key) \
  g_settings_get_enum ( \
    S_UI, key)

typedef struct Settings
{
  /**
   * General settings, like recent projects list.
   */
  GSettings * general;

  /** All preferences_* settings are to be shown in
   * the preferences dialog. */
  GSettings * preferences_dsp_pan;
  GSettings * preferences_editing_audio;
  GSettings * preferences_editing_automation;
  GSettings * preferences_editing_undo;
  GSettings * preferences_general_engine;
  GSettings * preferences_general_paths;
  GSettings * preferences_plugins_uis;
  GSettings * preferences_plugins_paths;
  GSettings * preferences_projects_general;
  GSettings * preferences_ui_general;

  /**
   * UI memory.
   *
   * This is for storing things like last selections, etc.,
   * that do not appear in the preferences but are "silently"
   * remembered.
   */
  GSettings * ui;

  /** Transport settings. */
  GSettings * transport;

  GSettings * export;

  GSettings * ui_inspector;
} Settings;

/**
 * Initializes settings.
 */
Settings *
settings_new (void);

/**
 * Resets settings to defaults.
 *
 * @param exit_on_finish Exit with a code on
 *   finish.
 */
void
settings_reset_to_factory (
  int confirm,
  int exit_on_finish);

/**
 * Prints the current settings.
 */
void
settings_print (
  int pretty_print);

/**
 * Frees settings.
 */
void
settings_free (
  Settings * self);

/**
 * @}
 */

#endif
