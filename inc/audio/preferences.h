/*
 * inc/audio/preferences.h - Preferences
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

#ifndef __AUDIO_PREFERENCES_H__
#define __AUDIO_PREFERENCES_H__

#include "audio/engine.h"

#define PREFERENCES ZRYTHM->preferences

typedef struct Preferences
{
  /* Audio tab */
  EngineBackend       audio_backend;

  /* Editing tab */
  int                 edit_region_size;

  Settings *          settings;
} Preferences;

/**
 * Initializes preferences with values from settings.
 */
Preferences *
preferences_new (Settings * settings);

void
preferences_set_audio_backend (Preferences * self,
                               EngineBackend backend);

void
preferences_set_edit_region_size (Preferences * self,
                                  int           region_size);

#endif
