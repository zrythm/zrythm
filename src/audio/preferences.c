/*
 * inc/audio/preferences.c - Preferences
 *
 * Copyright (C) 2019 Alexandros Theodotou
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/preferences.h"
#include "settings.h"

/**
 * Initializes preferences with values from settings.
 */
Preferences *
preferences_new (Settings * settings)
{
  Preferences * self = calloc (1, sizeof (Preferences));

  self->settings = settings;

  self->audio_backend =
    g_settings_get_int (settings->preferences,
                      "audio-backend");
  self->edit_region_size =
    g_settings_get_int (settings->preferences,
                      "audio-backend");

  return self;
}

void
preferences_set_audio_backend (Preferences * self,
                               EngineBackend backend)
{
  if (self->audio_backend == backend)
    return;

  /* save preferences value */
  self->audio_backend = backend;

  /* save gsettings value */
  g_settings_set_int (self->settings->preferences,
                      "audio-backend",
                      backend);

  /* update actual thing */
  /* not needed here, only takes effect on restart */

  /* activate corresponding action if necessary */
  /* not needed here, takes effect on restart */
}

void
preferences_set_edit_region_size (Preferences * self,
                                  int           region_size)
{
  if (self->edit_region_size == region_size)
    return;

  /* save preferences value */
  self->edit_region_size = region_size;

  /* save gsettings value */
  g_settings_set_int (self->settings->preferences,
                      "edit-region-size",
                      region_size);

  /* update actual thing */
  /* not needed here, used directly from preferences */

  /* activate corresponding action if necessary */
  /* not needed here, value will be used from now on */
}
