/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * The audio engine.
 */

#ifndef __AUDIO_ENGINE_H__
#define __AUDIO_ENGINE_H__

#include "config.h"
#include "audio/mixer.h"
#include "audio/pan.h"
#include "audio/sample_processor.h"
#include "audio/transport.h"
#include "zix/sem.h"

#ifdef HAVE_JACK
#include <jack/jack.h>
#include <jack/midiport.h>
#endif

#ifdef HAVE_PORT_AUDIO
#include <portaudio.h>
#endif

#ifdef __linux__
#include <alsa/asoundlib.h>
#endif

typedef struct StereoPorts StereoPorts;
typedef struct Port Port;
typedef struct Channel Channel;
typedef struct Plugin Plugin;
typedef struct Tracklist Tracklist;

/**
 * @defgroup audio Audio
 *
 * The audio module contains DSP and audio related
 * code.
 *
 * @{
 */

#define BLOCK_LENGTH 4096 // should be set by backend
#define MIDI_BUF_SIZE 1024 // should be set by backend

#define MIDI_IN_NUM_EVENTS \
  AUDIO_ENGINE->midi_in->midi_events->num_events


#define AUDIO_ENGINE (&PROJECT->audio_engine)
#define MANUAL_PRESS_EVENTS \
  (AUDIO_ENGINE->midi_editor_manual_press-> \
  midi_events)

#ifdef HAVE_JACK
typedef jack_nframes_t                nframes_t;
#endif

//typedef struct MIDI_Controller
//{
  //jack_midi_event_t    in_event[30];
  //int                  num_events;
//} MIDI_Controller;

typedef enum AudioBackend
{
  AUDIO_BACKEND_DUMMY,
  AUDIO_BACKEND_ALSA,
  AUDIO_BACKEND_JACK,
  AUDIO_BACKEND_PORT_AUDIO,
  NUM_AUDIO_BACKENDS,
} AudioBackend;

typedef enum MidiBackend
{
  MIDI_BACKEND_DUMMY,
  MIDI_BACKEND_ALSA,
  MIDI_BACKEND_JACK,
  NUM_MIDI_BACKENDS,
} MidiBackend;

typedef enum AudioEngineJackTransportType
{
  AUDIO_ENGINE_JACK_TIMEBASE_MASTER,
  AUDIO_ENGINE_JACK_TRANSPORT_CLIENT,
  AUDIO_ENGINE_NO_JACK_TRANSPORT,
} AudioEngineJackTransportType;

typedef struct AudioEngine
{
  /**
   * Cycle count to know which cycle we are in.
   *
   * Useful for debugging.
   */
  long    cycle;

#ifdef HAVE_JACK
  jack_client_t     * client;     ///< jack client

  /**
   * Whether transport master/client or no
   * connection with jack transport.
   */
  AudioEngineJackTransportType transport_type;
#endif

  /** Current audio backend. */
  AudioBackend       audio_backend;

  /** Current MIDI backend. */
  MidiBackend        midi_backend;
	uint32_t           block_length;   ///< Audio buffer size (block length)
	size_t             midi_buf_size;  ///< Size of MIDI port buffers
	uint32_t           sample_rate;    ///< Sample rate
  int               frames_per_tick;  ///< number of frames per tick
	int               buf_size_set;   ///< True iff buffer size callback fired
  Mixer              mixer;        ///< the mixer

  /**
   * To be serialized instead of StereoPorts.
   */

  /** stereo in ports from the backend. */
  StereoPorts       * stereo_in;

  /** stereo out ports to the backend. */
  StereoPorts       * stereo_out;

  /** MIDI in port from the audio engine. */
  Port              * midi_in;

  /** MIDI out port from the audio engine. */
  Port              * midi_out;

  /**
   * Flag to tell the UI that this channel had
   * MIDI activity.
   *
   * When processing this and setting it to 0,
   * the UI should create a separate event using
   * EVENTS_PUSH.
   */
  int                  trigger_midi_activity;

  /** Manual note prress in the piano roll. */
  Port              * midi_editor_manual_press;

  /** Number of frames/samples in the current
   * cycle. */
  uint32_t          nframes;

  /** Semaphore for blocking DSP while a plugin and
   * its ports are deleted. */
  ZixSem            port_operation_lock;

  /** Ok to process or not. */
  gint               run;

  /** 1 if currently exporting. */
  gint               exporting;

  /** Skip mid-cycle. */
  gint               skip_cycle;

  /** Send note off MIDI everywhere. */
  gint               panic;

  //ZixSem             alsa_callback_start;

  /* ----------- ALSA --------------- */
#ifdef __linux__
  /** Alsa playback handle. */
  snd_pcm_t *        playback_handle;
  snd_seq_t *        seq_handle;
  snd_pcm_hw_params_t * hw_params;
  snd_pcm_sw_params_t * sw_params;
  /** ALSA audio buffer. */
  float *            alsa_out_buf;

  /**
   * Since ALSA MIDI runs in its own thread,
   * store the events here temporarily and
   * pop them in the process cycle.
   *
   * Needs to be protected by some kind of
   * mutex.
   */
  //MidiEvents         alsa_midi_events;

  /** Semaphore for exclusively writing/reading
   * ALSA MIDI events from above. */
  //ZixSem             alsa_midi_events_sem;
#endif

  /* ------------------------------- */


  /** Flag used when processing in some backends. */
  volatile gint     filled_stereo_out_bufs;

#ifdef HAVE_PORT_AUDIO
  /**
   * Port Audio output buffer.
   *
   * Unlike JACK, the audio goes directly here.
   * FIXME this is not really needed, just
   * do the calculations in pa_stream_cb.
   */
  float *            pa_out_buf;

  PaStream *         pa_stream;
#endif

  /**
   * Timeline metadata like BPM, time signature, etc.
   */
  Transport         transport;

  PanLaw            pan_law;

  PanAlgorithm      pan_algo;

  gint64            last_time_taken;

  gint64            max_time_taken;

  /** When first set, it is equal to the max
   * playback latency of all initial trigger
   * nodes. */
  long              remaining_latency_preroll;

  SampleProcessor   sample_processor;

} AudioEngine;

static const cyaml_schema_field_t
engine_fields_schema[] =
{
  CYAML_FIELD_INT (
    "sample_rate", CYAML_FLAG_DEFAULT,
    AudioEngine, sample_rate),
  CYAML_FIELD_INT (
    "frames_per_tick", CYAML_FLAG_DEFAULT,
    AudioEngine, frames_per_tick),
  CYAML_FIELD_MAPPING (
    "mixer", CYAML_FLAG_DEFAULT,
    AudioEngine, mixer,
    mixer_fields_schema),
	CYAML_FIELD_MAPPING_PTR (
    "stereo_in", CYAML_FLAG_POINTER,
    AudioEngine, stereo_in,
    stereo_ports_fields_schema),
	CYAML_FIELD_MAPPING_PTR (
    "stereo_out", CYAML_FLAG_POINTER,
    AudioEngine, stereo_out,
    stereo_ports_fields_schema),
	CYAML_FIELD_MAPPING_PTR (
    "midi_in", CYAML_FLAG_POINTER,
    AudioEngine, midi_in,
    port_fields_schema),
	CYAML_FIELD_MAPPING_PTR (
    "midi_editor_manual_press", CYAML_FLAG_POINTER,
    AudioEngine, midi_editor_manual_press,
    port_fields_schema),
  CYAML_FIELD_MAPPING (
    "transport", CYAML_FLAG_DEFAULT,
    AudioEngine, transport,
    transport_fields_schema),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
engine_schema =
{
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
	  AudioEngine, engine_fields_schema),
};

void
engine_realloc_port_buffers (
  AudioEngine * self,
  int           buf_size);

/**
 * Init audio engine.
 *
 * loading is 1 if loading a project.
 */
void
engine_init (AudioEngine * self,
             int           loading);

/**
 * Updates frames per tick based on the time sig, the BPM,
 * and the sample rate
 */
void
engine_update_frames_per_tick (int beats_per_bar,
                               int bpm,
                               int sample_rate);

/**
 * To be called by each implementation to prepare the
 * structures before processing.
 *
 * Clears buffers, marks all as unprocessed, etc.
 */
void
engine_process_prepare (
  AudioEngine * self,
  uint32_t      nframes);

/**
 * Processes current cycle.
 *
 * To be called by each implementation in its
 * callback.
 */
int
engine_process (
  AudioEngine * self,
  uint32_t      nframes);

/**
 * To be called after processing for common logic.
 */
void
engine_post_process ();

/**
 * Closes any connections and free's data.
 */
void
engine_tear_down ();

/**
 * @}
 */

#endif
