// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/midi_event_buffer.h"
#include "utils/units.h"

namespace zrythm::controllers
{

struct RecordingMidiPacket
{
  static constexpr size_t kMaxEventsPerBlock = 256;
  static constexpr size_t kDefaultReserveBytes = 4096;

  units::sample_t      timeline_position;
  bool                 transport_recording{};
  units::sample_u32_t  nframes;
  dsp::MidiEventBuffer midi_events;

  static void write_to_slot (
    RecordingMidiPacket        &slot,
    units::sample_t             timeline_position,
    bool                        transport_recording,
    const dsp::MidiEventBuffer &events,
    units::sample_u32_t         nframes) noexcept [[clang::nonblocking]]
  {
    assert (slot.midi_events.capacity () > 0);
    slot.timeline_position = timeline_position;
    slot.transport_recording = transport_recording;
    slot.nframes = nframes;
    slot.midi_events.clear ();
    for (const auto &ev : events)
      {
        slot.midi_events.push_back (ev.time (), ev.data ());
      }
  }

  static void
  copy_from (RecordingMidiPacket &slot, const RecordingMidiPacket &source)
    [[clang::blocking]]
  {
    slot.timeline_position = source.timeline_position;
    slot.transport_recording = source.transport_recording;
    slot.nframes = source.nframes;
    slot.midi_events.clear ();
    for (const auto &ev : source.midi_events)
      {
        slot.midi_events.push_back (ev.time (), ev.data ());
      }
  }

  static void
  resize (RecordingMidiPacket &slot, units::sample_u32_t /*block_length*/)
  {
    slot.midi_events.reserve (kDefaultReserveBytes);
  }
};

}
