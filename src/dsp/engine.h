// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_callback.h"
#include "dsp/audio_port.h"
#include "dsp/graph_dispatcher.h"
#include "dsp/hardware_audio_interface.h"
#include "dsp/midi_panic_processor.h"
#include "dsp/midi_port.h"
#include "dsp/transport.h"
#include "utils/concurrency.h"
#include "utils/types.h"

namespace zrythm::dsp
{

/**
 * The audio engine.
 */
class AudioEngine : public QObject
{
  Q_OBJECT
  Q_PROPERTY (int sampleRate READ sampleRate NOTIFY sampleRateChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  enum class State : std::uint8_t
  {
    Uninitialized,
    Initialized,
    Active,
  };

public:
  /**
   * Create a new audio engine.
   *
   * This only initializes the engine and does not connect to any backend.
   */
  AudioEngine (
    dsp::Transport          &transport,
    IHardwareAudioInterface &hw_interface,
    dsp::DspGraphDispatcher &graph_dispatcher,
    const dsp::TempoMap     &tempo_map,
    QObject *                parent = nullptr);

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

  int sampleRate () const { return get_sample_rate ().in (units::sample_rate); }

  Q_SIGNAL void sampleRateChanged (int sampleRate);

  // =========================================================

  /**
   * @param force_pause Whether to force transport
   *   pause, otherwise for engine to process and
   *   handle the pause request.
   */
  void wait_for_pause (EngineState &state, bool force_pause, bool with_fadeout);

  void resume (const EngineState &state);

  Q_INVOKABLE void activate ();
  Q_INVOKABLE void deactivate ();

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
    dsp::Transport::TransportSnapshot               &transport_snapshot,
    nframes_t                                        nframes,
    SemaphoreRAII<moodycamel::LightweightSemaphore> &sem) noexcept
    [[clang::nonblocking]];

  enum class ProcessReturnStatus : std::uint8_t
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
  [[gnu::hot]] auto process (
    const dsp::PlayheadProcessingGuard &playhead_guard,
    nframes_t total_frames_to_process) noexcept [[clang::nonblocking]]
  -> ProcessReturnStatus;

  /**
   * @brief Advances the playhead if transport is rolling.
   *
   * @param roll_nframes Frames to roll (add to the playhead - if transport
   * rolling).
   * @param nframes Total frames for this processing cycle.
   */
  [[gnu::hot]] void advance_playhead_after_processing (
    dsp::Transport::TransportSnapshot  &transport_snapshot,
    const dsp::PlayheadProcessingGuard &playhead_guard,
    nframes_t                           roll_nframes,
    nframes_t nframes) noexcept [[clang::nonblocking]];

  auto &get_monitor_out_port () { return monitor_out_; }

  auto * midi_panic_processor () const { return midi_panic_processor_.get (); }

  /**
   * Queues MIDI note off to event queues.
   */
  void panic_all ();

  bool  activated () const { return state_ == State::Active; }
  bool  running () const { return run_.load (); }
  void  set_running (bool run) { run_.store (run); }
  auto &graph_dispatcher () { return graph_dispatcher_; }

  bool exporting () const { return exporting_; }
  void set_exporting (bool exporting) { exporting_.store (exporting); }

  auto get_processing_lock () [[clang::blocking]]
  {
    return SemaphoreRAII (process_lock_, true);
  }

  nframes_t get_block_length () const
  {
    return hw_interface_.get_block_length ();
  }

  units::sample_rate_t get_sample_rate () const
  {
    return hw_interface_.get_sample_rate ();
  }

  /**
   * @brief Executes the given function after pausing processing and then
   * resumes processing.
   *
   * @param recalculate_graph Whether to also recreate the processing graph
   * before resuming processing.
   */
  void execute_function_with_paused_processing_synchronously (
    const std::function<void ()> &func,
    bool                          recalculate_graph);

private:
  /**
   * Activates the audio engine to start processing and receiving events.
   *
   * @param activate Activate or deactivate.
   */
  void activate_impl (bool activate);

private:
  dsp::PortRegistry               port_registry_;
  dsp::ProcessorParameterRegistry param_registry_{ port_registry_ };

  dsp::Transport &transport_;

  /** The tempo map for timing calculations. */
  const dsp::TempoMap &tempo_map_;

  /** The processing graph router. */
  dsp::DspGraphDispatcher &graph_dispatcher_;

  IHardwareAudioInterface &hw_interface_;

  /**
   * Cycle count to know which cycle we are in.
   *
   * Useful for debugging.
   */
  std::atomic_uint64_t cycle_{ 0 };

  /**
   * @brief The last audio output in the signal chain.
   *
   * The contents of this port will be passed on to the audio output device
   * at the end of every processing cycle.
   *
   * @note Since the audio engine runs (but finishes early) even while the
   * graph gets recalculated, this port's buffers can be re-allocated by the
   * graph, so we need to make sure we don't use its buffers unless
   * processing is successful (no early finish). See the AudioCallback
   * lambda for details.
   */
  dsp::AudioPort monitor_out_;

  /**
   * Port used for receiving MIDI in messages for binding CC and other
   * non-recording purposes.
   *
   * This port is exposed to the backend.
   */
  std::unique_ptr<dsp::MidiPort> midi_in_;

  /**
   * Semaphore acquired during processing.
   */
  moodycamel::LightweightSemaphore process_lock_{ 1 };

  /** Ok to process or not. */
  std::atomic_bool run_{ false };

  /** Whether currently exporting. */
  std::atomic_bool exporting_{ false };

  juce::AudioProcessLoadMeasurer load_measurer_;

  /**
   * @brief When first set, it is equal to the max playback latency of all
   * initial trigger nodes.
   */
  nframes_t remaining_latency_preroll_{};

  /** To be set to 1 when the CC from the Midi in port should be captured. */
  std::atomic_bool capture_cc_{ false };

  /** Last MIDI CC captured. */
  std::array<midi_byte_t, 3> last_cc_captured_{};

  std::atomic<State> state_{ State::Uninitialized };
  static_assert (decltype (state_)::is_always_lock_free);

  utils::QObjectUniquePtr<dsp::MidiPanicProcessor> midi_panic_processor_;

  std::unique_ptr<AudioCallback> audio_callback_;

  /**
   * @brief Whether the audio callback is currently periodically getting
   * called.
   */
  bool callback_running_{};
};
}
