// SPDX-FileCopyrightText: © 2018-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_ENGINE_RTMIDI_H__
#define __AUDIO_ENGINE_RTMIDI_H__

#include "zrythm-config.h"

#if HAVE_RTMIDI

class AudioEngine;

#  if 0
/**
 * Tests if the backend is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_rtmidi_test (GtkWindow * win);
#  endif

/**
 * Refreshes the list of external ports.
 */
void
engine_rtmidi_rescan_ports (AudioEngine * self);

/**
 * Sets up the MIDI engine to use Rtmidi.
 *
 * @param loading Loading a Project or not.
 */
int
engine_rtmidi_setup (AudioEngine * self);

/**
 * Gets the number of input ports (devices).
 */
unsigned int
engine_rtmidi_get_num_in_ports (AudioEngine * self);

/**
 * Creates an input port, optionally opening it with
 * the given device ID (from rtmidi port count) and
 * label.
 */
RtMidiPtr
engine_rtmidi_create_in_port (
  AudioEngine * self,
  int           open_port,
  unsigned int  device_id,
  const char *  label);

void
engine_rtmidi_tear_down (AudioEngine * self);

int
engine_rtmidi_activate (AudioEngine * self, bool activate);

#endif /* HAVE_RTMIDI */
#endif
