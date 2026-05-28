// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>
#include <concepts>
#include <memory>
#include <vector>

#include "controllers/recording_audio_packet.h"
#include "controllers/recording_midi_packet.h"
#include "utils/units.h"

#include <QtClassHelperMacros>

namespace zrythm::controllers
{

template <typename T>
concept RecordingPacket =
  requires (T &slot, const T &source, units::sample_u32_t block_length) {
    { T::copy_from (slot, source) };
    { T::resize (slot, block_length) };
  };

/**
 * @brief Per-track recording state with a lock-free SPSC queue.
 *
 * The RT thread writes packets into pre-allocated slots and pushes slot indices
 * onto a farbot SPSC fifo. The non-RT timer drains pending indices and reads
 * the corresponding slot data into Packet objects.
 *
 * @tparam Packet The packet type (RecordingAudioPacket or RecordingMidiPacket).
 * Must provide static copy_from(slot, source) and resize(slot, block_length).
 *
 * Thread contracts:
 * - write(): audio thread only (single producer)
 * - drain_pending(): timer thread only (single consumer)
 * - finalize(): any thread (atomic flag — safe during active writes)
 * - reset(): any thread (must not overlap with write/drain)
 */
template <RecordingPacket Packet> class RecordingSession
{
public:
  enum class State : uint8_t
  {
    Armed,
    Capturing,
    Finalizing,
  };

  explicit RecordingSession (units::sample_u32_t max_block_length);
  ~RecordingSession ();

  Q_DISABLE_COPY_MOVE (RecordingSession)

  static constexpr size_t kFifoCapacity = 1024;

  using PacketType = Packet;

  /**
   * @brief Prepares internal buffers for processing at the given block length.
   *
   * Must be called before audio processing starts (not concurrently with
   * write() or drain_pending()).
   */
  void prepare_for_processing (units::sample_u32_t block_length);

  /**
   * @brief RT-safe: writes audio data into the ring buffer.
   *
   * Called from the audio thread. No allocations — copies data into
   * pre-allocated slot memory and pushes the slot index onto the SPSC fifo.
   */
  void write (
    units::sample_t        timeline_position,
    bool                   transport_recording,
    std::span<const float> l_data,
    std::span<const float> r_data) noexcept [[clang::nonblocking]]
    requires std::same_as<Packet, RecordingAudioPacket>;

  /**
   * @brief RT-safe: writes MIDI events into the ring buffer.
   *
   * Called from the audio thread. No allocations — copies events into
   * pre-allocated slot memory and pushes the slot index onto the SPSC fifo.
   */
  void write (
    units::sample_t                 timeline_position,
    bool                            transport_recording,
    std::span<const dsp::MidiEvent> midi_events,
    units::sample_u32_t             nframes) noexcept [[clang::nonblocking]]
    requires std::same_as<Packet, RecordingMidiPacket>;

  /**
   * @brief Non-RT: drains all pending packets from the ring buffer.
   *
   * Called from the timer thread. Reads slot indices from the SPSC fifo
   * and copies the slot data into Packet objects.
   */
  [[nodiscard]] std::vector<Packet> drain_pending () [[clang::blocking]];

  [[nodiscard]] auto state () const
  {
    return state_.load (std::memory_order_acquire);
  }

  /**
   * @brief Transitions to Finalizing state, rejecting further writes.
   */
  void finalize () noexcept [[clang::nonblocking]];

  /**
   * @brief Resets the session to Armed state for reuse.
   */
  void reset ();

  [[nodiscard]] uint64_t dropped_packets () const
  {
    return dropped_packets_.load (std::memory_order_relaxed);
  }

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;

  std::atomic<State>    state_{ State::Armed };
  std::atomic<uint64_t> dropped_packets_{ 0 };
};

using AudioRecordingSession = RecordingSession<RecordingAudioPacket>;
using MidiRecordingSession = RecordingSession<RecordingMidiPacket>;

}
