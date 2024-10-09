// SPDX-FileCopyrightText: © 2018-2019, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_ENGINE_JACK_H__
#define __AUDIO_ENGINE_JACK_H__

#include "zrythm-config.h"

#include "common/dsp/engine.h"

#if HAVE_JACK

#  include <cstdlib>

#  define JACK_PORT_T(exp) (static_cast<jack_port_t *> (exp))

#  if 0
/**
 * Tests if JACK is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_jack_test (GtkWindow * win);
#  endif

/**
 * Refreshes the list of external ports.
 */
void
engine_jack_rescan_ports (AudioEngine * self);

/**
 * Disconnects and reconnects the monitor output
 * port to the selected devices.
 *
 * @return Whether successful.
 */
bool
engine_jack_reconnect_monitor (AudioEngine * self, bool left, GError ** error);

void
engine_jack_handle_position_change (AudioEngine * self);

void
engine_jack_handle_start (AudioEngine * self);

void
engine_jack_handle_stop (AudioEngine * self);

void
engine_jack_handle_buf_size_change (AudioEngine * self, uint32_t frames);

void
engine_jack_handle_sample_rate_change (AudioEngine * self, uint32_t samplerate);

/**
 * Prepares for processing.
 *
 * Called at the start of each process cycle.
 */
void
engine_jack_prepare_process (AudioEngine * self);

/**
 * Updates the JACK Transport type.
 */
void
engine_jack_set_transport_type (
  AudioEngine *                  self,
  AudioEngine::JackTransportType type);

/**
 * Fills the external out bufs.
 */
void
engine_jack_fill_out_bufs (AudioEngine * self, const nframes_t nframes);

/**
 * Sets up the MIDI engine to use jack.
 *
 * @param loading Loading a Project or not.
 */
int
engine_jack_midi_setup (AudioEngine * self);

/**
 * Sets up the audio engine to use jack.
 *
 * @param loading Loading a Project or not.
 */
int
engine_jack_setup (AudioEngine * self);

std::string
engine_jack_get_error_message (jack_status_t status);

void
engine_jack_tear_down (AudioEngine * self);

int
engine_jack_activate (AudioEngine * self, bool activate);

/**
 * Returns the JACK type string.
 */
const char *
engine_jack_get_jack_type (PortType type);

/** Jack buffer size callback. */
int
engine_jack_buffer_size_cb (uint32_t nframes, AudioEngine * self);

/**
 * Returns if this is a pipewire session.
 */
bool
engine_jack_is_pipewire (AudioEngine * self);

#endif /* HAVE_JACK */
#endif /* header guard */
