// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "dsp/midi_panic_processor.h"
#include "dsp/midi_port.h"
#include "utils/registry_utils.h"

namespace zrythm::dsp
{

MidiPanicProcessor::MidiPanicProcessor (
  utils::IObjectRegistry &registry,
  QObject *               parent)
    : QObject (parent), dsp::ProcessorBase (registry, u8"MIDI Panic")
{
  add_output_port (
    utils::create_object<dsp::MidiPort> (
      registry, u8"Panic MIDI out", dsp::PortFlow::Output));
}

void
MidiPanicProcessor::custom_process_block (
  dsp::graph::ProcessBlockInfo time_nfo,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  const auto panic = panic_.exchange (false);
  if (!panic)
    return;

  // queue panic event — all-notes-off on all 16 channels
  for (midi_byte_t ch = 0; ch < 16; ++ch)
    {
      auto ev =
        dsp::midi_event::make_all_notes_off (ch, time_nfo.buffer_offset_);
      midi_out_->buffer_.push_back (ev.time_, ev.data ());
    }
}

} // namespace zrythm::dsp
