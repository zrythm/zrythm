// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <vector>

#include "dsp/midi_event.h"
#include "utils/units.h"

namespace zrythm::controllers
{

struct RecordingMidiPacket
{
  static constexpr size_t kMaxEventsPerBlock = 256;

  units::sample_t             timeline_position;
  bool                        transport_recording{};
  units::sample_u32_t         nframes;
  std::vector<dsp::MidiEvent> midi_events;

  static void write_to_slot (
    RecordingMidiPacket        &slot,
    units::sample_t             timeline_position,
    bool                        transport_recording,
    const dsp::MidiEventVector &events,
    units::sample_u32_t         nframes) noexcept [[clang::nonblocking]]
  {
    slot.timeline_position = timeline_position;
    slot.transport_recording = transport_recording;
    slot.nframes = nframes;
    auto count = std::min (events.size (), slot.midi_events.capacity ());
    slot.midi_events.resize (count);
    size_t i = 0;
    events.foreach_event ([&] (const dsp::MidiEvent &ev) {
      if (i >= count)
        return;
      slot.midi_events[i++] = ev;
    });
    slot.midi_events.resize (i);
  }

  static void
  copy_from (RecordingMidiPacket &slot, const RecordingMidiPacket &source)
    [[clang::blocking]]
  {
    slot.timeline_position = source.timeline_position;
    slot.transport_recording = source.transport_recording;
    slot.nframes = source.nframes;
    slot.midi_events = source.midi_events;
  }

  static void
  resize (RecordingMidiPacket &slot, units::sample_u32_t /*block_length*/)
  {
    slot.midi_events.reserve (kMaxEventsPerBlock);
  }
};

}
