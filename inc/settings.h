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
 * @brief Returns the value for the given key
 *
 * @param key       they key
 */
GVariant *
get_value (const char      * key);

int
get_int (const char *key);

const char *
get_string (const char * key);

/**
 * Stores given value in given key
 */
void
store_value (char     * key,
             GVariant * value);

#endif
