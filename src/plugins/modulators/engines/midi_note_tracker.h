// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <cstdint>

#include "dsp/midi_event_buffer.h"
#include "dsp/midi_port.h"

namespace zrythm::plugins::modulators
{

/**
 * @brief Collects a MIDI input port's cycle events into time order.
 *
 * Port buffers aggregate their sources by appending, so event order across
 * sources is not guaranteed. The tracker copies the events of the current
 * processing window into a pre-allocated buffer and sorts them by time, with
 * note-offs ordered before note-ons at equal times.
 */
class MidiNoteTracker final
{
public:
  /** Pre-allocates the event storage; call before the first collect(). */
  void prepare_resources ()
  {
    // Matches the reserve of the ports whose buffers are collected, so a
    // full port buffer is never truncated.
    events_.reserve (dsp::MidiEventBuffer::kMaxReserveBytes);
  }

  void release_resources () { events_ = dsp::MidiEventBuffer{}; }

  /**
   * @brief Collects @p port's events within [offset, offset + nframes) into
   * time order.
   *
   * Events at or after the window end are dropped; earlier timestamps are
   * clamped to the window start so consumers can index output buffers
   * directly.
   */
  void collect (
    const dsp::MidiPort &port,
    std::size_t          offset,
    std::size_t          nframes) noexcept
  {
    events_.clear ();
    if (nframes == 0)
      return;

    const auto window_start = static_cast<uint32_t> (offset);
    const auto window_end = static_cast<uint32_t> (offset + nframes);
    for (const auto &event : port.buffer_)
      {
        auto time_raw = event.time ().in<uint32_t> (units::samples);
        if (time_raw < window_start)
          time_raw = window_start;
        else if (time_raw >= window_end)
          continue;
        events_.push_back (units::samples (time_raw), event.data ());
      }
    dsp::midi_event::sort_with_note_off_priority (events_);
  }

  /** The events collected by the last collect() call, in time order. */
  [[nodiscard]] const dsp::MidiEventBuffer &events () const noexcept
  {
    return events_;
  }

private:
  dsp::MidiEventBuffer events_;
};

} // namespace zrythm::plugins::modulators
