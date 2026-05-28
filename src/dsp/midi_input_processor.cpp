// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_device_buffer.h"
#include "dsp/midi_event.h"
#include "dsp/midi_input_processor.h"
#include "dsp/midi_port.h"
#include "utils/registry_utils.h"

namespace zrythm::dsp
{

struct MidiInputProcessor::Impl
{
  std::shared_ptr<dsp::MidiDeviceBuffer> buffer;
  dsp::MidiDeviceBuffer *                buffer_rt = nullptr;
  juce::MidiBuffer                       drain_buffer;
  dsp::MidiPort *                        output_port = nullptr;
  units::sample_rate_t sample_rate{ units::sample_rate (48000) };
  std::optional<units::precise_second_t> block_start_time;

  explicit Impl (std::shared_ptr<dsp::MidiDeviceBuffer> buf)
      : buffer (std::move (buf)), buffer_rt (buffer.get ())
  {
  }
};

MidiInputProcessor::MidiInputProcessor (
  std::shared_ptr<dsp::MidiDeviceBuffer> buffer,
  utils::IObjectRegistry                &registry,
  QObject *                              parent)
    : QObject (parent),
      ProcessorBase (
        registry,
        utils::Utf8String::from_utf8_encoded_string ("MIDI Input Processor")),
      impl_ (std::make_unique<Impl> (std::move (buffer)))
{
  auto port_ref = utils::create_object<MidiPort> (
    registry, utils::Utf8String::from_utf8_encoded_string ("MIDI Input"),
    PortFlow::Output);
  add_output_port (port_ref);
  impl_->output_port = port_ref.get_object_as<dsp::MidiPort> ();
}

MidiInputProcessor::~MidiInputProcessor () = default;

void
MidiInputProcessor::custom_prepare_for_processing (
  const dsp::graph::GraphNode * node,
  units::sample_rate_t          sample_rate,
  units::sample_u32_t           max_block_length)
{
  impl_->sample_rate = sample_rate;
  constexpr int kMidiBufferBytesPerEvent = 16;
  impl_->drain_buffer.ensureSize (
    static_cast<int> (MidiDeviceBuffer::kCapacity * kMidiBufferBytesPerEvent));
  impl_->output_port->midi_events_.queued_events_.reserve (
    MidiDeviceBuffer::kCapacity);
}

void
MidiInputProcessor::set_block_start_time (units::precise_second_t time)
{
  impl_->block_start_time = time;
}

void
MidiInputProcessor::custom_process_block (
  dsp::graph::ProcessBlockInfo time_nfo,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  impl_->drain_buffer.clear ();
  impl_->buffer_rt->drain (
    impl_->drain_buffer, impl_->sample_rate, time_nfo.nframes_,
    impl_->block_start_time);

  if (impl_->drain_buffer.isEmpty ())
    return;

  auto &port = *impl_->output_port;
  for (const auto metadata : impl_->drain_buffer)
    {
      const auto &msg = metadata.getMessage ();
      const auto raw_data = std::span (msg.getRawData (), msg.getRawDataSize ());

      // SysEx messages (>3 bytes) are silently skipped. MidiEvent::raw_buffer_
      // is fixed at 3 bytes — supporting SysEx requires making it
      // variable-length.
      if (raw_data.size () > 3)
        continue;

      dsp::MidiEvent ev;
      ev.time_ =
        units::samples (static_cast<uint32_t> (metadata.samplePosition));
      std::ranges::copy (raw_data, ev.raw_buffer_.begin ());
      ev.raw_buffer_sz_ = static_cast<uint_fast8_t> (raw_data.size ());
      port.midi_events_.queued_events_.push_back (ev);

      // Prevent unbounded growth
      if (port.midi_events_.queued_events_.size () >= dsp::MAX_MIDI_EVENTS)
        break;
    }
}

dsp::MidiPort &
MidiInputProcessor::get_output_port () const noexcept
{
  return *impl_->output_port;
}

}
