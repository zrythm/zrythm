// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#if 0

#  ifdef HAVE_ALSA

#    ifndef __AUDIO_ENGINE_ALSA_H__
#      define __AUDIO_ENGINE_ALSA_H__

#      include <stdlib.h>

#      include "utils/types.h"

typedef struct AudioEngine AudioEngine;

/**
 * Copy the cached MIDI events to the MIDI events
 * in the MIDI in port, used at the start of each
 * cycle. */
void
engine_alsa_receive_midi_events (
  AudioEngine * self,
  int           print);

/**
 * Tests if ALSA works.
 *
 * @param win If window is non-null, it will display
 *   a message to it.
 * @return 0 for OK, non-zero for not ok.
 */
int
engine_alsa_test (
  GtkWindow * win);

int
engine_alsa_midi_setup (
  AudioEngine * self);

/**
 * Prepares for processing.
 *
 * Called at the start of each process cycle.
 */
void
engine_alsa_prepare_process (
  AudioEngine * self);

/**
 * Fill the output buffers at the end of the
 * cycle.
 */
void
engine_alsa_fill_out_bufs (
  AudioEngine *   self,
  const nframes_t nframes);

/**
 * Sets up the audio engine to use alsa.
 */
int
engine_alsa_setup (
  AudioEngine * self,
  int           loading);

void
engine_alsa_tear_down (void);

#    endif // header guard
#  endif   // HAVE_ALSA
#endif
