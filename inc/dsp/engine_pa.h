/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifdef HAVE_PORT_AUDIO

#  ifndef __AUDIO_ENGINE_PA_H__
#    define __AUDIO_ENGINE_PA_H__

#    include <portaudio.h>

typedef struct AudioEngine AudioEngine;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Set up Port Audio.
 */
int
engine_pa_setup (AudioEngine * self);

void
engine_pa_fill_out_bufs (
  AudioEngine *   self,
  const nframes_t nframes);

/**
 * Tests if PortAudio is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_pa_test (GtkWindow * win);

/**
 * Closes Port Audio.
 */
void
engine_pa_tear_down (AudioEngine * engine);

/**
 * @}
 */

#  endif
#endif // HAVE_PORT_AUDIO
