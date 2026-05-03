// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>

#include "controllers/audio_recording_session.h"
#include "utils/float_ranges.h"

#include <farbot/fifo.hpp>

namespace zrythm::controllers
{

struct AudioRecordingSession::Impl
{
  using IndexFifo = farbot::fifo<
    size_t,
    farbot::fifo_options::concurrency::single,
    farbot::fifo_options::concurrency::single,
    farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty,
    farbot::fifo_options::full_empty_failure_mode::return_false_on_full_or_empty>;

  // One extra slot beyond fifo capacity so the writer never wraps into a
  // slot whose index is still in the fifo.  The fifo limits in-flight
  // indices to kFifoCapacity, so with +1 the active write slot is always
  // distinct from any un-drained slot.
  static constexpr size_t kSlotCount = AudioRecordingSession::kFifoCapacity + 1;

  explicit Impl (units::sample_u32_t max_block_length)
      : index_buffer (static_cast<int> (AudioRecordingSession::kFifoCapacity)),
        max_block_length (max_block_length)
  {
    const auto block_size = max_block_length.in (units::samples);
    slots.resize (kSlotCount);
    for (auto &slot : slots)
      {
        slot.l_frames.resize (block_size);
        slot.r_frames.resize (block_size);
      }
  }

  std::vector<RecordingAudioPacket> slots;
  IndexFifo                         index_buffer;
  size_t                            next_write_slot = 0;
  const units::sample_u32_t         max_block_length;
};

AudioRecordingSession::AudioRecordingSession (
  units::sample_u32_t max_block_length)
    : impl_ (std::make_unique<Impl> (max_block_length))
{
}

AudioRecordingSession::~AudioRecordingSession () = default;

void
AudioRecordingSession::write_samples (
  units::sample_t        timeline_position,
  std::span<const float> l_data,
  std::span<const float> r_data) noexcept
{
  State expected = State::Armed;
  state_.compare_exchange_strong (
    expected, State::Capturing, std::memory_order_acq_rel);

  if (state_.load (std::memory_order_acquire) == State::Finalizing)
    return;
  assert (l_data.size () == r_data.size ());
  assert (
    l_data.size ()
    <= static_cast<size_t> (impl_->max_block_length.in (units::samples)));
  const size_t slot_idx = impl_->next_write_slot % impl_->slots.size ();
  auto        &slot = impl_->slots[slot_idx];
  slot.timeline_position = timeline_position;
  slot.nframes = units::samples (l_data.size ());
  const auto n = l_data.size ();
  utils::float_ranges::copy (std::span (slot.l_frames).subspan (0, n), l_data);
  utils::float_ranges::copy (std::span (slot.r_frames).subspan (0, n), r_data);

  auto push_idx = slot_idx;
  if (impl_->index_buffer.push (std::move (push_idx)))
    {
      ++impl_->next_write_slot;
    }
  else
    {
      dropped_packets_.fetch_add (1, std::memory_order_relaxed);
    }
}

std::vector<RecordingAudioPacket>
AudioRecordingSession::drain_pending ()
{
  std::vector<RecordingAudioPacket> packets;
  packets.reserve (kFifoCapacity);

  size_t slot_idx{};
  while (impl_->index_buffer.pop (slot_idx))
    {
      const auto          &slot = impl_->slots[slot_idx];
      RecordingAudioPacket packet;
      packet.timeline_position = slot.timeline_position;
      packet.nframes = slot.nframes;
      const auto n = slot.nframes.in<size_t> (units::samples);
      packet.l_frames.assign (
        slot.l_frames.begin (), slot.l_frames.begin () + n);
      packet.r_frames.assign (
        slot.r_frames.begin (), slot.r_frames.begin () + n);
      packets.push_back (std::move (packet));
    }

  return packets;
}

void
AudioRecordingSession::finalize ()
{
  state_.store (State::Finalizing, std::memory_order_release);
}

void
AudioRecordingSession::reset ()
{
  state_.store (State::Armed, std::memory_order_release);
  dropped_packets_.store (0, std::memory_order_relaxed);
  impl_->next_write_slot = 0;
  recorded_regions_.clear ();
  current_region_ = nullptr;

  size_t dummy{};
  while (impl_->index_buffer.pop (dummy))
    {
    }
}
}
