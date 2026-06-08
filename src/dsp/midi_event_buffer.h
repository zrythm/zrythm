// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <span>
#include <vector>

#include "utils/midi.h"
#include "utils/units.h"

namespace zrythm::dsp
{

/**
 * @brief Non-owning view into a MIDI event stored in a MidiEventBuffer.
 *
 * Lightweight and trivially copyable — holds a raw pointer into the buffer's
 * contiguous storage. Only valid while the buffer is not cleared or
 * destroyed.
 */
struct MidiEventView
{
  /** Event timestamp as a sample offset within the current processing cycle. */
  units::sample_u32_t time () const noexcept [[clang::nonblocking]]
  {
    return units::samples (time_raw_);
  }

  std::span<const midi_byte_t> data () const noexcept [[clang::nonblocking]]
  {
    return { data_, size_ };
  }

private:
  friend class MidiEventBuffer;

  MidiEventView (uint32_t t, const midi_byte_t * d, uint16_t s) noexcept
      : time_raw_ (t), data_ (d), size_ (s)
  {
  }

  uint32_t            time_raw_{};
  const midi_byte_t * data_{ nullptr };
  uint16_t            size_{ 0 };
};

/**
 * @brief Packed contiguous buffer for RT MIDI events.
 *
 * Events are stored as: [uint16_t size][uint32_t timestamp][size bytes data].
 * Size-first layout allows easy chunk splitting — read 2 bytes to skip an
 * event, or splice byte ranges directly.
 *
 * Zero per-event heap allocation. Pre-allocate with reserve(), clear each
 * cycle (no dealloc). Iteration yields non-owning MidiEventView objects.
 *
 * RT-safety: reserve() must be called before any RT use. All internal
 * storage (primary, scratch, index) is pre-allocated at that point.
 * push_back(), sort(), remove_if(), and clear() never allocate as long
 * as the pre-allocated capacity is not exceeded (asserted in debug).
 *
 * Not thread-safe — one writer (RT thread), readers must synchronize
 * externally.
 */
class MidiEventBuffer
{
public:
  /** Bytes per event header: 2 (size) + 4 (timestamp). */
  static constexpr size_t kHeaderSize = sizeof (uint16_t) + sizeof (uint32_t);

  /** Default pre-allocation. */
  static constexpr size_t kDefaultReserveBytes = 0x1000;

  /** Max bytes to hold in queues. */
  static constexpr size_t kMaxReserveBytes = 0x4000;

  /**
   * @brief Create a pre-allocated buffer suitable for test use.
   *
   * The returned buffer has enough storage reserved for typical test
   * scenarios, avoiding allocations during push_back.
   */
  [[nodiscard]] static MidiEventBuffer make_reserved ()
  {
    MidiEventBuffer buf;
    buf.reserve (kDefaultReserveBytes);
    return buf;
  }

  /**
   * @brief Forward iterator over packed events.
   *
   * Yields MidiEventView on dereference. Reads the size field to advance
   * to the next event.
   */
  class Iterator
  {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = MidiEventView;
    using difference_type = std::ptrdiff_t;
    using pointer = void;
    using reference = MidiEventView;

    Iterator () = default;
    Iterator (const midi_byte_t * ptr, const midi_byte_t * end) noexcept
        : ptr_ (ptr), end_ (end)
    {
    }

    MidiEventView operator* () const noexcept
    {
      assert (ptr_ != nullptr && ptr_ < end_);
      uint16_t size{};
      std::memcpy (&size, ptr_, sizeof (uint16_t));
      uint32_t time_raw{};
      std::memcpy (&time_raw, ptr_ + sizeof (uint16_t), sizeof (uint32_t));
      return MidiEventView{ time_raw, ptr_ + kHeaderSize, size };
    }

    Iterator &operator++ () noexcept
    {
      assert (ptr_ != nullptr && ptr_ < end_);
      uint16_t size{};
      std::memcpy (&size, ptr_, sizeof (uint16_t));
      ptr_ += kHeaderSize + size;
      return *this;
    }

    Iterator operator++ (int) noexcept
    {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator== (const Iterator &other) const noexcept
    {
      return ptr_ == other.ptr_;
    }

  private:
    const midi_byte_t * ptr_ = nullptr;
    const midi_byte_t * end_ = nullptr;
  };

  /**
   * @brief Append an event. memcpy's data into the internal byte storage.
   *
   * @return true if the event was appended, false if capacity was exceeded
   *         (event dropped).
   *
   * Callers should reserve() enough space during prepare_for_processing to
   * avoid drops. Critical callers (e.g. panic processor) should check the
   * return value.
   */
  bool push_back (
    units::sample_u32_t          time,
    std::span<const midi_byte_t> event_data) noexcept [[clang::nonblocking]]
  {
    assert (event_data.size () <= std::numeric_limits<uint16_t>::max ());
    const auto total = kHeaderSize + event_data.size ();
    const auto old_size = storage_.size ();
    assert (old_size + total <= storage_.capacity ());
    if (old_size + total > storage_.capacity ())
      return false;
    storage_.resize (old_size + total);

    const auto time_raw = time.in<uint32_t> (units::samples);
    auto *     ptr = storage_.data () + old_size;
    auto       sz = static_cast<uint16_t> (event_data.size ());
    std::memcpy (ptr, &sz, sizeof (uint16_t));
    std::memcpy (ptr + sizeof (uint16_t), &time_raw, sizeof (uint32_t));
    std::memcpy (ptr + kHeaderSize, event_data.data (), event_data.size ());
    ++event_count_;
    return true;
  }

  /** Reset buffer. No deallocation — storage is reused next cycle. */
  void clear () noexcept [[clang::nonblocking]]
  {
    storage_.clear ();
    event_count_ = 0;
  }

  /** Pre-allocate internal byte storage (and scratch/index buffers). */
  void reserve (size_t bytes = kMaxReserveBytes)
  {
    storage_.reserve (bytes);
    scratch_.reserve (bytes);
    // Worst case: 1 data byte per event (e.g., MIDI clock 0xF8).
    index_.reserve (bytes / (kHeaderSize + 1));
  }

  void swap (MidiEventBuffer &other) noexcept
  {
    storage_.swap (other.storage_);
    scratch_.swap (other.scratch_);
    index_.swap (other.index_);
    std::swap (event_count_, other.event_count_);
  }

  bool empty () const noexcept { return event_count_ == 0; }
  /** Number of events (not bytes). */
  size_t size () const noexcept { return event_count_; }
  /** Bytes used in storage. */
  size_t size_in_bytes () const noexcept { return storage_.size (); }
  /** Bytes reserved in storage. */
  size_t capacity () const noexcept { return storage_.capacity (); }

  MidiEventView front () const noexcept
  {
    assert (!empty ());
    return *begin ();
  }

  Iterator begin () const noexcept
  {
    return Iterator (storage_.data (), storage_.data () + storage_.size ());
  }

  Iterator end () const noexcept
  {
    const auto * end_ptr = storage_.data () + storage_.size ();
    return Iterator (end_ptr, end_ptr);
  }

  /**
   * @brief Sort events using the given comparator on MidiEventView.
   *
   * Unpacks, sorts, and repacks. Not cheap — call only when needed.
   */
  template <typename Comp>
  void sort (Comp &&comp) noexcept [[clang::nonblocking]]
  {
    if (event_count_ <= 1)
      return;

    assert (storage_.size () <= scratch_.capacity ());
    scratch_.resize (storage_.size ());
    assert (event_count_ <= index_.capacity ());
    index_.clear ();

    size_t offset = 0;
    while (offset < storage_.size ())
      {
        uint16_t size{};
        std::memcpy (&size, storage_.data () + offset, sizeof (uint16_t));
        uint32_t time_raw{};
        std::memcpy (
          &time_raw, storage_.data () + offset + sizeof (uint16_t),
          sizeof (uint32_t));
        assert (index_.size () < index_.capacity ());
        index_.push_back ({ offset, time_raw, size });
        offset += kHeaderSize + size;
      }

    const auto * storage_ptr = storage_.data ();
    std::ranges::sort (
      index_, [storage_ptr, &comp] (const IndexEntry &a, const IndexEntry &b) {
        MidiEventView va{
          a.time_raw, storage_ptr + a.offset + kHeaderSize, a.size
        };
        MidiEventView vb{
          b.time_raw, storage_ptr + b.offset + kHeaderSize, b.size
        };
        return comp (va, vb);
      });

    size_t write_pos = 0;
    for (const auto &entry : index_)
      {
        const auto chunk = kHeaderSize + entry.size;
        std::memcpy (
          scratch_.data () + write_pos, storage_.data () + entry.offset, chunk);
        write_pos += chunk;
      }

    scratch_.resize (write_pos);
    storage_.swap (scratch_);
  }

  /** Remove all events matching the predicate. Unpacks and repacks. */
  template <typename Pred>
  void remove_if (Pred &&pred) noexcept [[clang::nonblocking]]
  {
    if (empty ())
      return;

    assert (storage_.size () <= scratch_.capacity ());
    scratch_.resize (storage_.size ());

    size_t       write_pos = 0;
    size_t       new_count = 0;
    const auto * read_ptr = storage_.data ();
    const auto * end_ptr = storage_.data () + storage_.size ();

    while (read_ptr < end_ptr)
      {
        uint16_t size{};
        std::memcpy (&size, read_ptr, sizeof (uint16_t));
        uint32_t time_raw{};
        std::memcpy (&time_raw, read_ptr + sizeof (uint16_t), sizeof (uint32_t));

        MidiEventView view{ time_raw, read_ptr + kHeaderSize, size };

        if (!pred (view))
          {
            const auto chunk = kHeaderSize + size;
            std::memcpy (scratch_.data () + write_pos, read_ptr, chunk);
            write_pos += chunk;
            ++new_count;
          }

        read_ptr += kHeaderSize + size;
      }

    if (new_count == event_count_)
      return;

    scratch_.resize (write_pos);
    storage_.swap (scratch_);
    event_count_ = new_count;
  }

private:
  struct IndexEntry
  {
    size_t   offset{};
    uint32_t time_raw{};
    uint16_t size{};
  };

  std::vector<midi_byte_t> storage_;
  std::vector<midi_byte_t> scratch_;
  std::vector<IndexEntry>  index_;
  size_t                   event_count_ = 0;
};

namespace midi_event
{

inline void
sort_with_note_off_priority (MidiEventBuffer &buf) noexcept
  [[clang::nonblocking]]
{
  buf.sort ([] (const MidiEventView &a, const MidiEventView &b) {
    if (a.time () != b.time ())
      return a.time () < b.time ();
    return !utils::midi::midi_is_note_on (a.data ())
           && utils::midi::midi_is_note_on (b.data ());
  });
}

/**
 * @brief Append events from @p src to @p dst, filtered by time range
 * [range.first, range.second).
 */
inline void
append_in_range (
  MidiEventBuffer                                    &dst,
  const MidiEventBuffer                              &src,
  std::pair<units::sample_u32_t, units::sample_u32_t> range) noexcept
  [[clang::nonblocking]]
{
  for (const auto &ev : src)
    {
      if (ev.time () >= range.first && ev.time () < range.second)
        dst.push_back (ev.time (), ev.data ());
    }
}

} // namespace midi_event

} // namespace zrythm::dsp
