// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include "plugins/lv2_ui_event_ring.h"

#include <juce_core/juce_core.h>

namespace zrythm::plugins
{

/**
 * @brief Largest payload a worker queue record may carry.
 *
 * Records are copied into fixed-size scratch storage on the reading
 * side, so payloads above this bound are refused at push time.
 */
inline constexpr uint32_t kMaxWorkerRecordSize = 64 * 1024;

/**
 * @brief Bounded single-producer/single-consumer queue of
 * variable-size byte records for LV2 worker traffic.
 *
 * Records are framed as a uint32 size prefix followed by the payload.
 * Producers and consumers never allocate or lock; a record is only
 * visible once fully written, and a failed push leaves the queue
 * untouched. The producer role may be handed over between threads
 * only at points where the previous producer has stopped.
 */
class Lv2WorkerQueue
{
public:
  /**
   * @brief Constructs a queue holding at most @p capacity_bytes of
   * framed records.
   *
   * @param capacity_bytes Total byte capacity for framed records.
   */
  explicit Lv2WorkerQueue (size_t capacity_bytes)
      : fifo_ (static_cast<int> (capacity_bytes)),
        data_ (static_cast<size_t> (fifo_.getTotalSize ()))
  {
  }

  /**
   * @brief Appends @p payload as one framed record.
   *
   * @return False when the payload exceeds @ref kMaxWorkerRecordSize or
   * the ring holds fewer free bytes than the framed record needs; the
   * queue is left untouched and the failure is counted.
   */
  bool push (std::span<const std::byte> payload) noexcept
  {
    const auto size = static_cast<uint32_t> (payload.size ());
    if (size > kMaxWorkerRecordSize || (payload.data () == nullptr && size > 0))
      {
        dropped_records_.fetch_add (1, std::memory_order_relaxed);
        return false;
      }
    const auto header =
      std::span{ reinterpret_cast<const std::byte *> (&size), sizeof (size) };
    if (!fifo_write_record (fifo_, data_, header, payload))
      {
        dropped_records_.fetch_add (1, std::memory_order_relaxed);
        return false;
      }
    return true;
  }

  /**
   * @brief Pops the oldest record into @p payload_out.
   *
   * @param[out] size_out Receives the record's payload size.
   * @param payload_out Buffer of at least @ref kMaxWorkerRecordSize
   * bytes.
   * @return False when the queue holds no complete record.
   */
  bool pop (std::span<std::byte> payload_out, uint32_t &size_out) noexcept
  {
    std::byte header[sizeof (uint32_t)];
    if (!fifo_read_bytes (fifo_, data_, { header, sizeof (header) }))
      return false;
    uint32_t size = 0;
    std::memcpy (&size, header, sizeof (size));
    // A record is published whole, so its size always fits the
    // caller's scratch buffer
    assert (size <= payload_out.size ());
    // The record was published whole, so its payload bytes are present
    [[maybe_unused]] const bool payload_read = fifo_read_bytes (
      fifo_, data_, { payload_out.data (), static_cast<size_t> (size) });
    // A partial record would leave the stream misaligned: the framing
    // guarantees this cannot happen
    assert (payload_read);
    size_out = size;
    return true;
  }

  /**
   * @brief Returns the number of records refused by @ref push.
   */
  [[nodiscard]] uint32_t dropped_records () const noexcept
  {
    return dropped_records_.load (std::memory_order_relaxed);
  }

  /**
   * @brief Returns true while no complete record is queued. A false
   * result is authoritative; a true result may race with a concurrent
   * push.
   */
  [[nodiscard]] bool empty () const noexcept
  {
    return fifo_.getNumReady () == 0;
  }

  /**
   * @brief Clears the queue. Callers must ensure no producer or
   * consumer is active.
   */
  void reset () noexcept
  {
    fifo_.reset ();
    dropped_records_.store (0, std::memory_order_relaxed);
  }

private:
  juce::AbstractFifo     fifo_;
  std::vector<std::byte> data_;
  std::atomic<uint32_t>  dropped_records_{ 0 };
};

} // namespace zrythm::plugins
