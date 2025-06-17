// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/tempo_map.h"

namespace zrythm::dsp
{

/**
 * @class Playhead
 * @brief Provides thread-safe playhead positioning with sample-accurate timing.
 *
 * The Playhead class maintains synchronized position information in both
 * samples (for audio processing) and ticks (for musical operations). It
 * guarantees real-time safety for audio threads while allowing GUI updates.
 *
 * @startuml
 * title Playhead Thread Interactions
 *
 * box "Audio Thread" #LightBlue
 * participant AudioEngine
 * participant Playhead
 * end box
 *
 * box "GUI Thread" #LightGreen
 * participant TimelineView
 * end box
 *
 * group Audio Processing Block
 * AudioEngine -> Playhead : prepareForProcessing(nframes)
 * activate Playhead
 *
 * loop For each sample
 *     AudioEngine -> Playhead : positionDuringProcessing()
 *     AudioEngine -> Playhead : advanceProcessing(1)
 * end
 *
 * AudioEngine -> Playhead : finalizeProcessing()
 * deactivate Playhead
 * end
 *
 * group User Interaction
 * TimelineView -> Playhead : setPositionTicks(ticks)
 * activate Playhead
 * Playhead --> TimelineView :
 * deactivate Playhead
 * end
 *
 * group Periodic Update
 * TimelineView -> Playhead : positionTicks()
 * activate Playhead
 * Playhead --> TimelineView : current ticks
 * deactivate Playhead
 * end
 *
 * @enduml
 */
class Playhead
{
  friend class PlayheadProcessingGuard;

public:
  /**
   * @brief Construct a Playhead associated with a TempoMap
   * @param tempo_map Reference to project's TempoMap
   */
  Playhead (const TempoMap &tempo_map) : tempo_map_ (tempo_map) { }

  // Audio threads ONLY ----------------------------------------------------

private:
  /**
   * @brief Prepare for audio processing block (audio thread safe)
   *
   * @note Must be called at start of audio block processing
   * @warning Must only be called from the system audio callback thread.
   */
  void prepare_for_processing () noexcept
  {
    // Copy current position before processing
    position_samples_processing_.store (
      position_samples_.load (std::memory_order_acquire),
      std::memory_order_release);
  }

public:
  /**
   * @brief Advance processing position (audio thread safe)
   * @param nframes Number of frames advanced (negative for backwards, eg when
   * looping back)
   *
   * @warning Must only be called from the system audio callback thread.
   */
  void advance_processing (int64_t nframes) noexcept
  {
    if (nframes >= 0) [[likely]]
      {
        position_samples_processing_.fetch_add (static_cast<double> (nframes));
      }
    else
      {
        position_samples_processing_.fetch_sub (static_cast<double> (-nframes));
      }
  }

  /**
   * @brief Get current position during processing (audio thread safe)
   * @return Position in samples as double
   *
   * @note Can be called concurrently from multiple audio threads
   */
  auto position_during_processing_precise () const noexcept
  {
    return position_samples_processing_.load (std::memory_order_acquire);
  }
  auto position_during_processing_rounded () const noexcept
  {
    return static_cast<uint64_t> (std::round (
      position_samples_processing_.load (std::memory_order_acquire)));
  }

private:
  /**
   * @brief Finalize audio block processing (audio thread safe)
   *
   * @note Commits final position atomically.
   * @warning Must only be called from the system audio callback thread.
   */
  void finalize_processing () noexcept
  {
    // Commit position only at end of block
    position_samples_.store (
      position_samples_processing_.load (std::memory_order_acquire),
      std::memory_order_release);
  }

  // GUI thread ONLY ------------------------------------------------------

public:
  /**
   * @brief Set playhead position in musical ticks (GUI thread only)
   * @param ticks New position in ticks
   *
   * @note Uses mutex to synchronize with GUI thread
   */
  void set_position_ticks (double ticks) [[clang::blocking]]
  {
    std::lock_guard lock (position_mutex_);
    position_ticks_ = ticks;
    position_samples_.store (
      tempo_map_.tick_to_samples (ticks), std::memory_order_release);
  }

  /**
   * @brief Get current playhead position in ticks (GUI thread only)
   * @return Position in ticks
   */
  double position_ticks () const [[clang::blocking]]
  {
    std::lock_guard lock (position_mutex_);
    return position_ticks_;
  }

  // Any thread (non-RT) --------------------------------------------------

  /**
   * @brief Update tick position from sample position
   *
   * To be called periodically as part of the GUI event loop to synchronize
   * positions
   */
  void update_ticks_from_samples () [[clang::blocking]]
  {
    std::lock_guard lock (position_mutex_);
    position_ticks_ = tempo_map_.samples_to_tick (
      position_samples_.load (std::memory_order_acquire));
  }

  const auto &get_tempo_map () const { return tempo_map_; }

  /**
   * @brief Get current playhead position in samples (non-RT)
   * @note For testing and debugging only
   */
  double position_samples_FOR_TESTING () const noexcept
  {
    return position_samples_.load (std::memory_order_acquire);
  }

private:
  static constexpr auto kTicks = "ticks"sv;
  friend void           to_json (nlohmann::json &j, const Playhead &pos)
  {
    j[kTicks] = pos.position_ticks ();
  }
  friend void from_json (const nlohmann::json &j, Playhead &pos)
  {
    double ticks{};
    j.at (kTicks).get_to (ticks);
    pos.set_position_ticks (ticks);
  }

private:
  const TempoMap &tempo_map_;

  // Audio thread state
  std::atomic<double> position_samples_processing_ = 0.0;

  // Shared state (protected)
  std::atomic<double> position_samples_{ 0.0 };
  double              position_ticks_ = 0.0;
  mutable std::mutex  position_mutex_;
};

/**
 * @class PlayheadProcessingGuard
 * @brief RAII helper for Playhead audio processing block.
 *
 * Automatically calls prepare_for_processing() on construction
 * and finalize_processing() on destruction.
 */
class PlayheadProcessingGuard
{
public:
  /**
   * @brief Constructor - calls playhead.prepare_for_processing()
   * @param playhead Playhead instance to manage
   */
  explicit PlayheadProcessingGuard (Playhead &playhead) noexcept
      : playhead_ (playhead)
  {
    playhead_.prepare_for_processing ();
  }

  /// @brief Destructor - calls playhead.finalize_processing()
  ~PlayheadProcessingGuard () noexcept { playhead_.finalize_processing (); }

  // Prevent copying
  PlayheadProcessingGuard (const PlayheadProcessingGuard &) = delete;
  PlayheadProcessingGuard &operator= (const PlayheadProcessingGuard &) = delete;

private:
  Playhead &playhead_;
};

} // namespace zrythm::dsp
