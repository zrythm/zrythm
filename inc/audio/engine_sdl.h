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

#include "zrythm-config.h"

#ifdef HAVE_SDL

#  ifndef __AUDIO_ENGINE_SDL_H__
#    define __AUDIO_ENGINE_SDL_H__

typedef struct AudioEngine AudioEngine;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Set up Port Audio.
 */
int
engine_sdl_setup (AudioEngine * self);

/**
 * Returns a list of names inside \ref names that
 * must be free'd.
 *
 * @param input 1 for input, 0 for output.
 */
void
engine_sdl_get_device_names (
  AudioEngine * self,
  int           input,
  char **       names,
  int *         num_names);

/**
 * Tests if PortAudio is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_sdl_test (GtkWindow * win);

void
engine_sdl_activate (AudioEngine * self, bool activate);

/**
 * Closes Port Audio.
 */
void
engine_sdl_tear_down (AudioEngine * engine);

/**
 * @}
 */

#  endif
#endif // HAVE_SDL
