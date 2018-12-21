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

#include "settings.h"
#include "zrythm.h"

Settings *
settings_new ()
{
  Settings * self = calloc (1, sizeof (Settings));

  self->gsettings =
    g_settings_new ("org.zrythm");

  return self;
}

void
settings_init (Settings * settings)
{
}

/**
 * Returns the value for the given key
 */
GVariant *
settings_get_value (Settings * self,
                    const char * key)
{
  return g_settings_get_value (self->gsettings,
                               key);
}

int
settings_get_int (Settings * self,
                  const char * key)
{
  return g_variant_get_int32 (
    settings_get_value (self, key));
}

const char *
settings_get_string (Settings * self,
                     const char * key)
{
  return g_variant_get_string (
    settings_get_value (self, key),
    NULL);
}

/**
 * Stores given value in given key
 */
void
settings_store_value (Settings * self,
                      char     * key,
                      GVariant * value)
{

}
