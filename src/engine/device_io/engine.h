// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include "dsp/panning.h"
#include "engine/device_io/ext_port.h"
#include "engine/device_io/hardware_processor.h"
#include "engine/session/control_room.h"
#include "engine/session/pool.h"
#include "engine/session/sample_processor.h"
#include "engine/session/transport.h"
#include "gui/dsp/audio_port.h"
#include "gui/dsp/midi_port.h"
#include "structure/tracks/channel.h"
#include "utils/audio.h"
#include "utils/concurrency.h"
#include "utils/object_pool.h"
#include "utils/types.h"

#ifdef HAVE_JACK
#  include "weakjack/weak_libjack.h"
#endif

namespace zrythm::gui::old_dsp::plugins
{
class Plugin;
}

#define AUDIO_ENGINE \
  (zrythm::engine::device_io::AudioEngine::get_active_instance ())
#define AUDIO_POOL (AUDIO_ENGINE->pool_)

#define DENORMAL_PREVENTION_VAL(engine_) ((engine_)->denormal_prevention_val_)

class Project;

namespace zrythm::engine::device_io
{
constexpr int ENGINE_MAX_EVENTS = 128;

enum class AudioBackend : basic_enum_base_type_t
{
  Dummy,
  Jack,
};

/**
 * Mode used when bouncing, either during exporting
 * or when bouncing tracks or regions to audio.
 */
enum class BounceMode : basic_enum_base_type_t
{
  /** Don't bounce. */
  Off,

  /** Bounce. */
  On,

  /**
   * Bounce if parent is bouncing.
   *
   * To be used on regions to bounce if their track is bouncing.
   */
  Inherit,
};

enum class MidiBackend : basic_enum_base_type_t
{
  Dummy,
  Jack,
};

/**
 * The audio engine.
 */
class AudioEngine final : public ICloneable<AudioEngine>, public IPortOwner
{
public:
  /**
   * Samplerates to be used in comboboxes.
   */
  enum class SampleRate : basic_enum_base_type_t
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
   * @brief Implementation that drives the audio callbacks.
   */
  class AudioDriver
  {
  public:
    virtual ~AudioDriver () = default;
    virtual bool buffer_size_change_handled () const { return true; }
    virtual void set_buffer_size (uint32_t buf_size) { }
    virtual void handle_buf_size_change (uint32_t frames) { };
    virtual void handle_sample_rate_change (uint32_t samplerate) { };
    virtual utils::Utf8String get_driver_name () const = 0;

    /**
     * @brief Sets up the driver.
     * @return True if the driver was successfully setup, false otherwise.
     */
    virtual bool setup_audio () = 0;

    virtual bool activate_audio (bool activate) = 0;
    virtual void tear_down_audio () = 0;
    virtual void handle_start () { }
    virtual void handle_stop () { }
    // optional additional logic during AudioEngine::prepare_process()
    virtual void prepare_process_audio () { }
    // This is used to work around a bug in PipeWire (see the JACK audio driver
    // for details)
    virtual bool
    sanity_check_should_return_early (nframes_t total_frames_to_process)
    {
      return false;
    }
    virtual void                         handle_position_change () { }
    virtual std::unique_ptr<PortBackend> create_audio_port_backend () const
    {
      return nullptr;
    }
    /**
     * Collects external ports of the given type.
     *
     * @param flow The signal flow. Note that this is inverse to what Zrythm
     * sees. E.g., to get MIDI inputs like MIDI keyboards, pass @ref
     * Z_PORT_FLOW_OUTPUT here.
     * @param hw Hardware or not.
     */
    virtual std::vector<ExtPort>
    get_ext_audio_ports (dsp::PortFlow flow, bool hw) const
    {
      return {};
    }
  };

  /**
   * @brief Implementation that handles MIDI from/to devices.
   */
  class MidiDriver
  {
  public:
    virtual ~MidiDriver () = default;
    virtual bool                         setup_midi () = 0;
    virtual bool                         activate_midi (bool activate) = 0;
    virtual void                         tear_down_midi () = 0;
    virtual std::unique_ptr<PortBackend> create_midi_port_backend () const
    {
      return nullptr;
    }
    virtual utils::Utf8String get_driver_name () const = 0;
    virtual std::vector<ExtPort>
    get_ext_midi_ports (dsp::PortFlow flow, bool hw) const
    {
      return {};
    }
  };

  /**
   * Audio engine event.
   */
  class Event
  {
  public:
    Event () : file_ (nullptr), func_ (nullptr) { }
    AudioEngineEventType type_ =
      AudioEngineEventType::AUDIO_ENGINE_EVENT_BUFFER_SIZE_CHANGE;
    void *            arg_{};
    uint32_t          uint_arg_{};
    float             float_arg_{};
    const char *      file_{};
    const char *      func_{};
    int               lineno_{};
    utils::Utf8String backtrace_;
  };
  static_assert (std::is_default_constructible_v<Event>);

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

  auto &get_port_registry () { return *port_registry_; }
  auto &get_port_registry () const { return *port_registry_; }
  structure::tracks::TrackRegistry &get_track_registry ();
  structure::tracks::TrackRegistry &get_track_registry () const;

  bool has_handled_buffer_size_change () const
  {
    return audio_driver_->buffer_size_change_handled ();
  }

  bool is_in_active_project () const override;

  void set_port_metadata_from_owner (dsp::PortIdentifier &id, PortRange &range)
    const override;

  utils::Utf8String
  get_full_designation_for_port (const dsp::PortIdentifier &id) const override;

  void push_event (
    AudioEngineEventType       type,
    void *                     arg = nullptr,
    uint32_t                   uint_arg = 0,
    float                      float_arg = 0.0f,
    const std::source_location loc = std::source_location::current ())
  {
    auto ev = ev_pool_.acquire ();
    ev->file_ = loc.file_name ();
    ev->func_ = loc.function_name ();
    ev->lineno_ = loc.line ();
    ev->type_ = type;
    ev->arg_ = arg;
    ev->uint_arg_ = uint_arg;
    ev->float_arg_ = float_arg;
    ev_queue_.push_back (ev);
  }

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

  /**
   * Queues MIDI note off to event queues.
   *
   * @note This pushes the events to the queued events, not the active events.
   */
  void panic_all ();

  /**
   * Clears the underlying backend's output buffers.
   *
   * Used when returning early.
   */
  void clear_output_buffers (nframes_t nframes);

private:
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
   * @note For JACK, set the MIDI driver to the audio driver.
   */
  std::shared_ptr<AudioDriver> audio_driver_;
  std::shared_ptr<MidiDriver>  midi_driver_;

  /**
   * Cycle count to know which cycle we are in.
   *
   * Useful for debugging.
   */
  std::atomic_uint64_t cycle_ = 0;

  /** Current audio backend. */
  AudioBackend audio_backend_{};

  /** Current MIDI backend. */
  MidiBackend midi_backend_{};

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
  bool buf_size_set_{};

  /** The processing graph router. */
  std::unique_ptr<session::Router> router_;

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
  std::unique_ptr<session::ControlRoom> control_room_;

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

  QPointer<dsp::PortConnectionsManager> port_connections_manager_;

  std::unique_ptr<session::SampleProcessor> sample_processor_;

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
  BounceMode bounce_mode_ = BounceMode::Off;

  /** Bounce step cache. */
  utils::audio::BounceStep bounce_step_ = {};

  /** Whether currently bouncing with parents (cache). */
  bool bounce_with_parents_ = false;

  /** The metronome. */
  std::unique_ptr<session::Metronome> metronome_;

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
}

DEFINE_ENUM_FORMATTER (
  zrythm::engine::device_io::AudioBackend,
  AudioBackend,
  /* TRANSLATORS: Dummy backend */
  QT_TR_NOOP_UTF8 ("Dummy"),
  "JACK")

DEFINE_ENUM_FORMATTER (
  zrythm::engine::device_io::MidiBackend,
  MidiBackend,
  /* TRANSLATORS: Dummy backend */
  QT_TR_NOOP_UTF8 ("Dummy"),
  "JACK MIDI")

DEFINE_ENUM_FORMATTER (
  zrythm::engine::device_io::AudioEngine::SampleRate,
  AudioEngine_SampleRate,
  "22,050",
  "32,000",
  "44,100",
  "48,000",
  "88,200",
  "96,000",
  "192,000")

DEFINE_ENUM_FORMATTER (
  zrythm::engine::device_io::AudioEngine::BufferSize,
  AudioEngine_BufferSize,
  "16",
  "32",
  "64",
  "128",
  "256",
  "512",
  "1,024",
  "2,048",
  "4,096")
