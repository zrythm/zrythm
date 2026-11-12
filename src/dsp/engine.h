// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include "dsp/audio_callback.h"
#include "dsp/audio_input_processor.h"
#include "dsp/audio_port.h"
#include "dsp/graph_dispatcher.h"
#include "dsp/hardware_audio_interface.h"
#include "dsp/hardware_midi_interface.h"
#include "dsp/midi_input_processor.h"
#include "dsp/midi_panic_processor.h"
#include "dsp/midi_port.h"
#include "dsp/transport.h"
#include "utils/concurrency.h"
#include "utils/inplace_function.h"
#include "utils/object_registry.h"

namespace zrythm::dsp
{

/**
 * @brief Guard for the engine processing lock.
 *
 * The underlying semaphore is non-recursive: re-acquiring it from the thread
 * that already holds it is a bug (in blocking mode it would wait forever), so
 * the guard records the owning thread and re-entrant acquisition fails loudly
 * instead.
 *
 * Obtained from AudioEngine::get_processing_lock() (blocking) or
 * AudioEngine::try_get_processing_lock() (non-blocking, for the audio
 * thread).
 */
class EngineProcessingLockGuard
{
public:
  /**
   * @param try_acquire Try to acquire the semaphore without blocking. On
   * failure, is_acquired() returns false and the guard releases nothing.
   */
  EngineProcessingLockGuard (
    moodycamel::LightweightSemaphore &sem,
    std::atomic<std::thread::id>     &owner,
    bool                              try_acquire = false);
  ~EngineProcessingLockGuard ();

  EngineProcessingLockGuard (const EngineProcessingLockGuard &) = delete;
  EngineProcessingLockGuard &
  operator= (const EngineProcessingLockGuard &) = delete;
  EngineProcessingLockGuard (EngineProcessingLockGuard &&) = delete;
  EngineProcessingLockGuard &operator= (EngineProcessingLockGuard &&) = delete;

  bool is_acquired () const noexcept { return lock_->is_acquired (); }

private:
  std::atomic<std::thread::id> &owner_;

  /**
   * Always engaged: the constructor emplaces unconditionally after the
   * re-entrancy check (its only other exit is abort). Optional because the
   * check must run before acquisition.
   */
  std::optional<SemaphoreRAII<moodycamel::LightweightSemaphore>> lock_;
};

/**
 * The audio engine.
 */
class AudioEngine : public QObject
{
  Q_OBJECT
  Q_PROPERTY (int sampleRate READ sampleRate NOTIFY sampleRateChanged)
  Q_PROPERTY (int blockLength READ blockLength NOTIFY blockLengthChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  enum class State : std::uint8_t
  {
    Uninitialized,
    Initialized,
    Active,
  };

  /**
   * @brief RAII scope that pauses engine processing for its lifetime.
   *
   * The outermost scope of a nesting chain performs the full pause: it
   * sends a MIDI panic, pauses the transport, stops processing cycles
   * and, while the audio device is running, processes one flush cycle
   * for the panic messages. Its destruction restores the transport and
   * resumes processing. Nested scopes are inert: the outermost scope
   * owns both the pause and the restoration.
   *
   * A scope constructed while the engine is not running pauses nothing
   * and its destruction restores nothing.
   *
   * Scopes must be created and destroyed on the engine's thread. A
   * scope that must outlive the current stack frame can be
   * heap-allocated and destroyed on the engine's thread later.
   */
  class ProcessingPauseScope
  {
  public:
    struct Options
    {
      /**
       * @brief Set the transport play state to Paused directly instead
       * of waiting for an in-flight pause request to be applied by an
       * audio cycle.
       */
      bool hard_transport_pause = false;
    };

    explicit ProcessingPauseScope (AudioEngine &engine);
    explicit ProcessingPauseScope (AudioEngine &engine, Options options);
    ~ProcessingPauseScope ();

    /**
     * @brief Makes the destruction of this scope skip the resumption.
     *
     * The engine stays paused with the transport stopped.
     */
    void keep_paused () noexcept;

    ProcessingPauseScope (const ProcessingPauseScope &) = delete;
    ProcessingPauseScope &operator= (const ProcessingPauseScope &) = delete;
    ProcessingPauseScope (ProcessingPauseScope &&) = delete;
    ProcessingPauseScope &operator= (ProcessingPauseScope &&) = delete;

  private:
    AudioEngine &engine_;

    /** Whether this scope is nested inside another pause scope. */
    bool nested_ = false;

    /** Whether keep_paused() was called. */
    bool keep_paused_called_ = false;

    /** Transport and engine state captured when the pause started. */
    bool saved_running_ = false;
    bool saved_playing_ = false;
    bool saved_looping_ = false;
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
    IHardwareMidiInterface  &midi_interface,
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

  /**
   * @brief Current sample rate from the hardware interface (QML-friendly).
   */
  int sampleRate () const;
  /**
   * @brief Current block length from the hardware interface (QML-friendly).
   */
  int blockLength () const;

  Q_SIGNAL void sampleRateChanged (int sampleRate);
  Q_SIGNAL void blockLengthChanged (int blockLength);

  // =========================================================

  /**
   * @brief Current sample rate as a unit type.
   */
  units::sample_rate_t sample_rate () const;
  /**
   * @brief Current block length as a unit type.
   */
  units::sample_u32_t block_length () const;
  /**
   * @brief Current audio device name from the hardware interface.
   */
  utils::Utf8String device_name () const;

  /**
   * @brief Activate the engine if not already active.
   *
   * This method is idempotent.
   */
  Q_INVOKABLE void activate ();

  /**
   * @brief Deactivates the engine if active.
   *
   * This method is idempotent.
   */
  Q_INVOKABLE void deactivate ();

  /**
   * To be called by each implementation to prepare the structures before
   * processing.
   *
   * Clears buffers, marks all as unprocessed, etc.
   *
   * @param processing_lock Proof that the processing lock is held.
   */
  [[gnu::hot]] void preprocess (
    dsp::Transport::TransportSnapshot &transport_snapshot,
    const EngineProcessingLockGuard   &processing_lock) noexcept
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
   *
   * @param processing_lock Proof that the processing lock was attempted. If
   * it was not acquired, the cycle is skipped before any shared state is
   * touched.
   */
  [[gnu::hot]] auto process (
    const EngineProcessingLockGuard &processing_lock,
    units::sample_u32_t total_frames_to_process) noexcept [[clang::nonblocking]]
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
    units::sample_u32_t                 roll_nframes,
    units::sample_u32_t nframes) noexcept [[clang::nonblocking]];

  void set_monitor_out_source (dsp::AudioPort &port)
  {
    assert (!audio_callback_active_);
    monitor_out_source_ = &port;
  }

  auto * midi_panic_processor () const { return midi_panic_processor_.get (); }

  /**
   * @brief Returns the audio input processor, or nullptr if no audio device has
   * started.
   */
  auto * audio_input_processor () const
  {
    return audio_input_processor_.get ();
  }

  const auto &midi_input_processors () const { return midi_input_processors_; }

  /**
   * Queues MIDI note off to event queues.
   */
  void panic_all ();

  bool activated () const { return state_ == State::Active; }
  bool running () const { return run_.load (); }
  void set_running (bool run) { run_.store (run); }

  /**
   * @brief True while an offline render owns processing.
   *
   * While this is set, pause-gated mutations (state restores,
   * reactivations) are refused: the engine is paused for the render,
   * but the render thread still processes the same plugin instances.
   */
  bool offline_render_active () const { return offline_render_active_.load (); }

  /**
   * @brief Owns an engine handover to an offline render.
   *
   * The constructor pauses the engine and marks processing as owned
   * by the render. finish() clears the mark, recalculates the graph
   * and resumes the engine; it is idempotent and runs at most once,
   * and the destructor calls it as a backstop when a future chain was
   * torn down without any completion callback running. finish() and the
   * destructor both leave the engine resumed on the engine's thread: a
   * destructor running on another thread marshals the call there and
   * blocks until it completes.
   *
   * The render runs asynchronously: callers keep the session in a
   * shared_ptr captured by the future's completion callbacks and call
   * finish() from whichever callback runs.
   */
  class OfflineRenderSession
  {
  public:
    explicit OfflineRenderSession (AudioEngine &engine);
    ~OfflineRenderSession ();

    void finish () noexcept;

    OfflineRenderSession (const OfflineRenderSession &) = delete;
    OfflineRenderSession &operator= (const OfflineRenderSession &) = delete;
    OfflineRenderSession (OfflineRenderSession &&) = delete;
    OfflineRenderSession &operator= (OfflineRenderSession &&) = delete;

  private:
    AudioEngine &engine_;

    /** Pause owned by this session; released by finish(). */
    std::unique_ptr<ProcessingPauseScope> pause_scope_;

    std::atomic<bool> finished_{ false };
  };

  auto &graph_dispatcher () { return graph_dispatcher_; }

  /**
   * @brief Acquires the processing lock, blocking until the audio thread
   * releases it.
   *
   * The lock is non-recursive: re-entrant acquisition from the same thread
   * fails loudly instead of deadlocking.
   */
  EngineProcessingLockGuard get_processing_lock () [[clang::blocking]];

  /**
   * @brief Try-acquires the processing lock without blocking.
   *
   * For the audio thread: failure to acquire is a normal outcome (e.g., the
   * main thread holds the lock during a flush or graph recalculation) and
   * the caller must skip the cycle. Re-entrant acquisition from the
   * lock-owning thread fails loudly.
   */
  EngineProcessingLockGuard
  try_get_processing_lock () noexcept [[clang::nonblocking]];

  /**
   * @brief Executes the given function after pausing processing and then
   * resumes processing.
   *
   * The function is dropped, with a warning, when an offline render owns
   * processing: it must not run until the render finishes.
   *
   * @param recalculate_graph Whether to also recreate the processing graph
   * before resuming processing.
   */
  void with_paused_processing (
    utils::InplaceCallback func,
    bool                   recalculate_graph = false);

  /**
   * @brief Runs @p func with processing suspended: the run flag is
   * cleared, the processing lock is held across the call and the run
   * flag is restored afterwards, also when @p func throws.
   *
   * Unlike a ProcessingPauseScope, this does not panic processors or
   * touch the transport: it is a lightweight suspension for callers
   * that already hold a pause or only need the processing lock.
   */
  void run_with_processing_suspended (const std::function<void ()> &func);

private:
  /**
   * Activates the audio engine to start processing and receiving events.
   *
   * @param activate Activate or deactivate.
   */
  void activate_impl (bool activate);

  /**
   * @brief Sets the engine lifecycle state (main thread only).
   */
  void set_state (State new_state);

  /**
   * @brief Marks processing as owned by an offline render (main thread
   * only).
   */
  void claim_offline_render ();

  /**
   * @brief Clears the offline-render ownership mark (main thread only).
   *
   * Releasing an unclaimed mark is an assertion failure.
   */
  void release_offline_render ();

  /**
   * @brief Updates the per-device MIDI input processors based on active
   * buffers.
   *
   * @note Must only be called with processing paused.
   *
   * @param active_buffers Map of device identifier to MidiDeviceBuffer.
   * @return Processors for devices that are no longer active, removed from
   * this engine. The current processing graph's nodes reference them until
   * the next graph recalculation releases its old node collection, so the
   * caller must keep them alive until that recalculation completes.
   */
  std::vector<utils::QObjectUniquePtr<MidiInputProcessor>>
  update_midi_processors (
    const IHardwareMidiInterface::BufferMap &active_buffers);

private:
  utils::ObjectRegistry local_registry_;

  dsp::Transport &transport_;

  /** The tempo map for timing calculations. */
  const dsp::TempoMap &tempo_map_;

  /** The processing graph router. */
  dsp::DspGraphDispatcher &graph_dispatcher_;

  IHardwareAudioInterface &hw_interface_;
  IHardwareMidiInterface  &midi_interface_;

  /**
   * Cycle count to know which cycle we are in.
   *
   * Useful for debugging.
   */
  std::atomic_uint64_t cycle_{ 0 };

  /**
   * @brief Pointer to the audio port whose buffers are copied to the audio
   * output device at the end of every processing cycle.
   *
   * Set externally via set_monitor_out_source(). This is typically the
   * monitor fader's stereo output port.
   *
   * @note This port's buffers can be re-allocated when the graph is
   * recalculated. The copy is safe because the AudioCallback lambda holds
   * the processing lock across it (graph recalculation also requires the
   * lock), and only copies after a completed processing cycle. See the
   * AudioCallback lambda for details.
   */
  dsp::AudioPort * monitor_out_source_ = nullptr;

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

  /** Thread currently holding @ref process_lock_ (default ID when free). */
  std::atomic<std::thread::id> process_lock_owner_{};

  /** Ok to process or not. */
  std::atomic_bool run_{ false };

  /**
   * @brief Number of ProcessingPauseScope objects currently open (main
   * thread only).
   *
   * A depth above 1 marks the scopes as nested; only the outermost
   * scope pauses and resumes the engine.
   */
  int pause_depth_ = 0;

  juce::AudioProcessLoadMeasurer load_measurer_;

  /**
   * @brief When first set, it is equal to the max playback latency of all
   * initial trigger nodes.
   */
  units::sample_u32_t remaining_latency_preroll_;

  /** To be set to 1 when the CC from the Midi in port should be captured. */
  std::atomic_bool capture_cc_{ false };

  /** Last MIDI CC captured. */
  std::array<midi_byte_t, 3> last_cc_captured_{};

  std::atomic<State> state_{ State::Uninitialized };
  static_assert (decltype (state_)::is_always_lock_free);
  /** @see offline_render_active() */
  std::atomic<bool> offline_render_active_{ false };

  std::optional<dsp::AudioDeviceInfo> cached_device_info_;

  utils::QObjectUniquePtr<dsp::MidiPanicProcessor> midi_panic_processor_;

  std::unique_ptr<AudioCallback> audio_callback_;

  utils::QObjectUniquePtr<AudioInputProcessor> audio_input_processor_;

  std::map<utils::Utf8String, utils::QObjectUniquePtr<MidiInputProcessor>>
    midi_input_processors_;

  /**
   * @brief Current hardware audio input channels, updated each audio callback.
   *
   * Only accessed on the audio callback thread. Set at the start of
   * process_audio(), cleared before return. AudioInputProcessor's provider
   * reads it synchronously during the same callback.
   */
  std::span<const float * const> current_hw_input_;

  /**
   * @brief Whether the audio callback is currently periodically getting
   * called.
   */
  bool audio_callback_active_{};

  /**
   * @brief Block start time captured from the audio callback, read inside
   * process() to set per-processor timestamps.
   *
   * Only accessed on the audio callback thread.
   */
  units::precise_second_t block_start_time_;
};
}
