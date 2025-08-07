// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_port.h"
#include "dsp/midi_panic_processor.h"
#include "dsp/midi_port.h"
#include "dsp/panning.h"
#include "engine/device_io/audio_callback.h"
#include "engine/session/control_room.h"
#include "engine/session/sample_processor.h"
#include "engine/session/transport.h"
#include "utils/audio.h"
#include "utils/concurrency.h"
#include "utils/types.h"

#define AUDIO_ENGINE \
  (zrythm::engine::device_io::AudioEngine::get_active_instance ())

#define DENORMAL_PREVENTION_VAL(engine_) ((engine_)->denormal_prevention_val_)

class Project;

namespace zrythm::engine::device_io
{
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

/**
 * The audio engine.
 */
class AudioEngine : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
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
   * Buffer sizes to be used in combo boxes.
   */
  enum class BufferSize : basic_enum_base_type_t
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
  AudioEngine (
    Project *                                 project = nullptr,
    std::shared_ptr<juce::AudioDeviceManager> device_mgr =
      std::make_shared<juce::AudioDeviceManager> ());

  /**
   * Closes any connections and free's data.
   */
  ~AudioEngine () override;

  Q_INVOKABLE int xRunCount () const { return load_measurer_.getXRunCount (); }
  Q_INVOKABLE double loadPercentage () const
  {
    return load_measurer_.getLoadAsPercentage ();
  }

  auto &get_port_registry () { return *port_registry_; }
  auto &get_port_registry () const { return *port_registry_; }
  auto &get_param_registry () { return *param_registry_; }
  auto &get_param_registry () const { return *param_registry_; }
  structure::tracks::TrackRegistry &get_track_registry ();
  structure::tracks::TrackRegistry &get_track_registry () const;

  /**
   * @param force_pause Whether to force transport
   *   pause, otherwise for engine to process and
   *   handle the pause request.
   */
  void wait_for_pause (State &state, bool force_pause, bool with_fadeout);

  void realloc_port_buffers (nframes_t buf_size);

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

  using BeatsPerBarGetter = std::function<int ()>;
  using BpmGetter = std::function<bpm_t ()>;

  /**
   * Sets up the audio engine after the project is initialized/loaded.
   *
   * This also calls update_frames_per_tick() which requires the project to be
   * initialized/loaded.
   */
  void setup (BeatsPerBarGetter beats_per_bar_getter, BpmGetter bpm_getter);

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
   * Reset the bounce mode on the engine, all tracks and regions to OFF.
   */
  void reset_bounce_mode ();

  friend void init_from (
    AudioEngine           &obj,
    const AudioEngine     &other,
    utils::ObjectCloneType clone_type);

  std::pair<dsp::AudioPort &, dsp::AudioPort &> get_monitor_out_ports ();
  std::pair<dsp::AudioPort &, dsp::AudioPort &> get_dummy_input_ports ();

  /**
   * Queues MIDI note off to event queues.
   */
  void panic_all ();

  /**
   * Clears the underlying backend's output buffers.
   *
   * Used when returning early.
   */
  void clear_output_buffers (nframes_t nframes);

  auto get_device_manager () const { return device_manager_; }

  nframes_t get_block_length () const
  {
    auto * dev = device_manager_->getCurrentAudioDevice ();
    assert (dev != nullptr);
    return dev->getCurrentBufferSizeSamples ();
  }

  sample_rate_t get_sample_rate () const
  {
    auto * dev = device_manager_->getCurrentAudioDevice ();
    assert (dev != nullptr);
    return static_cast<sample_rate_t> (dev->getCurrentSampleRate ());
  }

private:
  static constexpr auto kFramesPerTickKey = "framesPerTick"sv;
  static constexpr auto kMonitorOutLKey = "monitorOutL"sv;
  static constexpr auto kMonitorOutRKey = "monitorOutR"sv;
  static constexpr auto kMidiInKey = "midiIn"sv;
  static constexpr auto kControlRoomKey = "controlRoom"sv;
  static constexpr auto kSampleProcessorKey = "sampleProcessor"sv;
  static constexpr auto kHwInProcessorKey = "hwInProcessor"sv;
  static constexpr auto kHwOutProcessorKey = "hwOutProcessor"sv;
  friend void           to_json (nlohmann::json &j, const AudioEngine &engine)
  {
    j = nlohmann::json{
      { kFramesPerTickKey,   engine.frames_per_tick_   },
      { kMonitorOutLKey,     engine.monitor_out_left_  },
      { kMonitorOutRKey,     engine.monitor_out_right_ },
      { kMidiInKey,          engine.midi_in_           },
      { kControlRoomKey,     engine.control_room_      },
      { kSampleProcessorKey, engine.sample_processor_  },
    };
  }
  friend void from_json (const nlohmann::json &j, AudioEngine &engine);

  [[gnu::cold]] void init_common ();

  void update_position_info (PositionInfo &pos_nfo, nframes_t frames_to_add);

  void receive_midi_events (uint32_t nframes);

private:
  OptionalRef<dsp::PortRegistry>               port_registry_;
  OptionalRef<dsp::ProcessorParameterRegistry> param_registry_;

public:
  /** Pointer to owner project, if any. */
  Project * project_ = nullptr;

  std::shared_ptr<juce::AudioDeviceManager> device_manager_;

  /**
   * Cycle count to know which cycle we are in.
   *
   * Useful for debugging.
   */
  std::atomic_uint64_t cycle_ = 0;

  /** Number of frames/samples per tick. */
  dsp::FramesPerTick frames_per_tick_;

  /**
   * Reciprocal of @ref frames_per_tick_.
   */
  dsp::TicksPerFrame ticks_per_frame_;

  /** True iff buffer size callback fired. */
  bool buf_size_set_{};

  /** The processing graph router. */
  std::unique_ptr<session::DspGraphDispatcher> router_;

// TODO: these should be separate processors
#if 0
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
#endif

  /** The ControlRoom. */
  std::unique_ptr<session::ControlRoom> control_room_;

  /**
   * Used during tests to pass input data for recording.
   *
   * Will be ignored if NULL.
   */
  std::optional<dsp::PortUuidReference> dummy_left_input_;
  std::optional<dsp::PortUuidReference> dummy_right_input_;

  /**
   * Monitor - these should be the last ports in the signal
   * chain.
   *
   * The L/R ports are exposed to the backend.
   */
  std::optional<dsp::PortUuidReference> monitor_out_left_;
  std::optional<dsp::PortUuidReference> monitor_out_right_;

  /**
   * Manual note press events from the piano roll.
   *
   * The events from here should be fed to the corresponding track
   * processor's MIDI in port (TrackProcessor.midi_in). To avoid having to
   * recalculate the graph to reattach this port to the correct track
   * processor, only connect this port to the initial processor in the
   * routing graph and fetch the events manually when processing the
   * corresponding track processor (or just do this at the start of each
   * processing cycle).
   *
   * @see DspGraphDispatcher.
   */
  dsp::MidiEventVector midi_editor_manual_press_;

  /**
   * Port used for receiving MIDI in messages for binding CC and other
   * non-recording purposes.
   *
   * This port is exposed to the backend.
   */
  std::unique_ptr<dsp::MidiPort> midi_in_;

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

  /* note: these 2 are ignored at the moment */
  /** Pan law. */
  dsp::PanLaw pan_law_ = {};
  /** Pan algorithm */
  dsp::PanAlgorithm pan_algo_ = {};

  juce::AudioProcessLoadMeasurer load_measurer_;

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

  /** Whether the cycle is currently running. */
  std::atomic_bool cycle_running_{ false };

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

  utils::QObjectUniquePtr<dsp::MidiPanicProcessor> midi_panic_processor_;

  std::unique_ptr<AudioCallback> audio_callback_;
};
}
