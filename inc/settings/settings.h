/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __SETTINGS_SETTINGS_H__
#define __SETTINGS_SETTINGS_H__

#include <gtk/gtk.h>

#define SETTINGS (&ZRYTHM->settings)
#define S_PREFERENCES SETTINGS->preferences
#define S_UI SETTINGS->ui
#define S_GENERAL SETTINGS->general
#define S_IS_DEBUG (g_settings_get_int ( \
  S_GENERAL, "debug"))

typedef struct Settings
{
  GSettings * root;

  /**
   * This is for everything stored in preferences that can
   * be edited by the user.
   */
  GSettings * preferences;

  /**
   * General settings, like recent projects list.
   */
  GSettings * general;

  /**
   * UI memory.
   *
   * This is for storing things like last selections, etc.,
   * that do not appear in the preferences but are "silently"
   * remembered.
   */
  GSettings * ui;
} Settings;

/**
 * Initializes settings.
 */
void
settings_init (Settings * self);

#endif
