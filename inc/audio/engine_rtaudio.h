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

#ifndef __AUDIO_ENGINE_RTAUDIO_H__
#define __AUDIO_ENGINE_RTAUDIO_H__

#include "zrythm-config.h"

#ifdef HAVE_RTAUDIO

#  include <rtaudio_c.h>

typedef struct AudioEngine AudioEngine;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Set up the engine.
 */
int
engine_rtaudio_setup (AudioEngine * self);

void
engine_rtaudio_activate (AudioEngine * self, bool activate);

rtaudio_t
engine_rtaudio_create_rtaudio (
  AudioEngine * self,
  AudioBackend  backend);

/**
 * Returns a list of names inside \ref names that
 * must be free'd.
 *
 * @param input 1 for input, 0 for output.
 */
void
engine_rtaudio_get_device_names (
  AudioEngine * self,
  AudioBackend  backend,
  int           input,
  char **       names,
  int *         num_names);

/**
 * Tests if the backend is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_rtaudio_test (GtkWindow * win);

/**
 * Closes the engine.
 */
void
engine_rtaudio_tear_down (AudioEngine * engine);

/**
 * @}
 */

#endif // HAVE_RTAUDIO
#endif
