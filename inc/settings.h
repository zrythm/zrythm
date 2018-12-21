/*
 * settings.h - Settings
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include <gtk/gtk.h>

#define SETTINGS ZRYTHM->settings

typedef struct Settings
{
  GSettings * gsettings;
} Settings;

Settings *
settings_new ();

/**
 * Initializes the settings manager
 */
void
settings_init (Settings * self);

/**
 * Returns the value for the given key
 */
GVariant *
settings_get_value (Settings * self,
                    const char * key);

int
settings_get_int (Settings * self,
                  const char * key);

const char *
settings_get_string (Settings * self,
                     const char * key);

/**
 * Stores given value in given key
 */
void
settings_store_value (Settings * self,
                      char     * key,
                      GVariant * value);

#endif
