// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
