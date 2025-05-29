// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2020 Ryan Gonzalez <rymg19 at gmail dot com>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include "dsp/panning.h"
#include "gui/backend/channel.h"
#include "gui/dsp/audio_port.h"
#include "gui/dsp/control_room.h"
#include "gui/dsp/ext_port.h"
#include "gui/dsp/hardware_processor.h"
#include "gui/dsp/midi_port.h"
#include "gui/dsp/pool.h"
#include "gui/dsp/sample_processor.h"
#include "gui/dsp/transport.h"
#include "utils/audio.h"
#include "utils/backtrace.h"
#include "utils/concurrency.h"
#include "utils/object_pool.h"
#include "utils/types.h"

#ifdef HAVE_JACK
#  include "weakjack/weak_libjack.h"
#endif

#if HAVE_PULSEAUDIO
#  include <pulse/pulseaudio.h>
#endif

#ifdef HAVE_PORT_AUDIO
#  include <portaudio.h>
#endif

#if HAVE_RTAUDIO
#  include <rtaudio_c.h>
#endif

namespace zrythm::gui::old_dsp::plugins
{
class Plugin;
}
class Tracklist;
class ExtPort;
class MidiMappings;
class Router;
class Metronome;
class Project;
class HardwareProcessor;

/**
 * @addtogroup dsp Audio
 *
 * @{
 */

// should be set by backend
constexpr int BLOCK_LENGTH = 4096;

// should be set by backend
constexpr int MIDI_BUF_SIZE = 1024;

#define AUDIO_ENGINE (AudioEngine::get_active_instance ())
#define AUDIO_POOL (AUDIO_ENGINE->pool_)

#define DENORMAL_PREVENTION_VAL(engine_) ((engine_)->denormal_prevention_val_)

constexpr int ENGINE_MAX_EVENTS = 128;

/**
 * Push events.
 */
#define ENGINE_EVENTS_PUSH(et, _arg, _uint_arg, _float_arg) \
  { \
    auto _ev = AUDIO_ENGINE->ev_pool_.acquire (); \
    _ev->file_ = __FILE__; \
    _ev->func_ = __func__; \
    _ev->lineno_ = __LINE__; \
    _ev->type_ = et; \
    _ev->arg_ = (void *) _arg; \
    _ev->uint_arg_ = _uint_arg; \
    _ev->float_arg_ = _float_arg; \
    AUDIO_ENGINE->ev_queue_.push_back (_ev); \
  }

enum class AudioBackend
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
};

static inline bool
audio_backend_is_rtaudio (AudioBackend backend)
{
  return backend == AudioBackend::AUDIO_BACKEND_ALSA_RTAUDIO
         || backend == AudioBackend::AUDIO_BACKEND_JACK_RTAUDIO
         || backend == AudioBackend::AUDIO_BACKEND_PULSEAUDIO_RTAUDIO
         || backend == AudioBackend::AUDIO_BACKEND_COREAUDIO_RTAUDIO
         || backend == AudioBackend::AUDIO_BACKEND_WASAPI_RTAUDIO
         || backend == AudioBackend::AUDIO_BACKEND_ASIO_RTAUDIO;
}

/**
 * Mode used when bouncing, either during exporting
 * or when bouncing tracks or regions to audio.
 */
enum class BounceMode
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
};

enum class MidiBackend
{
  MIDI_BACKEND_DUMMY,
  MIDI_BACKEND_ALSA,
  MIDI_BACKEND_ALSA_RTMIDI,
  MIDI_BACKEND_JACK,
  MIDI_BACKEND_JACK_RTMIDI,
  MIDI_BACKEND_WINDOWS_MME,
  MIDI_BACKEND_WINDOWS_MME_RTMIDI,
  MIDI_BACKEND_COREMIDI_RTMIDI,
  MIDI_BACKEND_WINDOWS_UWP_RTMIDI,
};

static inline bool
midi_backend_is_rtmidi (MidiBackend backend)
{
  return backend == MidiBackend::MIDI_BACKEND_ALSA_RTMIDI
         || backend == MidiBackend::MIDI_BACKEND_JACK_RTMIDI
         || backend == MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI
         || backend == MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI
         || backend == MidiBackend::MIDI_BACKEND_WINDOWS_UWP_RTMIDI;
}

/**
 * The audio engine.
 */
class AudioEngine final : public ICloneable<AudioEngine>, public IPortOwner
{
public:
  enum class JackTransportType
  {
    TimebaseMaster,
    TransportClient,
    NoJackTransport,
  };

  /**
   * Samplerates to be used in comboboxes.
   */
  enum class SampleRate
  {
    SR_22050,
    SR_32000,
    SR_44100,
    SR_48000,
    SR_88200,
    SR_96000,
    SR_192000,
  };

  /**
   * Audio engine event type.
   */
  enum class AudioEngineEventType
  {
    AUDIO_ENGINE_EVENT_BUFFER_SIZE_CHANGE,
    AUDIO_ENGINE_EVENT_SAMPLE_RATE_CHANGE,
  };

  /**
   * Audio engine event.
   */
  class Event
  {
  public:
    Event () : file_ (nullptr), func_ (nullptr) { }

  public:
    AudioEngineEventType type_ =
      AudioEngineEventType::AUDIO_ENGINE_EVENT_BUFFER_SIZE_CHANGE;
    void *            arg_ = nullptr;
    uint32_t          uint_arg_ = 0;
    float             float_arg_ = 0.0f;
    const char *      file_ = nullptr;
    const char *      func_ = nullptr;
    int               lineno_ = 0;
    utils::Utf8String backtrace_;
  };

  /**
   * Buffer sizes to be used in combo boxes.
   */
  enum class BufferSize
  {
    _16,
    _32,
    _64,
    _128,
    _256,
    _512,
    _1024,
    _2048,
    _4096,
  };

  // TODO: check if pausing/resuming can be done with RAII
  struct State
  {
    /** Engine running. */
    bool running_;
    /** Playback. */
    bool playing_;
    /** Transport loop. */
    bool looping_;
  };

  struct PositionInfo
  {
    /** Transport is rolling. */
    bool is_rolling_ = false;

    /** BPM. */
    float bpm_ = 120.f;

    /** Exact playhead position (in ticks). */
    double playhead_ticks_ = 0.0;

    /* - below are used as cache to avoid re-calculations - */

    /** Current bar. */
    int32_t bar_ = 1;

    /** Current beat (within bar). */
    int32_t beat_ = 1;

    /** Current sixteenth (within beat). */
    int32_t sixteenth_ = 1;

    /** Current sixteenth (within bar). */
    int32_t sixteenth_within_bar_ = 1;

    /** Current sixteenth (within song, ie, total
     * sixteenths). */
    int32_t sixteenth_within_song_ = 1;

    /** Current tick-within-beat. */
    double tick_within_beat_ = 0.0;

    /** Current tick (within bar). */
    double tick_within_bar_ = 0.0;

    /** Total 1/96th notes completed up to current pos. */
    int32_t ninetysixth_notes_ = 0;
  };

public:
  /**
   * Create a new audio engine.
   *
   * This only initializes the engine and does not connect to any backend.
   */
  AudioEngine (Project * project = nullptr);

  /**
   * Closes any connections and free's data.
   */
  ~AudioEngine () override;

  auto          &get_port_registry () { return *port_registry_; }
  auto          &get_port_registry () const { return *port_registry_; }
  TrackRegistry &get_track_registry ();
  TrackRegistry &get_track_registry () const;

  bool has_handled_buffer_size_change () const
  {
    return audio_backend_ != AudioBackend::AUDIO_BACKEND_JACK
           || (audio_backend_ == AudioBackend::AUDIO_BACKEND_JACK && handled_jack_buffer_size_change_.load());
  }

  bool is_in_active_project () const override;

  void set_port_metadata_from_owner (dsp::PortIdentifier &id, PortRange &range)
    const override;

  utils::Utf8String
  get_full_designation_for_port (const dsp::PortIdentifier &id) const override;

  /**
   * @param force_pause Whether to force transport
   *   pause, otherwise for engine to process and
   *   handle the pause request.
   */
  void wait_for_pause (State &state, bool force_pause, bool with_fadeout);

  void realloc_port_buffers (nframes_t buf_size);

  /**
   * Renames the port on the backend side.
   */
  void rename_port_backend (Port &port)
  {
    if (!port.is_exposed_to_backend ())
      return;

    // just re-expose - this causes a rename if already exposed
    set_port_exposed_to_backend (port, true);
  }

  /**
   * Sets whether to expose the port to the backend and exposes it or removes
   * it.
   *
   * It checks what the backend is using the engine's audio backend or midi
   * backend settings.
   */
  void set_port_exposed_to_backend (Port &port, bool expose);

  /**
   * @brief
   *
   * @param project
   * @throw ZrythmError if failed to initialize.
   */
  [[gnu::cold]] void init_loaded (Project * project);

  void resume (State &state);

  /**
   * Waits for n processing cycles to finish.
   *
   * Used during tests.
   */
  void wait_n_cycles (int n);

  void append_ports (std::vector<Port *> &ports);

  /**
   * Sets up the audio engine before the project is initialized/loaded.
   */
  void pre_setup ();

  /**
   * Sets up the audio engine after the project is initialized/loaded.
   */
  void setup ();

  static AudioEngine * get_active_instance ();

  /**
   * Activates the audio engine to start processing and receiving events.
   *
   * @param activate Activate or deactivate.
   */
  [[gnu::cold]] void activate (bool activate);

  /**
   * Updates frames per tick based on the time sig, the BPM, and the sample rate
   *
   * @param thread_check Whether to throw a warning if not called from GTK
   * thread.
   * @param update_from_ticks Whether to update the positions based on ticks
   * (true) or frames (false).
   * @param bpm_change Whether this is a BPM change.
   */
  void update_frames_per_tick (
    int           beats_per_bar,
    bpm_t         bpm,
    sample_rate_t sample_rate,
    bool          thread_check,
    bool          update_from_ticks,
    bool          bpm_change);

  /**
   * Timeout function to be called periodically by Glib.
   */
  bool process_events ();

  /**
   * To be called by each implementation to prepare the structures before
   * processing.
   *
   * Clears buffers, marks all as unprocessed, etc.
   *
   * @param sem SemamphoreRAII to check if acquired. If not acquired before
   * calling this function, it will only clear output buffers and return true.
   * @return Whether the cycle should be skipped.
   */
  [[gnu::hot]] bool process_prepare (
    nframes_t                                         nframes,
    SemaphoreRAII<moodycamel::LightweightSemaphore> * sem = nullptr);

  /**
   * Processes current cycle.
   *
   * To be called by each implementation in its callback.
   */
  [[gnu::hot]] int process (nframes_t total_frames_to_process);

  /**
   * To be called after processing for common logic.
   *
   * @param roll_nframes Frames to roll (add to the playhead - if transport
   * rolling).
   * @param nframes Total frames for this processing cycle.
   */
  [[gnu::hot]] void post_process (nframes_t roll_nframes, nframes_t nframes);

  /**
   * Called to fill in the external buffers at the end of the processing cycle.
   */
  void fill_out_bufs (nframes_t nframes);

  /**
   * Returns the int value corresponding to the given AudioEngineBufferSize.
   */
  static int buffer_size_enum_to_int (BufferSize buffer_size);

  /**
   * Returns the int value corresponding to the given AudioEngineSamplerate.
   */
  static int samplerate_enum_to_int (SampleRate samplerate);

  /**
   * Request the backend to set the buffer size.
   *
   * The backend is expected to call the buffer size change callbacks.
   *
   * @see jack_set_buffer_size().
   */
  void set_buffer_size (uint32_t buf_size);

  /**
   * Reset the bounce mode on the engine, all tracks and regions to OFF.
   */
  void reset_bounce_mode ();

  /**
   * Detects the best backends on the system and sets them to GSettings.
   *
   * @param reset_to_dummy Whether to reset the backends to dummy before
   * attempting to set defaults.
   */
  static void set_default_backends (bool reset_to_dummy);

  void init_after_cloning (const AudioEngine &other, ObjectCloneType clone_type)
    override;

  std::pair<AudioPort &, AudioPort &> get_monitor_out_ports ();
  std::pair<AudioPort &, AudioPort &> get_dummy_input_ports ();

private:
  static constexpr auto kTransportTypeKey = "transportType"sv;
  static constexpr auto kSampleRateKey = "sampleRate"sv;
  static constexpr auto kFramesPerTickKey = "framesPerTick"sv;
  static constexpr auto kMonitorOutLKey = "monitorOutL"sv;
  static constexpr auto kMonitorOutRKey = "monitorOutR"sv;
  static constexpr auto kMidiEditorManualPressKey = "midiEditorManualPress"sv;
  static constexpr auto kMidiInKey = "midiIn"sv;
  static constexpr auto kPoolKey = "pool"sv;
  static constexpr auto kControlRoomKey = "controlRoom"sv;
  static constexpr auto kSampleProcessorKey = "sampleProcessor"sv;
  static constexpr auto kHwInProcessorKey = "hwInProcessor"sv;
  static constexpr auto kHwOutProcessorKey = "hwOutProcessor"sv;
  friend void           to_json (nlohmann::json &j, const AudioEngine &engine)
  {
    j = nlohmann::json{
      { kTransportTypeKey,         engine.transport_type_           },
      { kSampleRateKey,            engine.sample_rate_              },
      { kFramesPerTickKey,         engine.frames_per_tick_          },
      { kMonitorOutLKey,           engine.monitor_out_left_         },
      { kMonitorOutRKey,           engine.monitor_out_right_        },
      { kMidiEditorManualPressKey, engine.midi_editor_manual_press_ },
      { kMidiInKey,                engine.midi_in_                  },
      { kPoolKey,                  engine.pool_                     },
      { kControlRoomKey,           engine.control_room_             },
      { kSampleProcessorKey,       engine.sample_processor_         },
      { kHwInProcessorKey,         engine.hw_in_processor_          },
      { kHwOutProcessorKey,        engine.hw_out_processor_         },
    };
  }
  friend void from_json (const nlohmann::json &j, AudioEngine &engine);

  /**
   * Cleans duplicate events and copies the events to the given vector.
   */
  int clean_duplicate_events_and_copy (std::array<Event *, 100> &ret);

  [[gnu::cold]] void init_common ();

  void update_position_info (PositionInfo &pos_nfo, nframes_t frames_to_add);

  /**
   * Clears the underlying backend's output buffers.
   *
   * Used when returning early.
   */
  void clear_output_buffers (nframes_t nframes);

  void receive_midi_events (uint32_t nframes);

  /**
   * @brief Stops events from getting fired.
   *
   */
  void stop_events ();

private:
  OptionalRef<PortRegistry> port_registry_;

public:
  /** Pointer to owner project, if any. */
  Project * project_ = nullptr;

  /**
   * Cycle count to know which cycle we are in.
   *
   * Useful for debugging.
   */
  std::atomic_uint64_t cycle_ = 0;

#ifdef HAVE_JACK
  /** JACK client. */
  jack_client_t * client_ = nullptr;
#else
  void * client_ = nullptr;
#endif

  /**
   * Whether pending jack buffer change was handled (buffers reallocated).
   *
   * To be set to zero when a change starts and 1 when the change is fully
   * processed.
   */
  std::atomic_bool handled_jack_buffer_size_change_ = false;

  /**
   * Whether transport master/client or no connection with jack transport.
   */
  JackTransportType transport_type_ = (JackTransportType) 0;

  /** Current audio backend. */
  AudioBackend audio_backend_ = (AudioBackend) 0;

  /** Current MIDI backend. */
  MidiBackend midi_backend_ = (MidiBackend) 0;

  /** Audio buffer size (block length), per channel. */
  nframes_t block_length_ = 0;

  /** Size of MIDI port buffers in bytes. */
  size_t midi_buf_size_ = 0;

  /** Sample rate. */
  sample_rate_t sample_rate_ = 0;

  /** Number of frames/samples per tick. */
  dsp::FramesPerTick frames_per_tick_;

  /**
   * Reciprocal of @ref frames_per_tick_.
   */
  dsp::TicksPerFrame ticks_per_frame_;

  /** True iff buffer size callback fired. */
  int buf_size_set_ = 0;

  /** The processing graph router. */
  std::unique_ptr<Router> router_;

  /** Input device processor. */
  std::unique_ptr<HardwareProcessor> hw_in_processor_;

  /** Output device processor. */
  std::unique_ptr<HardwareProcessor> hw_out_processor_;

  /**
   * MIDI Clock input TODO.
   *
   * This port is exposed to the backend.
   */
  std::unique_ptr<MidiPort> midi_clock_in_;

  /**
   * MIDI Clock output.
   *
   * This port is exposed to the backend.
   */
  std::unique_ptr<MidiPort> midi_clock_out_;

  /** The ControlRoom. */
  std::unique_ptr<ControlRoom> control_room_;

  /** Audio file pool. */
  std::unique_ptr<AudioPool> pool_;

  /**
   * Used during tests to pass input data for recording.
   *
   * Will be ignored if NULL.
   */
  std::optional<PortUuidReference> dummy_left_input_;
  std::optional<PortUuidReference> dummy_right_input_;

  /**
   * Monitor - these should be the last ports in the signal
   * chain.
   *
   * The L/R ports are exposed to the backend.
   */
  std::optional<PortUuidReference> monitor_out_left_;
  std::optional<PortUuidReference> monitor_out_right_;

  /**
   * Flag to tell the UI that this channel had MIDI activity.
   *
   * When processing this and setting it to 0, the UI should
   * create a separate event using EVENTS_PUSH.
   */
  std::atomic_bool trigger_midi_activity_{ false };

  /**
   * Manual note press events from the piano roll.
   *
   * The events from here should be read by the corresponding track
   * processor's MIDI in port (TrackProcessor.midi_in). To avoid having to
   * recalculate the graph to reattach this port to the correct track
   * processor, only connect this port to the initial processor in the
   * routing graph and fetch the events manually when processing the
   * corresponding track processor.
   */
  std::unique_ptr<MidiPort> midi_editor_manual_press_;

  /**
   * Port used for receiving MIDI in messages for binding CC and other
   * non-recording purposes.
   *
   * This port is exposed to the backend.
   */
  std::unique_ptr<MidiPort> midi_in_;

  /**
   * Number of frames/samples in the current cycle, per channel.
   *
   * @note This is used by the engine internally.
   */
  nframes_t nframes_ = 0;

  /**
   * Semaphore for blocking DSP while a plugin and its ports are deleted.
   */
  moodycamel::LightweightSemaphore port_operation_lock_{ 1 };

  /** Ok to process or not. */
  std::atomic_bool run_{ false };

  /** To be set to true when preparing to export. */
  bool preparing_to_export_ = false;

  /** Whether currently exporting. */
  std::atomic_bool exporting_{ false };

  /** Send note off MIDI everywhere. */
  std::atomic_bool panic_{ false };

#ifdef HAVE_PORT_AUDIO
  PaStream * port_audio_stream_ = nullptr;
#else
  void * port_audio_stream_ = nullptr;
#endif

  /**
   * Port Audio output buffer.
   *
   * Unlike JACK, the audio goes directly here.
   * FIXME: this is not really needed, just do the calculations in
   * port_audio_stream_cb.
   */
  float * port_audio_out_buf_ = nullptr;

#if HAVE_RTAUDIO
  rtaudio_t rtaudio_ = nullptr;
#else
  void * rtaudio_ = nullptr;
#endif

#if HAVE_PULSEAUDIO
  pa_threaded_mainloop * pulse_mainloop_ = nullptr;
  pa_context *           pulse_context_ = nullptr;
  pa_stream *            pulse_stream_ = nullptr;
#else
  void * pulse_mainloop_ = nullptr;
  void * pulse_context_ = nullptr;
  void * pulse_stream_ = nullptr;
#endif
  std::atomic_bool pulse_notified_underflow_{ false };

  /**
   * @brief Dummy audio DSP processing thread.
   *
   * Used during tests or when no audio backend is available.
   *
   * Use signalThreadShouldExit() to stop the thread.
   *
   * @note The thread will join automatically when the engine is destroyed.
   */
  std::unique_ptr<juce::Thread> dummy_audio_thread_;

  /* note: these 2 are ignored at the moment */
  /** Pan law. */
  dsp::PanLaw pan_law_ = {};
  /** Pan algorithm */
  dsp::PanAlgorithm pan_algo_ = {};

  /** Time taken to process in the last cycle */
  // SteadyDuration last_time_taken_;

  /**
   * @brief Max time taken in the last few processing cycles.
   *
   * This is used by the DSP usage meter, which also clears the value to 0
   * upon reading it.
   *
   * @note Could use a lock since it is written to by 2 different threads,
   * but there are no consequences if it has bad data.
   */
  RtDuration max_time_taken_{};

  /** Timestamp at the start of the current cycle. */
  RtTimePoint timestamp_start_{};

  /** Expected timestamp at the end of the current cycle. */
  // SteadyTimePoint timestamp_end_{};

  /** Timestamp at start of previous cycle. */
  RtTimePoint last_timestamp_start_{};

  /** Timestamp at end of previous cycle. */
  // SteadyTimePoint last_timestamp_end_{};

  /** When first set, it is equal to the max
   * playback latency of all initial trigger
   * nodes. */
  nframes_t remaining_latency_preroll_ = 0;

  std::unique_ptr<SampleProcessor> sample_processor_;

  /** To be set to 1 when the CC from the Midi in port should be captured. */
  std::atomic_bool capture_cc_{ false };

  /** Last MIDI CC captured. */
  std::array<midi_byte_t, 3> last_cc_captured_{};

  /**
   * Last time an XRUN notification was shown.
   *
   * This is to prevent multiple XRUN notifications being shown so quickly
   * that Zrythm becomes unusable.
   */
  SteadyTimePoint last_xrun_notification_;

  /**
   * Whether the denormal prevention value (1e-12 ~ 1e-20) is positive.
   *
   * This should be swapped often to avoid DC offset prevention algorithms
   * removing it.
   *
   * See https://www.earlevel.com/main/2019/04/19/floating-point-denormals/
   * for details.
   */
  bool  denormal_prevention_val_positive_ = true;
  float denormal_prevention_val_ = 1e-12f;

  /**
   * If this is on, only tracks/regions marked as "for bounce" will be
   * allowed to make sound.
   *
   * Automation and everything else will work as normal.
   */
  BounceMode bounce_mode_ = BounceMode::BOUNCE_OFF;

  /** Bounce step cache. */
  utils::audio::BounceStep bounce_step_ = {};

  /** Whether currently bouncing with parents (cache). */
  bool bounce_with_parents_ = false;

  /** The metronome. */
  std::unique_ptr<Metronome> metronome_;

  /* --- events --- */

  /**
   * Event queue.
   *
   * Events such as buffer size change request and sample size change
   * request should be queued here.
   *
   * The engine will skip processing while the queue still has events or is
   * currently processing events.
   */
  MPMCQueue<Event *> ev_queue_{ ENGINE_MAX_EVENTS };

  /**
   * Object pool of event structs to avoid allocation.
   */
  ObjectPool<Event> ev_pool_{ ENGINE_MAX_EVENTS };

  /** ID of the event processing source func. */
  // sigc::scoped_connection process_source_id_;

  /** Whether currently processing events. */
  bool processing_events_ = false;

  /** Time last event processing started. */
  SteadyTimePoint last_events_process_started_;

  /** Time last event processing completed. */
  SteadyTimePoint last_events_processed_;

  /* --- end events --- */

  /** Whether the cycle is currently running. */
  std::atomic_bool cycle_running_{ false };

  /** Whether the engine is already pre-set up. */
  bool pre_setup_ = false;

  /** Whether the engine is already set up. */
  bool setup_ = false;

  /** Whether the engine is currently activated. */
  bool activated_ = false;

  /** Whether the engine is currently undergoing destruction. */
  bool destroying_ = false;

  /**
   * True while updating frames per tick.
   *
   * See engine_update_frames_per_tick().
   */
  bool updating_frames_per_tick_ = false;

  /**
   * Position info at the end of the previous cycle before moving the
   * transport.
   */
  PositionInfo pos_nfo_before_ = {};

  /**
   * Position info at the start of the current cycle.
   */
  PositionInfo pos_nfo_current_ = {};

  /**
   * Expected position info at the end of the current cycle.
   */
  PositionInfo pos_nfo_at_end_ = {};
};

DEFINE_ENUM_FORMATTER (
  AudioBackend,
  AudioBackend,
  /* TRANSLATORS: Dummy backend */
  QT_TR_NOOP_UTF8 ("Dummy"),
  QT_TR_NOOP_UTF8 ("Dummy (libsoundio)"),
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
  "ASIO (rtaudio)")

DEFINE_ENUM_FORMATTER (
  MidiBackend,
  MidiBackend,
  /* TRANSLATORS: Dummy backend */
  QT_TR_NOOP_UTF8 ("Dummy"),
  QT_TR_NOOP_UTF8 ("ALSA Sequencer (not working)"),
  QT_TR_NOOP_UTF8 ("ALSA Sequencer (rtmidi)"),
  "JACK MIDI",
  "JACK MIDI (rtmidi)",
  "Windows MME",
  "Windows MME (rtmidi)",
  "CoreMIDI (rtmidi)",
  "Windows UWP (rtmidi)")

DEFINE_ENUM_FORMATTER (
  AudioEngine::SampleRate,
  AudioEngine_SampleRate,
  QT_TR_NOOP_UTF8 ("22,050"),
  QT_TR_NOOP_UTF8 ("32,000"),
  QT_TR_NOOP_UTF8 ("44,100"),
  QT_TR_NOOP_UTF8 ("48,000"),
  QT_TR_NOOP_UTF8 ("88,200"),
  QT_TR_NOOP_UTF8 ("96,000"),
  QT_TR_NOOP_UTF8 ("192,000"))

DEFINE_ENUM_FORMATTER (
  AudioEngine::BufferSize,
  AudioEngine_BufferSize,
  "16",
  "32",
  "64",
  "128",
  "256",
  "512",
  QT_TR_NOOP_UTF8 ("1,024"),
  QT_TR_NOOP_UTF8 ("2,048"),
  QT_TR_NOOP_UTF8 ("4,096"))

/**
 * @}
 */
