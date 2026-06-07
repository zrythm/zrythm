// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>

#include "controllers/recording_session.h"

#include <farbot/fifo.hpp>

namespace zrythm::controllers
{

template <RecordingPacket Packet> struct RecordingSession<Packet>::Impl
{
  using IndexFifo = farbot::fifo<
    size_t,
    farbot::fifo_options::concurrency::single,
    farbot::fifo_options::concurrency::single,
    farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
    farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty>;

  static constexpr size_t kSlotCount =
    RecordingSession<Packet>::kFifoCapacity + 2;

  explicit Impl (units::sample_u32_t max_block_length_arg)
      : index_buffer (static_cast<int> (RecordingSession<Packet>::kFifoCapacity)),
        max_block_length (max_block_length_arg)
  {
    slots.resize (kSlotCount);
    for (auto &slot : slots)
      {
        Packet::resize (slot, max_block_length);
      }
  }

  /** Pre-allocated packet slots written by RT thread (lock-free ring). */
  std::vector<Packet> slots;
  /** SPSC FIFO passing written slot indices from RT to non-RT thread. */
  IndexFifo index_buffer;
  /** Monotonically increasing slot index (wraps via modulo). Atomic for
      thread-safe access if the RT thread is interleaved with reset(). */
  std::atomic<size_t> next_write_slot{ 0 };
  /** Current max block length; slots are resized if this grows. */
  units::sample_u32_t max_block_length;
};

template <RecordingPacket Packet>
RecordingSession<Packet>::RecordingSession (units::sample_u32_t max_block_length)
    : impl_ (std::make_unique<Impl> (max_block_length))
{
}

template <RecordingPacket Packet>
RecordingSession<Packet>::~RecordingSession () = default;

template <RecordingPacket Packet>
void
RecordingSession<Packet>::prepare_for_processing (
  units::sample_u32_t block_length)
{
  assert (block_length > units::samples (0u));
  if (block_length > impl_->max_block_length)
    {
      for (auto &slot : impl_->slots)
        {
          Packet::resize (slot, block_length);
        }
    }
  impl_->max_block_length = block_length;
}

template <RecordingPacket Packet>
void
RecordingSession<Packet>::write (
  units::sample_t        timeline_position,
  bool                   transport_recording,
  std::span<const float> l_data,
  std::span<const float> r_data) noexcept
  requires std::same_as<Packet, RecordingAudioPacket>
{
  State expected = State::Armed;
  state_.compare_exchange_strong (
    expected, State::Capturing, std::memory_order_acq_rel);

  if (state_.load (std::memory_order_acquire) == State::Finalizing)
    return;

  assert (l_data.size () == r_data.size ());
  const size_t slot_idx =
    impl_->next_write_slot.load (std::memory_order_relaxed)
    % impl_->slots.size ();
  auto &slot = impl_->slots[slot_idx];
  Packet::write_to_slot (
    slot, timeline_position, transport_recording, l_data, r_data);

  auto push_idx = slot_idx;
  if (impl_->index_buffer.push (std::move (push_idx)))
    {
      impl_->next_write_slot.fetch_add (1, std::memory_order_relaxed);
    }
  else
    {
      dropped_packets_.fetch_add (1, std::memory_order_relaxed);
    }
}

template <RecordingPacket Packet>
void
RecordingSession<Packet>::write (
  units::sample_t             timeline_position,
  bool                        transport_recording,
  const dsp::MidiEventBuffer &midi_events,
  units::sample_u32_t         nframes_arg) noexcept
  requires std::same_as<Packet, RecordingMidiPacket>
{
  State expected = State::Armed;
  state_.compare_exchange_strong (
    expected, State::Capturing, std::memory_order_acq_rel);

  if (state_.load (std::memory_order_acquire) == State::Finalizing)
    return;

  const size_t slot_idx =
    impl_->next_write_slot.load (std::memory_order_relaxed)
    % impl_->slots.size ();
  auto &slot = impl_->slots[slot_idx];
  Packet::write_to_slot (
    slot, timeline_position, transport_recording, midi_events, nframes_arg);

  auto push_idx = slot_idx;
  if (impl_->index_buffer.push (std::move (push_idx)))
    {
      impl_->next_write_slot.fetch_add (1, std::memory_order_relaxed);
    }
  else
    {
      dropped_packets_.fetch_add (1, std::memory_order_relaxed);
    }
}

template <RecordingPacket Packet>
std::vector<Packet>
RecordingSession<Packet>::drain_pending ()
{
  std::vector<Packet> packets;
  packets.reserve (kFifoCapacity);

  size_t slot_idx{};
  while (impl_->index_buffer.pop (slot_idx))
    {
      const auto &slot = impl_->slots[slot_idx];
      Packet      packet;
      Packet::resize (packet, slot.nframes);
      Packet::copy_from (packet, slot);
      packets.push_back (std::move (packet));
    }

  return packets;
}

template <RecordingPacket Packet>
void
RecordingSession<Packet>::finalize () noexcept
{
  state_.store (State::Finalizing, std::memory_order_release);
}

template <RecordingPacket Packet>
void
RecordingSession<Packet>::reset ()
{
  impl_->next_write_slot.store (0, std::memory_order_release);
  dropped_packets_.store (0, std::memory_order_relaxed);
  state_.store (State::Armed, std::memory_order_release);

  size_t dummy{};
  while (impl_->index_buffer.pop (dummy))
    {
    }
}

template class RecordingSession<RecordingAudioPacket>;
template class RecordingSession<RecordingMidiPacket>;

}
