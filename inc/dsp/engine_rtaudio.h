// SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_ENGINE_RTAUDIO_H__
#define __AUDIO_ENGINE_RTAUDIO_H__

#include "zrythm-config.h"

#ifdef HAVE_RTAUDIO

#  include <rtaudio_c.h>

typedef struct AudioEngine AudioEngine;

/**
 * @addtogroup dsp
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
engine_rtaudio_create_rtaudio (AudioEngine * self, AudioBackend backend);

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
