/*
 * settings.c - application settings
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

#include <stdlib.h>

#include "settings_manager.h"
#include "zrythm_app.h"

static GSettings * gsettings;

void
init_settings_manager ()
{
  g_message ("Initializing settings...");
  gsettings =
    g_settings_new ("online.alextee.zrythm");

  Settings_Manager * settings_manager =
    calloc (1, sizeof (Settings_Manager));

  settings_manager->gsettings = gsettings;

  zrythm_app->settings_manager = settings_manager;
}

/**
 * Returns the value for the given key
 */
GVariant *
get_value (const char * key)
{
  return g_settings_get_value (gsettings,
                               key);
}

int
get_int (const char * key)
{
  return g_variant_get_int32 (get_value (key));
}

const char *
get_string (const char * key)
{
  return g_variant_get_string (get_value (key),
                               NULL);
}

/**
 * Stores given value in given key
 */
void
store_value (char     * key,
             GVariant * value)
{

}
