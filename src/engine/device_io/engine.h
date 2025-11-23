// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_port.h"
#include "dsp/midi_panic_processor.h"
#include "dsp/midi_port.h"
#include "dsp/panning.h"
#include "dsp/transport.h"
#include "engine/device_io/audio_callback.h"
#include "engine/session/graph_dispatcher.h"
#include "engine/session/sample_processor.h"
#include "utils/audio.h"
#include "utils/concurrency.h"
#include "utils/types.h"

#define AUDIO_ENGINE \
  (zrythm::engine::device_io::AudioEngine::get_active_instance ())

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
    dsp::Transport                           &transport,
    const dsp::TempoMap                      &tempo_map,
    dsp::PortRegistry                        &port_registry,
    dsp::ProcessorParameterRegistry          &param_registry,
    dsp::PanLaw                               pan_law,
    dsp::PanAlgorithm                         pan_algorithm,
    std::shared_ptr<juce::AudioDeviceManager> device_mgr,
    QObject *                                 parent = nullptr);

  /**
   * Closes any connections and free's data.
   */
  ~AudioEngine () override;

  // =========================================================
  // QML interface
  // =========================================================

  Q_INVOKABLE int xRunCount () const { return load_measurer_.getXRunCount (); }
  Q_INVOKABLE double loadPercentage () const
  {
    return load_measurer_.getLoadAsPercentage ();
  }

  // =========================================================

  /**
   * @param force_pause Whether to force transport
   *   pause, otherwise for engine to process and
   *   handle the pause request.
   */
  void wait_for_pause (EngineState &state, bool force_pause, bool with_fadeout);

  /**
   * @brief
   *
   * @param project
   * @throw ZrythmError if failed to initialize.
   */
  [[gnu::cold]] void init_loaded ();

  void resume (EngineState &state);

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
    dsp::Transport::TransportSnapshot                &transport_snapshot,
    nframes_t                                         nframes,
    SemaphoreRAII<moodycamel::LightweightSemaphore> * sem = nullptr);

  enum class ProcessReturnStatus
  {
    // Process completed normally
    ProcessCompleted,
    // Process skipped (e.g., when recalculating the graph)
    ProcessSkipped,
    // Process failed for some reason
    ProcessFailed,
  };

  /**
   * Processes current cycle.
   *
   * To be called by each implementation in its callback.
   */
  [[gnu::hot]] auto
  process (nframes_t total_frames_to_process) -> ProcessReturnStatus;

  /**
   * To be called after processing for common logic.
   *
   * @param roll_nframes Frames to roll (add to the playhead - if transport
   * rolling).
   * @param nframes Total frames for this processing cycle.
   */
  [[gnu::hot]] void post_process (
    dsp::Transport::TransportSnapshot &transport_snapshot,
    nframes_t                          roll_nframes,
    nframes_t                          nframes);

  /**
   * Reset the bounce mode on the engine, all tracks and regions to OFF.
   */
  void reset_bounce_mode ();

  friend void init_from (
    AudioEngine           &obj,
    const AudioEngine     &other,
    utils::ObjectCloneType clone_type);

  auto &get_monitor_out_port () { return monitor_out_; }

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

  bool  activated () const { return activated_; }
  bool  running () const { return run_.load (); }
  void  set_running (bool run) { run_.store (run); }
  auto &graph_dispatcher () { return router_; }

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
  static constexpr auto kMidiInKey = "midiIn"sv;
  static constexpr auto kSampleProcessorKey = "sampleProcessor"sv;
  static constexpr auto kHwInProcessorKey = "hwInProcessor"sv;
  static constexpr auto kHwOutProcessorKey = "hwOutProcessor"sv;
  friend void           to_json (nlohmann::json &j, const AudioEngine &engine);
  friend void from_json (const nlohmann::json &j, AudioEngine &engine);

  void update_position_info (PositionInfo &pos_nfo, nframes_t frames_to_add);

  void receive_midi_events (uint32_t nframes);

private:
  dsp::PortRegistry               &port_registry_;
  dsp::ProcessorParameterRegistry &param_registry_;

  dsp::Transport      &transport_;
  const dsp::TempoMap &tempo_map_;

  std::shared_ptr<juce::AudioDeviceManager> device_manager_;

  /**
   * Cycle count to know which cycle we are in.
   *
   * Useful for debugging.
   */
  std::atomic_uint64_t cycle_{ 0 };

  /**
   * @brief The last audio output in the signal chain.
   *
   * The contents of this port will be passed on to the audio output device at
   * the end of every processing cycle.
   */
  dsp::AudioPort monitor_out_;

public:
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

private:
  /**
   * Port used for receiving MIDI in messages for binding CC and other
   * non-recording purposes.
   *
   * This port is exposed to the backend.
   */
  std::unique_ptr<dsp::MidiPort> midi_in_;

  /**
   * Semaphore for blocking DSP while a plugin and its ports are deleted.
   */
  moodycamel::LightweightSemaphore port_operation_lock_{ 1 };

  /** Ok to process or not. */
  std::atomic_bool run_{ false };

public:
  /** Whether currently exporting. */
  std::atomic_bool exporting_{ false };

private:
  /* note: these 2 are ignored at the moment */
  /** Pan law. */
  dsp::PanLaw pan_law_{};
  /** Pan algorithm */
  dsp::PanAlgorithm pan_algo_{};

  juce::AudioProcessLoadMeasurer load_measurer_;

  /**
   * @brief When first set, it is equal to the max playback latency of all
   * initial trigger nodes.
   */
  nframes_t remaining_latency_preroll_{};

  std::unique_ptr<session::SampleProcessor> sample_processor_;

  /** To be set to 1 when the CC from the Midi in port should be captured. */
  std::atomic_bool capture_cc_{ false };

  /** Last MIDI CC captured. */
  std::array<midi_byte_t, 3> last_cc_captured_{};

public:
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

private:
  /** Whether the engine is already set up. */
  bool setup_ = false;

  /** Whether the engine is currently activated. */
  bool activated_ = false;

  /** Whether the engine is currently undergoing destruction. */
  bool destroying_ = false;

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

public:
  utils::QObjectUniquePtr<dsp::MidiPanicProcessor> midi_panic_processor_;

private:
  std::unique_ptr<AudioCallback> audio_callback_;

  /** The processing graph router. */
  std::unique_ptr<engine::session::DspGraphDispatcher> router_;
};
}
