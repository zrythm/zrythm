// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_input_processor.h"
#include "dsp/midi_port.h"
#include "utils/registry_utils.h"

namespace zrythm::dsp
{

MidiInputProcessor::MidiInputProcessor (
  MidiEventProvider       provider,
  utils::IObjectRegistry &registry,
  QObject *               parent)
    : QObject (parent),
      ProcessorBase (
        registry,
        utils::Utf8String::from_utf8_encoded_string ("MIDI Input Processor")),
      provider_ (std::move (provider))
{
  auto port_ref = utils::create_object<MidiPort> (
    registry, utils::Utf8String::from_utf8_encoded_string ("MIDI Input"),
    PortFlow::Output);
  add_output_port (port_ref);
}

void
MidiInputProcessor::custom_process_block (
  dsp::graph::ProcessBlockInfo time_nfo,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  const auto &buffer = provider_ ();
  if (buffer.isEmpty ())
    return;

  auto &port = get_output_port ();
  for (const auto metadata : buffer)
    {
      const auto    &msg = metadata.getMessage ();
      const auto     raw = msg.getRawData ();
      const auto     size = msg.getRawDataSize ();
      dsp::MidiEvent ev;
      ev.time_ =
        units::samples (static_cast<uint32_t> (metadata.samplePosition));
      for (int i = 0; i < std::min (size, 3); ++i)
        ev.raw_buffer_[i] = raw[i];
      ev.raw_buffer_sz_ = static_cast<uint_fast8_t> (std::min (size, 3));
      port.midi_events_.queued_events_.push_back (ev);
    }
}

dsp::MidiPort &
MidiInputProcessor::get_output_port () const noexcept
{
  return *get_output_ports ().at (0).get_object_as<dsp::MidiPort> ();
}

}
