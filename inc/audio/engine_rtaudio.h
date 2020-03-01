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

#ifdef HAVE_RTAUDIO

#ifndef __AUDIO_ENGINE_RTAUDIO_H__
#define __AUDIO_ENGINE_RTAUDIO_H__

#include <rtaudio/rtaudio_c.h>

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
engine_rtaudio_setup (
  AudioEngine * self,
  int           loading);

void
engine_rtaudio_fill_out_bufs (
  AudioEngine *   self,
  const nframes_t nframes);

void
engine_rtaudio_activate (
  AudioEngine * self);

/**
 * Tests if the backend is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_rtaudio_test (
  GtkWindow * win);

/**
 * Closes the engine.
 */
void
engine_rtaudio_tear_down (
  AudioEngine * engine);

/**
 * @}
 */

#endif
#endif // HAVE_RTAUDIO
