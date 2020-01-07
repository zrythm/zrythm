/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "config.h"

#ifdef _WIN32

#ifndef __AUDIO_ENGINE_WINDOWS_MME_H__
#define __AUDIO_ENGINE_WINDOWS_MME_H__

#include <Windows.h>
#include <mmeapi.h>

typedef struct AudioEngine AudioEngine;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Initialize Port MIDI.
 */
int
engine_windows_mme_setup (
  AudioEngine * self,
  int           loading);

int
engine_windows_mme_tear_down (
  AudioEngine * self);

/**
 * Tests if the engine is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_windows_mme_test (
  GtkWindow * win);

/**
 * @}
 */

#endif
#endif // _WIN32
