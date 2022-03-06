/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
 * Copyright (C) 2020 Ryan Gonzalez <rymg19 at gmail dot com>
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
#include "audio/exporter.h"
#include "audio/ext_port.h"
#include "audio/hardware_processor.h"
#include "audio/pan.h"
#include "audio/pool.h"
#include "audio/sample_processor.h"
#include "audio/transport.h"
#include "utils/types.h"
#include "zix/sem.h"

#ifdef HAVE_JACK
#include "weak_libjack.h"
#endif

#ifdef HAVE_PULSEAUDIO
#include <pulse/pulseaudio.h>
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
#include <rtaudio_c.h>
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
typedef struct HardwareProcessor HardwareProcessor;
typedef struct ObjectPool ObjectPool;
typedef struct MPMCQueue MPMCQueue;

/**
 * @addtogroup audio Audio
 *
 * @{
 */

#define AUDIO_ENGINE_SCHEMA_VERSION 1

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

#define engine_is_in_active_project(self) \
  (self->project == PROJECT)

/** Set whether engine should process (true) or
 * skip (false). */
#define engine_set_run(engine,_run) \
  g_atomic_int_set (&(engine)->run, _run)
#define engine_get_run(engine) \
  g_atomic_int_get (&(engine)->run)

#define ENGINE_MAX_EVENTS 100

#define engine_queue_push_back_event(q,x) \
  mpmc_queue_push_back ( \
    q, (void *) x)

#define engine_queue_dequeue_event(q,x) \
  mpmc_queue_dequeue ( \
    q, (void *) x)

/**
 * Push events.
 */
#define ENGINE_EVENTS_PUSH( \
  et,_arg,_uint_arg,_float_arg) \
  if (true) \
    { \
      AudioEngineEvent * _ev = \
        (AudioEngineEvent *) \
        object_pool_get (AUDIO_ENGINE->ev_pool); \
      _ev->file = __FILE__; \
      _ev->func = __func__; \
      _ev->lineno = __LINE__; \
      _ev->type = et; \
      _ev->arg = (void *) _arg; \
      _ev->uint_arg = _uint_arg; \
      _ev->float_arg = _float_arg; \
      if (zrythm_app->gtk_thread == \
            g_thread_self ()) \
        { \
          _ev->backtrace = \
            backtrace_get ("", 40, false); \
          g_debug ( \
            "pushing engine event " #et \
            " (%s:%d)", __func__, __LINE__); \
        } \
      engine_queue_push_back_event ( \
        AUDIO_ENGINE->ev_queue, _ev); \
    }

/**
 * Audio engine event type.
 */
typedef enum AudioEngineEventType
{
  AUDIO_ENGINE_EVENT_BUFFER_SIZE_CHANGE,
  AUDIO_ENGINE_EVENT_SAMPLE_RATE_CHANGE,
} AudioEngineEventType;

/**
 * Audio engine event.
 */
typedef struct AudioEngineEvent
{
  AudioEngineEventType type;
  void *               arg;
  uint32_t             uint_arg;
  float                float_arg;
  const char *         file;
  const char *         func;
  int                  lineno;
  char *               backtrace;
} AudioEngineEvent;

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
  AUDIO_BACKEND_DUMMY_LIBSOUNDIO,
  AUDIO_BACKEND_ALSA,
  AUDIO_BACKEND_ALSA_LIBSOUNDIO,
  AUDIO_BACKEND_ALSA_RTAUDIO,
  AUDIO_BACKEND_JACK,
  AUDIO_BACKEND_JACK_LIBSOUNDIO,
  AUDIO_BACKEND_JACK_RTAUDIO,
  AUDIO_BACKEND_PULSEAUDIO,
  AUDIO_BACKEND_PULSEAUDIO_LIBSOUNDIO,
  AUDIO_BACKEND_PULSEAUDIO_RTAUDIO,
  AUDIO_BACKEND_COREAUDIO_LIBSOUNDIO,
  AUDIO_BACKEND_COREAUDIO_RTAUDIO,
  AUDIO_BACKEND_SDL,
  AUDIO_BACKEND_WASAPI_LIBSOUNDIO,
  AUDIO_BACKEND_WASAPI_RTAUDIO,
  AUDIO_BACKEND_ASIO_RTAUDIO,
  NUM_AUDIO_BACKENDS,
} AudioBackend;

static inline bool
audio_backend_is_rtaudio (
  AudioBackend backend)
{
  return
    backend == AUDIO_BACKEND_ALSA_RTAUDIO ||
    backend == AUDIO_BACKEND_JACK_RTAUDIO ||
    backend == AUDIO_BACKEND_PULSEAUDIO_RTAUDIO ||
    backend == AUDIO_BACKEND_COREAUDIO_RTAUDIO ||
    backend == AUDIO_BACKEND_WASAPI_RTAUDIO ||
    backend == AUDIO_BACKEND_ASIO_RTAUDIO;
}

__attribute__ ((unused))
static const char * audio_backend_str[] =
{
  /* TRANSLATORS: Dummy backend */
  __("Dummy"),
  __("Dummy (libsoundio)"),
  "ALSA (not working)",
  "ALSA (libsoundio)",
  "ALSA (rtaudio)",
  "JACK",
  "JACK (libsoundio)",
  "JACK (rtaudio)",
  "PulseAudio",
  "PulseAudio (libsoundio)",
  "PulseAudio (rtaudio)",
  "CoreAudio (libsoundio)",
  "CoreAudio (rtaudio)",
  "SDL",
  "WASAPI (libsoundio)",
  "WASAPI (rtaudio)",
  "ASIO (rtaudio)",
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
  MIDI_BACKEND_ALSA_RTMIDI,
  MIDI_BACKEND_JACK,
  MIDI_BACKEND_JACK_RTMIDI,
  MIDI_BACKEND_WINDOWS_MME,
  MIDI_BACKEND_WINDOWS_MME_RTMIDI,
  MIDI_BACKEND_COREMIDI_RTMIDI,
  NUM_MIDI_BACKENDS,
} MidiBackend;

static inline bool
midi_backend_is_rtmidi (
  MidiBackend backend)
{
  return
    backend == MIDI_BACKEND_ALSA_RTMIDI ||
    backend == MIDI_BACKEND_JACK_RTMIDI ||
    backend == MIDI_BACKEND_WINDOWS_MME_RTMIDI ||
    backend == MIDI_BACKEND_COREMIDI_RTMIDI;
}

static const char * midi_backend_str[] =
{
  /* TRANSLATORS: Dummy backend */
  __("Dummy"),
  __("ALSA Sequencer (not working)"),
  __("ALSA Sequencer (rtmidi)"),
  "JACK MIDI",
  "JACK MIDI (rtmidi)",
  "Windows MME",
  "Windows MME (rtmidi)",
  "CoreMIDI (rtmidi)",
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
  int               schema_version;

  /**
   * Cycle count to know which cycle we are in.
   *
   * Useful for debugging.
   */
  uint_fast64_t     cycle;

#ifdef HAVE_JACK
  /** JACK client. */
  jack_client_t *   client;
#else
  void *            client;
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

  /** Audio buffer size (block length), per
   * channel. */
  nframes_t         block_length;

  /** Size of MIDI port buffers in bytes. */
  size_t            midi_buf_size;

  /** Sample rate. */
  sample_rate_t     sample_rate;

  /** Number of frames/samples per tick. */
  double            frames_per_tick;

  /**
   * Reciprocal of \ref
   * AudioEngine.frames_per_tick.
   */
  double            ticks_per_frame;

  /** True iff buffer size callback fired. */
  int               buf_size_set;

  /** The processing graph router. */
  Router *          router;

  /** Input device processor. */
  HardwareProcessor * hw_in_processor;

  /** Output device processor. */
  HardwareProcessor * hw_out_processor;

#if 0
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
#endif

  /** MIDI Clock in TODO. */
  Port *            midi_clock_in;

  /** The ControlRoom. */
  ControlRoom *     control_room;

  /** Audio file pool. */
  AudioPool *       pool;

  /**
   * Used during tests to pass input data for
   * recording.
   *
   * Will be ignored if NULL.
   */
  StereoPorts *     dummy_input;

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
   * Manual note press events from the piano roll.
   *
   * The events from here should be read by the
   * corresponding track processor's MIDI in port
   * (TrackProcessor.midi_in). To avoid having to
   * recalculate the graph to reattach this port
   * to the correct track processor, only connect
   * this port to the initial processor in the
   * routing graph and fetch the events manually
   * when processing the corresponding track
   * processor.
   */
  Port *            midi_editor_manual_press;

  /**
   * Port used for receiving MIDI in messages for
   * binding CC and other non-recording purposes.
   */
  Port *            midi_in;

  /**
   * Number of frames/samples in the current
   * cycle, per channel.
   *
   * @note This is used by the engine internally.
   */
  nframes_t         nframes;

  /** Semaphore for blocking DSP while a plugin and
   * its ports are deleted. */
  ZixSem            port_operation_lock;

  /** Ok to process or not. */
  volatile gint     run;

  /** 1 if currently exporting. */
  gint              exporting;

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
#else
  void *       playback_handle;
  void *       seq_handle;
  void * hw_params;
  void * sw_params;
#endif

  /** ALSA audio buffer. */
  float *           alsa_out_buf;

  /* ------------------------------- */


  /** Flag used when processing in some backends. */
  volatile gint     filled_stereo_out_bufs;

  /** Flag used to check if we are inside
   * engine_process_prepare(). */
  gint              preparing_for_process;

#ifdef HAVE_PORT_AUDIO
  PaStream *        pa_stream;
#else
  void *            pa_stream;
#endif

  /**
   * Port Audio output buffer.
   *
   * Unlike JACK, the audio goes directly here.
   * FIXME this is not really needed, just
   * do the calculations in pa_stream_cb.
   */
  float *           pa_out_buf;

#ifdef _WOE32
  /** Windows MME MIDI devices. */
  WindowsMmeDevice * mme_in_devs[1024];
  int                num_mme_in_devs;
  WindowsMmeDevice * mme_out_devs[1024];
  int                num_mme_out_devs;
#else
  void * mme_in_devs[1024];
  int                num_mme_in_devs;
  void * mme_out_devs[1024];
  int                num_mme_out_devs;
#endif

#ifdef HAVE_SDL
  SDL_AudioDeviceID dev;
#else
  uint32_t          sdl_dev;
#endif

#ifdef HAVE_RTAUDIO
  rtaudio_t         rtaudio;
#else
  void *            rtaudio;
#endif

#ifdef HAVE_PULSEAUDIO
  pa_threaded_mainloop * pulse_mainloop;
  pa_context *           pulse_context;
  pa_stream *            pulse_stream;
#else
  void *                 pulse_mainloop;
  void *                 pulse_context;
  void *                 pulse_stream;
#endif
  gboolean               pulse_notified_underflow;

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

  /** Timestamp at start of previous cycle. */
  gint64            last_timestamp_start;

  /** Timestamp at end of previous cycle. */
  gint64            last_timestamp_end;

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

  /* --- trial version flags --- */

  /** Time at start to keep track if trial limit
   * is reached. */
  gint64            zrythm_start_time;

  /** Flag to keep track of the first time the
   * limit is reached. */
  int               limit_reached;

  /* --- end trial version flags --- */

  /**
   * If this is on, only tracks/regions marked as
   * "for bounce" will be allowed to make sound.
   *
   * Automation and everything else will work as
   * normal.
   */
  BounceMode        bounce_mode;

  /** Bounce step cache. */
  BounceStep        bounce_step;

  /** Whether currently bouncing with parents
   * (cache). */
  bool              bounce_with_parents;

  /** The metronome. */
  Metronome *       metronome;

  /* --- events --- */

  /**
   * Event queue.
   *
   * Events such as buffer size change request and
   * sample size change request should be queued
   * here.
   *
   * The engine will skip processing while the
   * queue still has events or is currently
   * processing events.
   */
  MPMCQueue *       ev_queue;

  /**
   * Object pool of event structs to avoid
   * allocation.
   */
  ObjectPool *      ev_pool;

  /** ID of the event processing source func. */
  guint             process_source_id;

  /** Whether currently processing events. */
  int               processing_events;

  /** Time last event processing started. */
  gint64            last_events_process_started;

  /** Time last event processing completed. */
  gint64            last_events_processed;

  /* --- end events --- */

  /** Whether the cycle is currently running. */
  volatile gint     cycle_running;

  /** Whether the engine is already pre-set up. */
  bool              pre_setup;

  /** Whether the engine is already set up. */
  bool              setup;

  /** Whether the engine is currently activated. */
  bool              activated;

  /** Pointer to owner project, if any. */
  Project *         project;
} AudioEngine;

static const cyaml_schema_field_t
engine_fields_schema[] =
{
  YAML_FIELD_INT (
    AudioEngine, schema_version),
  YAML_FIELD_ENUM (
    AudioEngine, transport_type,
    jack_transport_type_strings),
  YAML_FIELD_INT (
    AudioEngine, sample_rate),
  YAML_FIELD_FLOAT (
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
  YAML_FIELD_MAPPING_PTR (
    AudioEngine, hw_in_processor,
    hardware_processor_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine, hw_out_processor,
    hardware_processor_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
engine_schema =
{
  YAML_VALUE_PTR (
    AudioEngine, engine_fields_schema),
};

void
engine_realloc_port_buffers (
  AudioEngine * self,
  nframes_t     buf_size);

COLD
NONNULL_ARGS (1)
void
engine_init_loaded (
  AudioEngine * self,
  Project *     project);

/**
 * Create a new audio engine.
 *
 * This only initializes the engine and doe snot
 * connect to the backend.
 */
COLD
WARN_UNUSED_RESULT
AudioEngine *
engine_new (
  Project * project);

typedef struct EngineState
{
  /** Engine running. */
  int      running;
  /** Playback. */
  bool     playing;
  /** Transport loop. */
  bool     looping;
} EngineState;

/**
 * @param force_pause Whether to force transport
 *   pause, otherwise for engine to process and
 *   handle the pause request.
 */
void
engine_wait_for_pause (
  AudioEngine * self,
  EngineState * state,
  bool          force_pause);

void
engine_resume (
  AudioEngine * self,
  EngineState * state);

/**
 * Waits for n processing cycles to finish.
 *
 * Used during tests.
 */
void
engine_wait_n_cycles (
  AudioEngine * self,
  int           n);

void
engine_append_ports (
  AudioEngine * self,
  GPtrArray *   ports);

/**
 * Sets up the audio engine before the project is
 * initialized/loaded.
 */
void
engine_pre_setup (
  AudioEngine * self);

/**
 * Sets up the audio engine after the project
 * is initialized/loaded.
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
COLD
void
engine_activate (
  AudioEngine * self,
  bool          activate);

/**
 * Updates frames per tick based on the time sig,
 * the BPM, and the sample rate
 *
 * @param thread_check Whether to throw a warning
 *   if not called from GTK thread.
 * @param update_from_ticks Whether to update the
 *   positions based on ticks (true) or frames
 *   (false).
 * @param bpm_change Whether this is a BPM change.
 */
void
engine_update_frames_per_tick (
  AudioEngine *       self,
  const int           beats_per_bar,
  const bpm_t         bpm,
  const sample_rate_t sample_rate,
  bool                thread_check,
  bool                update_from_ticks,
  bool                bpm_change);

/**
 * GSourceFunc to be added using idle add.
 *
 * This will loop indefinintely.
 */
int
engine_process_events (
  AudioEngine * self);

/**
 * To be called by each implementation to prepare
 * the structures before processing.
 *
 * Clears buffers, marks all as unprocessed, etc.
 *
 * @return Whether the cycle should be skipped.
 */
NONNULL
HOT
bool
engine_process_prepare (
  AudioEngine * self,
  nframes_t     nframes);

/**
 * Processes current cycle.
 *
 * To be called by each implementation in its
 * callback.
 */
NONNULL
HOT
int
engine_process (
  AudioEngine *   self,
  const nframes_t total_frames_to_process);

/**
 * To be called after processing for common logic.
 *
 * @param roll_nframes Frames to roll (add to the
 *   playhead - if transport rolling).
 * @param nframes Total frames for this processing
 *   cycle.
 */
NONNULL
HOT
void
engine_post_process (
  AudioEngine *   self,
  const nframes_t roll_nframes,
  const nframes_t nframes);

/**
 * Called to fill in the external buffers at the end
 * of the processing cycle.
 */
NONNULL
void
engine_fill_out_bufs (
  AudioEngine *   self,
  const nframes_t nframes);

/**
 * Returns the int value corresponding to the
 * given AudioEngineBufferSize.
 */
int
engine_buffer_size_enum_to_int (
  AudioEngineBufferSize buffer_size);

/**
 * Returns the int value corresponding to the
 * given AudioEngineSamplerate.
 */
int
engine_samplerate_enum_to_int (
  AudioEngineSamplerate samplerate);

/**
 * Request the backend to set the buffer size.
 *
 * The backend is expected to call the buffer size
 * change callbacks.
 *
 * @seealso jack_set_buffer_size().
 */
void
engine_set_buffer_size (
  AudioEngine * self,
  uint32_t      buf_size);

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
const char *
engine_audio_backend_to_string (
  AudioBackend backend);

AudioBackend
engine_audio_backend_from_string (
  char * str);

/**
 * Returns the MIDI backend as a string.
 */
static inline const char *
engine_midi_backend_to_string (
  MidiBackend backend)
{
  return midi_backend_str[backend];
}

MidiBackend
engine_midi_backend_from_string (
  char * str);

/**
 * Reset the bounce mode on the engine, all tracks
 * and regions to OFF.
 */
void
engine_reset_bounce_mode (
  AudioEngine * self);

/**
 * Clones the audio engine.
 *
 * To be used for serialization.
 */
COLD
NONNULL
AudioEngine *
engine_clone (
  const AudioEngine * src);

/**
 * Closes any connections and free's data.
 */
COLD
NONNULL
void
engine_free (
  AudioEngine * self);

/**
 * @}
 */

#endif
