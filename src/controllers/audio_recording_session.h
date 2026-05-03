// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>
#include <memory>
#include <span>
#include <vector>

#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/arranger_object_fwd.h"
#include "utils/units.h"

#include <QPointer>

namespace zrythm::controllers
{

/**
 * @brief A single packet of recorded audio data transferred from RT to non-RT.
 */
struct RecordingAudioPacket
{
  units::sample_t     timeline_position;
  units::sample_u32_t nframes;
  std::vector<float>  l_frames;
  std::vector<float>  r_frames;
};

/**
 * @brief Per-track recording state with a lock-free SPSC queue.
 *
 * The RT thread writes audio into pre-allocated slots and pushes slot indices
 * onto a farbot SPSC fifo. The non-RT timer drains pending indices and reads
 * the corresponding slot data into RecordingAudioPacket objects.
 *
 * Thread contracts:
 * - write_samples(): audio thread only (single producer)
 * - drain_pending(): timer thread only (single consumer)
 * - finalize(), reset(): any thread (must not overlap with write/drain)
 */
class AudioRecordingSession
{
public:
  enum class State : uint8_t
  {
    Armed,
    Capturing,
    Finalizing,
  };

  explicit AudioRecordingSession (units::sample_u32_t max_block_length);
  ~AudioRecordingSession ();

  Q_DISABLE_COPY_MOVE (AudioRecordingSession)

  static constexpr size_t kFifoCapacity = 64;

  /**
   * @brief RT-safe: writes audio data into the ring buffer.
   *
   * Called from the audio thread. No allocations — copies data into
   * pre-allocated slot memory and pushes the slot index onto the SPSC fifo.
   * If the fifo is full, the slot is overwritten with the latest data but
   * the index is not queued and the dropped counter is incremented. The
   * next call reuses the same slot, so once the consumer drains enough
   * entries the most recent audio is always captured.
   */
  void write_samples (
    units::sample_t        timeline_position,
    std::span<const float> l_data,
    std::span<const float> r_data) noexcept [[clang::nonblocking]];

  /**
   * @brief Non-RT: drains all pending packets from the ring buffer.
   *
   * Called from the timer thread. Reads slot indices from the SPSC fifo
   * and copies the slot data into RecordingAudioPacket objects.
   */
  std::vector<RecordingAudioPacket> drain_pending () [[clang::blocking]];

  [[nodiscard]] auto state () const
  {
    return state_.load (std::memory_order_acquire);
  }

  /**
   * @brief Transitions to Finalizing state, rejecting further writes.
   */
  void finalize ();

  /**
   * @brief Resets the session to Armed state for reuse.
   *
   * Must only be called when neither write_samples() nor drain_pending()
   * are active. Clears all buffered data, recorded regions, and counters.
   */
  void reset ();

  /**
   * @brief Returns the number of packets dropped due to fifo overflow.
   */
  [[nodiscard]] uint64_t dropped_packets () const
  {
    return dropped_packets_.load (std::memory_order_relaxed);
  }

  /**
   * @brief Returns the UUIDs of all recorded regions.
   */
  const std::vector<structure::arrangement::ArrangerObject::Uuid> &
  recorded_regions () const
  {
    return recorded_regions_;
  }

  /**
   * @brief Registers a region UUID created during this recording session.
   */
  void add_recorded_region (structure::arrangement::ArrangerObject::Uuid id)
  {
    recorded_regions_.push_back (id);
  }

  /**
   * @brief Returns the region currently being recorded into, or nullptr.
   */
  [[nodiscard]] structure::arrangement::AudioRegion * current_region () const
  {
    return current_region_;
  }
  /**
   * @brief Sets the region currently being recorded into.
   */
  void set_current_region (structure::arrangement::AudioRegion * region)
  {
    current_region_ = region;
  }

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  std::atomic<State>    state_{ State::Armed };
  std::atomic<uint64_t> dropped_packets_{ 0 };

  std::vector<structure::arrangement::ArrangerObject::Uuid> recorded_regions_;
  QPointer<structure::arrangement::AudioRegion>             current_region_;
};

}
