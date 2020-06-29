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

#include "zrythm-config.h"

#include "audio/control_room.h"
#include "audio/ext_port.h"
#include "audio/pan.h"
#include "audio/pool.h"
#include "audio/sample_processor.h"
#include "audio/transport.h"
#include "utils/types.h"
#include "zix/sem.h"

#ifdef HAVE_JACK
#include "weak_libjack.h"
#endif

#ifdef HAVE_PORT_AUDIO
#include <portaudio.h>
#endif

#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>
#endif

#ifdef HAVE_SDL
#include <SDL2/SDL_audio.h>
#endif

#ifdef HAVE_RTAUDIO
#include <rtaudio/rtaudio_c.h>
#endif

typedef struct StereoPorts StereoPorts;
typedef struct Port Port;
typedef struct Channel Channel;
typedef struct Plugin Plugin;
typedef struct Tracklist Tracklist;
typedef struct ExtPort ExtPort;
typedef struct MidiMappings MidiMappings;
typedef struct WindowsMmeDevice WindowsMmeDevice;
typedef struct Router Router;
typedef struct Metronome Metronome;
typedef struct Project Project;

/**
 * @addtogroup audio Audio
 *
 * @{
 */

#define BLOCK_LENGTH 4096 // should be set by backend
#define MIDI_BUF_SIZE 1024 // should be set by backend

#define MIDI_IN_NUM_EVENTS \
  AUDIO_ENGINE->midi_in->midi_events->num_events


#define AUDIO_ENGINE (PROJECT->audio_engine)
#define MANUAL_PRESS_EVENTS \
  (AUDIO_ENGINE->midi_editor_manual_press-> \
  midi_events)

#define DENORMAL_PREVENTION_VAL \
  (AUDIO_ENGINE->denormal_prevention_val)

/** Set whether engine should process (true) or
 * skip (false). */
#define engine_set_run(engine,_run) \
  g_atomic_int_set (&engine->run, _run)

/**
 * Buffer sizes to be used in combo boxes.
 */
typedef enum AudioEngineBufferSize
{
  AUDIO_ENGINE_BUFFER_SIZE_16,
  AUDIO_ENGINE_BUFFER_SIZE_32,
  AUDIO_ENGINE_BUFFER_SIZE_64,
  AUDIO_ENGINE_BUFFER_SIZE_128,
  AUDIO_ENGINE_BUFFER_SIZE_256,
  AUDIO_ENGINE_BUFFER_SIZE_512,
  AUDIO_ENGINE_BUFFER_SIZE_1024,
  AUDIO_ENGINE_BUFFER_SIZE_2048,
  AUDIO_ENGINE_BUFFER_SIZE_4096,
  NUM_AUDIO_ENGINE_BUFFER_SIZES,
} AudioEngineBufferSize;

static const char * buffer_size_str[] =
{
  "16",
  "32",
  "64",
  "128",
  "256",
  "512",
  __("1,024"),
  __("2,048"),
  __("4,096"),
};

static inline const char *
engine_buffer_size_to_string (
  AudioEngineBufferSize buf_size)
{
  return buffer_size_str[buf_size];
}

/**
 * Samplerates to be used in comboboxes.
 */
typedef enum AudioEngineSamplerate
{
  AUDIO_ENGINE_SAMPLERATE_22050,
  AUDIO_ENGINE_SAMPLERATE_32000,
  AUDIO_ENGINE_SAMPLERATE_44100,
  AUDIO_ENGINE_SAMPLERATE_48000,
  AUDIO_ENGINE_SAMPLERATE_88200,
  AUDIO_ENGINE_SAMPLERATE_96000,
  AUDIO_ENGINE_SAMPLERATE_192000,
  NUM_AUDIO_ENGINE_SAMPLERATES,
} AudioEngineSamplerate;

static const char * sample_rate_str[] =
{
  __("22,050"),
  __("32,000"),
  __("44,100"),
  __("48,000"),
  __("88,200"),
  __("96,000"),
  __("192,000"),
};

static inline const char *
engine_sample_rate_to_string (
  AudioEngineSamplerate sample_rate)
{
  return sample_rate_str[sample_rate];
}

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
  AUDIO_BACKEND_SDL,
  AUDIO_BACKEND_RTAUDIO,
  NUM_AUDIO_BACKENDS,
} AudioBackend;

static const char * audio_backend_str[] =
{
  /* TRANSLATORS: Dummy backend */
  __("Dummy"),
  "ALSA",
  "JACK",
  "PortAudio",
  "SDL",
  "RtAudio",
};

/**
 * Mode used when bouncing, either during exporting
 * or when bouncing tracks or regions to audio.
 */
typedef enum BounceMode
{
  /** Don't bounce. */
  BOUNCE_OFF,

  /** Bounce. */
  BOUNCE_ON,

  /**
   * Bounce if parent is bouncing.
   *
   * To be used on regions to bounce if their track
   * is bouncing.
   */
  BOUNCE_INHERIT,
} BounceMode;

typedef enum MidiBackend
{
  MIDI_BACKEND_DUMMY,
  MIDI_BACKEND_ALSA,
  MIDI_BACKEND_JACK,
  MIDI_BACKEND_WINDOWS_MME,
  MIDI_BACKEND_RTMIDI,
  NUM_MIDI_BACKENDS,
} MidiBackend;

static const char * midi_backend_str[] =
{
  /* TRANSLATORS: Dummy backend */
  __("Dummy"),
  __("ALSA Sequencer"),
  "JACK MIDI",
  "Windows MME",
  "RtMidi",
};

typedef enum AudioEngineJackTransportType
{
  AUDIO_ENGINE_JACK_TIMEBASE_MASTER,
  AUDIO_ENGINE_JACK_TRANSPORT_CLIENT,
  AUDIO_ENGINE_NO_JACK_TRANSPORT,
} AudioEngineJackTransportType;

static const cyaml_strval_t
jack_transport_type_strings[] =
{
  { "Timebase master",
    AUDIO_ENGINE_JACK_TIMEBASE_MASTER    },
  { "Transport client",
    AUDIO_ENGINE_JACK_TRANSPORT_CLIENT    },
  { "No JACK transport",
    AUDIO_ENGINE_NO_JACK_TRANSPORT    },
};

/**
 * The audio engine.
 */
typedef struct AudioEngine
{
  /**
   * Cycle count to know which cycle we are in.
   *
   * Useful for debugging.
   */
  volatile long     cycle;

#ifdef HAVE_JACK
  /** JACK client. */
  jack_client_t *   client;
#endif

  /**
   * Whether transport master/client or no
   * connection with jack transport.
   */
  AudioEngineJackTransportType transport_type;

  /** Current audio backend. */
  AudioBackend      audio_backend;

  /** Current MIDI backend. */
  MidiBackend       midi_backend;

  /** Audio buffer size (block length). */
  nframes_t         block_length;

  /** Size of MIDI port buffers. */
  size_t            midi_buf_size;

  /** Sample rate. */
  sample_rate_t     sample_rate;

  /** Number of frames/samples per tick. */
  double            frames_per_tick;

  /** True iff buffer size callback fired. */
  int               buf_size_set;

  /** The processing graph router. */
  Router *          router;

  /**
   * Audio intefrace outputs (only 2 are used).
   *
   * These should be initialized at the start and
   * only used at runtime for getting the buffers.
   *
   * They should eventually be stored in GSettings
   * since they are user settings and not project
   * related.
   */
  ExtPort *         hw_stereo_outs[EXT_PORTS_MAX];
  int               num_hw_stereo_outs;

  /**
   * Audio intefrace inputs (only 2 are used).
   *
   * These should be initialized at the start and
   * only used at runtime for getting the buffers.
   *
   * They should eventually be stored in GSettings
   * since they are user settings and not project
   * related.
   *
   * They should only be used in audio tracks as
   * default recording input.
   */
  ExtPort *         hw_stereo_ins[EXT_PORTS_MAX];
  int               num_hw_stereo_ins;

  /** MIDI Clock in TODO. */
  Port *            midi_clock_in;

  /** The ControlRoom. */
  ControlRoom *     control_room;

  /** Audio file pool. */
  AudioPool *       pool;

  /**
   * Monitor - these should be the last ports in
   * the signal chain.
   */
  StereoPorts *     monitor_out;

  /**
   * Flag to tell the UI that this channel had
   * MIDI activity.
   *
   * When processing this and setting it to 0,
   * the UI should create a separate event using
   * EVENTS_PUSH.
   */
  int               trigger_midi_activity;

  /**
   * Manual note press in the piano roll.
   *
   * This should route to the post MIDI in of the
   * applicable channel.
   */
  Port *            midi_editor_manual_press;

  /**
   * Port used for receiving MIDI in messages for
   * binding CC and other non-recording purposes.
   */
  Port *            midi_in;

  /** Number of frames/samples in the current
   * cycle. */
  nframes_t         nframes;

  /** Semaphore for blocking DSP while a plugin and
   * its ports are deleted. */
  ZixSem            port_operation_lock;

  /** Ok to process or not. */
  volatile gint     run;

  /** 1 if currently exporting. */
  gint              exporting;

  /** Skip mid-cycle. */
  gint              skip_cycle;

  /** Send note off MIDI everywhere. */
  volatile gint     panic;

  //ZixSem             alsa_callback_start;

  /* ----------- ALSA --------------- */
#ifdef HAVE_ALSA
  /** Alsa playback handle. */
  snd_pcm_t *       playback_handle;
  snd_seq_t *       seq_handle;
  snd_pcm_hw_params_t * hw_params;
  snd_pcm_sw_params_t * sw_params;
  /** ALSA audio buffer. */
  float *           alsa_out_buf;

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
  float *           pa_out_buf;

  PaStream *        pa_stream;
#endif

#ifdef _WOE32
  /** Windows MME MIDI devices. */
  WindowsMmeDevice * mme_in_devs[1024];
  int                num_mme_in_devs;
  WindowsMmeDevice * mme_out_devs[1024];
  int                num_mme_out_devs;
#endif

#ifdef HAVE_SDL
  SDL_AudioDeviceID dev;
#endif

#ifdef HAVE_RTAUDIO
  rtaudio_t         rtaudio;
#endif

  /**
   * Dummy audio DSP processing thread.
   */
  GThread *         dummy_audio_thread;

  /** Set to 1 to stop the dummy audio thread. */
  int               stop_dummy_audio_thread;

  /**
   * Timeline metadata like BPM, time signature, etc.
   */
  Transport *       transport;

  /* note: these 2 are ignored at the moment */
  /** Pan law. */
  PanLaw            pan_law;
  /** Pan algorithm */
  PanAlgorithm      pan_algo;

  /** Time taken to process in the last cycle */
  gint64            last_time_taken;

  /** Max time taken to process in the last few
   * cycles. */
  gint64            max_time_taken;

  /** Timestamp at the start of the current
   * cycle. */
  gint64            timestamp_start;

  /** Expected timestamp at the end of the current
   * cycle. */
  gint64            timestamp_end;

  /** When first set, it is equal to the max
   * playback latency of all initial trigger
   * nodes. */
  nframes_t         remaining_latency_preroll;

  SampleProcessor * sample_processor;

  /** To be set to 1 when the CC from the Midi in
   * port should be captured. */
  int               capture_cc;

  /** Last MIDI CC captured. */
  midi_byte_t       last_cc[3];

  /**
   * Last time an XRUN notification was shown.
   *
   * This is to prevent multiple XRUN notifications
   * being shown so quickly that Zrythm becomes
   * unusable.
   */
  gint64            last_xrun_notification;

  /**
   * Whether the denormal prevention value
   * (1e-12 ~ 1e-20) is positive.
   *
   * This should be swapped often to avoid DC offset
   * prevention algorithms removing it.
   *
   * See https://www.earlevel.com/main/2019/04/19/floating-point-denormals/ for details.
   */
  bool              denormal_prevention_val_positive;
  float             denormal_prevention_val;

#ifdef TRIAL_VER
  /** Time at start to keep track if trial limit
   * is reached. */
  gint64            zrythm_start_time;

  /** Flag to keep track of the first time the
   * limit is reached. */
  int               limit_reached;
#endif

  /**
   * If this is on, only tracks/regions marked as
   * "for bounce" will be allowed to make sound.
   *
   * Automation and everything else will work as
   * normal.
   */
  BounceMode        bounce_mode;

  /** The metronome. */
  Metronome *       metronome;

  /** Whether the engine is already set up. */
  bool              setup;

  /** Whether the engine is currently activated. */
  bool              activated;

} AudioEngine;

static const cyaml_schema_field_t
engine_fields_schema[] =
{
  CYAML_FIELD_ENUM (
    "transport_type", CYAML_FLAG_DEFAULT,
    AudioEngine, transport_type,
    jack_transport_type_strings,
    CYAML_ARRAY_LEN (jack_transport_type_strings)),
  CYAML_FIELD_INT (
    "sample_rate", CYAML_FLAG_DEFAULT,
    AudioEngine, sample_rate),
  CYAML_FIELD_FLOAT (
    "frames_per_tick", CYAML_FLAG_DEFAULT,
    AudioEngine, frames_per_tick),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine, monitor_out,
    stereo_ports_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine, midi_editor_manual_press,
    port_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine, midi_in,
    port_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine, transport,
    transport_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine, pool,
    audio_pool_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine, control_room,
    control_room_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine, sample_processor,
    sample_processor_fields_schema),

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
  nframes_t     buf_size);

void
engine_init_loaded (
  AudioEngine * self);

/**
 * Create a new audio engine.
 *
 * This only initializes the engine and doe snot
 * connect to the backend.
 */
AudioEngine *
engine_new (
  Project * project);

/**
 * Set up the audio engine, ie, connect to the
 * backend but does not start processing yet.
 */
void
engine_setup (
  AudioEngine * self);

/**
 * Activates the audio engine to start processing
 * and receiving events.
 *
 * @param activate Activate or deactivate.
 */
void
engine_activate (
  AudioEngine * self,
  bool          activate);

/**
 * Updates frames per tick based on the time sig,
 * the BPM, and the sample rate
 */
void
engine_update_frames_per_tick (
  AudioEngine *       self,
  const int           beats_per_bar,
  const float         bpm,
  const sample_rate_t sample_rate);

/**
 * To be called by each implementation to prepare the
 * structures before processing.
 *
 * Clears buffers, marks all as unprocessed, etc.
 */
void
engine_process_prepare (
  AudioEngine * self,
  nframes_t     nframes);

/**
 * Processes current cycle.
 *
 * To be called by each implementation in its
 * callback.
 */
int
engine_process (
  AudioEngine * self,
  nframes_t     nframes);

/**
 * To be called after processing for common logic.
 */
void
engine_post_process (
  AudioEngine * self,
  const nframes_t nframes);

/**
 * Called to fill in the external buffers at the end
 * of the processing cycle.
 */
void
engine_fill_out_bufs (
  AudioEngine *   self,
  const nframes_t nframes);

/**
 * Returns the int value correesponding to the
 * given AudioEngineBufferSize.
 */
int
engine_buffer_size_enum_to_int (
  AudioEngineBufferSize buffer_size);

/**
 * Returns the int value correesponding to the
 * given AudioEngineSamplerate.
 */
int
engine_samplerate_enum_to_int (
  AudioEngineSamplerate samplerate);

/**
 * Returns 1 if the port is an engine port or
 * control room port, otherwise 0.
 */
#define engine_is_port_own(self,port) \
  (port == MONITOR_FADER->stereo_in->l || \
  port == MONITOR_FADER->stereo_in->r || \
  port == MONITOR_FADER->stereo_out->l || \
  port == MONITOR_FADER->stereo_out->r)

/**
 * Returns the audio backend as a string.
 */
static inline const char *
engine_audio_backend_to_string (
  AudioBackend backend)
{
  return audio_backend_str[backend];
}

static inline AudioBackend
engine_audio_backend_from_string (
  char * str)
{
  for (int i = 0; i < NUM_AUDIO_BACKENDS; i++)
    {
      if (string_is_equal (
            audio_backend_str[i], str, true))
        {
          return (AudioBackend) i;
        }
    }

  if (string_is_equal (str, "none", true))
    {
      return AUDIO_BACKEND_DUMMY;
    }

  g_return_val_if_reached (AUDIO_BACKEND_DUMMY);
}

/**
 * Returns the MIDI backend as a string.
 */
static inline const char *
engine_midi_backend_to_string (
  MidiBackend backend)
{
  return midi_backend_str[backend];
}

static inline MidiBackend
engine_midi_backend_from_string (
  char * str)
{
  for (int i = 0; i < NUM_MIDI_BACKENDS; i++)
    {
      if (string_is_equal (
            midi_backend_str[i], str, true))
        {
          return (MidiBackend) i;
        }
    }

  if (string_is_equal (str, "none", true))
    {
      return MIDI_BACKEND_DUMMY;
    }
  else if (string_is_equal (str, "jack", true))
    {
      return MIDI_BACKEND_JACK;
    }

  g_return_val_if_reached (MIDI_BACKEND_DUMMY);
}

/**
 * Reset the bounce mode on the engine, all tracks
 * and regions to OFF.
 */
void
engine_reset_bounce_mode (
  AudioEngine * self);

/**
 * Closes any connections and free's data.
 */
void
engine_free (
  AudioEngine * self);

/**
 * @}
 */

#endif
