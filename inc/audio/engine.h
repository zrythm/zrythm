/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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
#include "audio/transport.h"
#include "utils/sem.h"

#ifdef HAVE_JACK
#include <jack/jack.h>
#include <jack/midiport.h>
#endif

#ifdef HAVE_PORT_AUDIO
#include <portaudio.h>
#endif

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


#define AUDIO_ENGINE (&PROJECT->audio_engine)
#define MANUAL_PRESS_QUEUE \
  (AUDIO_ENGINE->midi_editor_manual_press-> \
  midi_events->queue)
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
typedef struct StereoPorts StereoPorts;
typedef struct Port Port;
typedef struct Channel Channel;
typedef struct Plugin Plugin;
typedef struct Tracklist Tracklist;

typedef enum AudioBackend
{
  AUDIO_BACKEND_DUMMY,
  AUDIO_BACKEND_JACK,
  AUDIO_BACKEND_ALSA,
  AUDIO_BACKEND_PORT_AUDIO,
  NUM_AUDIO_BACKENDS,
} AudioBackend;

typedef enum MidiBackend
{
  MIDI_BACKEND_DUMMY,
  MIDI_BACKEND_JACK,
  NUM_MIDI_BACKENDS,
} MidiBackend;

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
  int               midi_in_id;
  int               midi_editor_manual_press_id;;

  /** stereo in ports from the backend. */
  StereoPorts       * stereo_in;

  /** stereo out ports to the backend. */
  StereoPorts       * stereo_out;
  Port              * midi_in;     ///< MIDI in port from JACK
  Port              * midi_editor_manual_press; ///< manual note press in editor
  uint32_t          nframes;     ///< nframes for current cycle
  //MIDI_Controller    * midi_controller; ///< the midi input on JACK
  //Port_Manager      * port_manager;  ///< manages all ports created for/by plugins
  ZixSem            port_operation_lock;  ///< semaphore for blocking DSP while plugin and its ports are deleted

  /** Ok to process or not. */
  gint               run;

  /** 1 if currently exporting. */
  gint               exporting;

  /** Skip mid-cycle. */
  gint               skip_cycle;

  /** Send note off MIDI everywhere. */
  gint               panic;

  /**
   * Port buffer for raw MIDI data.
   */
  void *            port_buf;

  /** Flag used when processing in some backends. */
  volatile gint     filled_stereo_out_bufs;

#ifdef HAVE_PORT_AUDIO
  /**
   * Port Audio output buffer.
   *
   * Unlike JACK, the audio goes directly here.
   */
  float *            pa_out_buf;

  PaStream *         pa_stream;
#endif

  /**
   * Timeline metadata like BPM, time signature, etc.
   */
  Transport         transport;

  gint64            last_time_taken;

  gint64            max_time_taken;

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
  CYAML_FIELD_INT (
    "midi_in_id", CYAML_FLAG_DEFAULT,
    AudioEngine, midi_in_id),
  CYAML_FIELD_INT (
    "midi_editor_manual_press_id", CYAML_FLAG_DEFAULT,
    AudioEngine, midi_editor_manual_press_id),
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
engine_process_prepare (uint32_t      nframes);

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
