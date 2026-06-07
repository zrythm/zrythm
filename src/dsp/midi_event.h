// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <variant>

#include "utils/midi.h"
#include "utils/traits.h"
#include "utils/units.h"

namespace zrythm::dsp
{
/** Max events to hold in queues. */
constexpr int MAX_MIDI_EVENTS = 2560;

/**
 * @brief Type-erased MIDI event with small-buffer optimization.
 *
 * Stores channel messages (≤3 bytes) inline in a fixed-size array sized to
 * sizeof(pointer), and larger messages (e.g. sysex) via shared
 * heap storage.
 *
 * @note Not realtime-safe. use MidiEventBuffer in RT contexts.
 *
 * @tparam TimeT The timestamp type (e.g. units::sample_t,
 * units::precise_tick_t).
 */
template <typename TimeT> struct MidiEvent
{
  using InlineStorage = std::array<midi_byte_t, sizeof (const midi_byte_t *)>;
  using ExternalStorage = std::shared_ptr<const midi_byte_t[]>;
  using TimeType = TimeT;

  static constexpr size_t inline_capacity = InlineStorage{}.size ();

  // Note: ExternalStorage thread safety needs auditing before use in
  // RT contexts. Inline storage (channel messages) is always RT-safe.

  std::variant<InlineStorage, ExternalStorage> storage;
  uint16_t size_{}; ///< Number of valid MIDI bytes
  TimeType time_{}; ///< Timestamp (meaning depends on TimeType)

  MidiEvent () = default;

  /**
   * @brief Converting constructor for compatible time types.
   *
   * Allows implicit conversion e.g. from MidiEvent<Quantity<Sample, int>>
   * to MidiEvent<Quantity<Sample, long>> (SampleBasedMidiEvent).
   */
  template <typename OtherTimeType>
    requires std::constructible_from<TimeType, OtherTimeType>
               && (!std::same_as<TimeType, OtherTimeType>)
  MidiEvent (const MidiEvent<OtherTimeType> &other) noexcept
      : storage (other.storage), size_ (other.size_),
        time_ (static_cast<TimeType> (other.time_))
  {
  }

  std::span<const midi_byte_t> data () const noexcept [[clang::nonblocking]]
  {
    return {
      std::visit (
        [] (const auto &s) -> const midi_byte_t * {
          if constexpr (
            std::is_same_v<std::decay_t<decltype (s)>, InlineStorage>)
            return s.data ();
          else
            return s.get ();
        },
        storage),
      size_
    };
  }

  void set_inline (std::span<const midi_byte_t> d) noexcept [[clang::blocking]]
  {
    assert (d.size () <= inline_capacity);
    size_ = static_cast<uint16_t> (d.size ());
    if (!std::holds_alternative<InlineStorage> (storage))
      storage = InlineStorage{};
    auto &arr = std::get<InlineStorage> (storage);
    std::ranges::copy (d, arr.begin ());
  }

  /**
   * @brief RT-safe version of set_inline that only works on inline storage.
   *
   * Asserts if the variant currently holds external storage — this must
   * only be called on freshly-constructed or already-inline events.
   */
  void
  set_inline_rt (std::span<const midi_byte_t> d) noexcept [[clang::nonblocking]]
  {
    assert (d.size () <= inline_capacity);
    assert (std::holds_alternative<InlineStorage> (storage));
    size_ = static_cast<uint16_t> (d.size ());
    auto &arr = std::get<InlineStorage> (storage);
    std::ranges::copy (d, arr.begin ());
  }

  void
  set_external (std::shared_ptr<const midi_byte_t[]> ptr, uint16_t sz) noexcept
    [[clang::blocking]]
  {
    size_ = sz;
    storage = std::move (ptr);
  }

  bool is_inline () const noexcept
  {
    return std::holds_alternative<InlineStorage> (storage);
  }

  friend bool operator== (const MidiEvent &lhs, const MidiEvent &rhs) noexcept
  {
    if (lhs.time_ != rhs.time_ || lhs.size_ != rhs.size_)
      return false;
    const auto ld = lhs.data ();
    const auto rd = rhs.data ();
    return std::ranges::equal (ld, rd);
  }
};

/// Cache/playback: absolute sample position (int64, rounded from ticks).
using SampleBasedMidiEvent = MidiEvent<units::sample_t>;
/// Serialization: tick position (double precision).
using TickBasedMidiEvent = MidiEvent<units::precise_tick_t>;
/// RT buffers: relative frame offset within a processing cycle.
using RealtimeMidiEvent = MidiEvent<units::sample_u32_t>;

/// Factory functions and algorithms for MidiEvent containers.
namespace midi_event
{

/**
 * @brief Creates a note on event.
 *
 * @param channel MIDI channel (0-based, 0-15).
 */
template <typename TimeType>
MidiEvent<TimeType>
make_note_on (
  midi_byte_t channel,
  midi_byte_t note_pitch,
  midi_byte_t velocity,
  TimeType    time)
{
  assert (channel <= 15);
  const std::array<midi_byte_t, 3> raw = {
    static_cast<midi_byte_t> (utils::midi::MIDI_CH1_NOTE_ON | channel),
    note_pitch, velocity
  };
  MidiEvent<TimeType> ev;
  ev.set_inline_rt (raw);
  ev.time_ = time;
  assert (utils::midi::midi_is_note_on (ev.data ()));
  return ev;
}

/**
 * @brief Creates a note off event.
 *
 * @param channel MIDI channel (0-based, 0-15).
 */
template <typename TimeType>
MidiEvent<TimeType>
make_note_off (midi_byte_t channel, midi_byte_t note_pitch, TimeType time)
{
  assert (channel <= 15);
  const std::array<midi_byte_t, 3> raw = {
    static_cast<midi_byte_t> (utils::midi::MIDI_CH1_NOTE_OFF | channel),
    note_pitch, 0
  };
  MidiEvent<TimeType> ev;
  ev.set_inline_rt (raw);
  ev.time_ = time;
  assert (utils::midi::midi_is_note_off (ev.data ()));
  return ev;
}

/**
 * @brief Creates a control change event.
 *
 * @param channel MIDI channel (0-based, 0-15).
 */
template <typename TimeType>
MidiEvent<TimeType>
make_control_change (
  midi_byte_t channel,
  midi_byte_t controller,
  midi_byte_t value,
  TimeType    time)
{
  assert (channel <= 15);
  const std::array<midi_byte_t, 3> raw = {
    static_cast<midi_byte_t> (0xB0 | channel), controller, value
  };
  MidiEvent<TimeType> ev;
  ev.set_inline_rt (raw);
  ev.time_ = time;
  return ev;
}

/**
 * @brief Creates a pitchbend event.
 *
 * @param channel MIDI channel (0-based, 0-15).
 * @param pitchbend 0 to 16384 (8192 = center).
 */
template <typename TimeType>
MidiEvent<TimeType>
make_pitchbend (midi_byte_t channel, uint32_t pitchbend, TimeType time)
{
  assert (channel <= 15);
  assert (pitchbend <= 16384);
  const std::array<midi_byte_t, 3> raw = {
    static_cast<midi_byte_t> (0xE0 | channel),
    static_cast<midi_byte_t> (pitchbend & 0x7F),
    static_cast<midi_byte_t> ((pitchbend >> 7) & 0x7F)
  };
  MidiEvent<TimeType> ev;
  ev.set_inline_rt (raw);
  ev.time_ = time;
  return ev;
}

/**
 * @brief Creates a channel pressure (aftertouch) event.
 *
 * @param channel MIDI channel (0-based, 0-15).
 */
template <typename TimeType>
MidiEvent<TimeType>
make_channel_pressure (midi_byte_t channel, midi_byte_t value, TimeType time)
{
  assert (channel <= 15);
  const std::array<midi_byte_t, 2> raw = {
    static_cast<midi_byte_t> (0xD0 | channel), value
  };
  MidiEvent<TimeType> ev;
  ev.set_inline_rt (raw);
  ev.time_ = time;
  return ev;
}

/**
 * @brief Creates an all-notes-off event.
 *
 * @param channel MIDI channel (0-based, 0-15).
 */
template <typename TimeType>
MidiEvent<TimeType>
make_all_notes_off (midi_byte_t channel, TimeType time)
{
  assert (channel <= 15);
  const std::array<midi_byte_t, 3> raw = {
    static_cast<midi_byte_t> (0xB0 | channel), 0x7B, 0
  };
  MidiEvent<TimeType> ev;
  ev.set_inline_rt (raw);
  ev.time_ = time;
  return ev;
}

/**
 * @brief Creates a MIDI event from the given bytes, RT-safe.
 *
 * Only handles inline messages (≤ inline_capacity bytes). Asserts on
 * larger payloads — use make_raw() for those (not RT-safe).
 */
template <typename TimeType>
MidiEvent<TimeType>
make_raw_rt (std::span<const midi_byte_t> raw, TimeType time) noexcept
  [[clang::nonblocking]]
{
  assert (raw.size () <= MidiEvent<TimeType>::inline_capacity);
  MidiEvent<TimeType> ev;
  ev.set_inline_rt (raw);
  ev.time_ = time;
  return ev;
}

/**
 * @brief Creates a raw MIDI event from the given bytes.
 *
 * Handles arbitrarily-sized payloads (including SysEx) by allocating
 * external storage. Not RT-safe — use make_raw_rt() in RT contexts.
 */
template <typename TimeType>
MidiEvent<TimeType>
make_raw (std::span<const midi_byte_t> raw, TimeType time) [[clang::blocking]]
{
  MidiEvent<TimeType> ev;
  if (raw.size () <= MidiEvent<TimeType>::inline_capacity)
    {
      ev.set_inline_rt (raw);
    }
  else
    {
      auto external = std::make_shared<midi_byte_t[]> (raw.size ());
      std::ranges::copy (raw, external.get ());
      ev.set_external (
        std::move (external), static_cast<uint16_t> (raw.size ()));
    }
  ev.time_ = time;
  return ev;
}

/**
 * @brief Comparator for sorting MidiEvents by time, with noteOff before
 * noteOn at the same timestamp.
 */
struct NoteOffBeforeNoteOnCompare
{
  template <typename TimeType>
  bool
  operator() (const MidiEvent<TimeType> &a, const MidiEvent<TimeType> &b) const
  {
    if (a.time_ != b.time_)
      return a.time_ < b.time_;
    const auto ad = a.data ();
    const auto bd = b.data ();
    return !utils::midi::midi_is_note_on (ad) && utils::midi::midi_is_note_on (bd);
  }
};

/**
 * @brief Sorts events by time, with noteOff before noteOn at the same
 * timestamp.
 */
template <std::ranges::random_access_range Container>
void
sort_with_note_off_priority (Container &container)
{
  std::ranges::stable_sort (container, NoteOffBeforeNoteOnCompare{});
}

/**
 * @brief Sorts events by time only. Event ordering at the same timestamp
 * is preserved (stable sort). Producers (caches, input processors) are
 * responsible for inserting events in the correct order.
 */
template <std::ranges::random_access_range Container>
void
sort (Container &container)
{
  auto proj = [] (const auto &ev) -> const auto & { return ev.time_; };
  std::ranges::stable_sort (container, {}, proj);
}

/**
 * @brief Sets the MIDI channel on all events in the container.
 *
 * @param channel MIDI channel (0-based, 0-15).
 */
template <std::ranges::random_access_range Container>
void
set_channel (Container &container, midi_byte_t channel)
{
  assert (channel <= 15);
  for (auto &ev : container)
    {
      auto d = ev.data ();
      if (d.size () >= 1 && (d[0] & 0xF0) != 0xF0)
        {
          std::array<midi_byte_t, 8> raw{};
          raw[0] = static_cast<midi_byte_t> ((d[0] & 0xF0) | channel);
          std::ranges::copy (d | std::views::drop (1), raw.begin () + 1);
          ev.set_inline_rt (
            std::span<const midi_byte_t>{ raw.data (), d.size () });
        }
    }
}

template <
  typename TimeType,
  utils::MutableContainerOf<MidiEvent<TimeType>> DestContainer,
  std::ranges::range                             SrcContainer>
void
append_in_range (
  DestContainer                &dest,
  const SrcContainer           &src,
  std::pair<TimeType, TimeType> range)
{
  for (const auto &ev : src)
    {
      if (ev.time_ >= range.first && ev.time_ < range.second)
        dest.push_back (ev);
    }
}

} // namespace midi_event

extern template struct MidiEvent<units::sample_t>;
extern template struct MidiEvent<units::precise_tick_t>;
extern template struct MidiEvent<units::sample_u32_t>;

} // namespace zrythm::dsp
