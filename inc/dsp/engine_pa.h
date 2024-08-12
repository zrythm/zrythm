// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#ifdef HAVE_PORT_AUDIO

#  ifndef __AUDIO_ENGINE_PA_H__
#    define __AUDIO_ENGINE_PA_H__

#    include <portaudio.h>

class AudioEngine;

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
engine_pa_fill_out_bufs (AudioEngine * self, const nframes_t nframes);

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
