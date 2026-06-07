// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_port.h"
#include "utils/midi.h"
#include "utils/views.h"

namespace zrythm::dsp
{
MidiPort::MidiPort (utils::Utf8String label, PortFlow flow)
    : Port (std::move (label), PortType::Midi, flow)
{
}

void
init_from (MidiPort &obj, const MidiPort &other, utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Port &> (obj), static_cast<const Port &> (other), clone_type);
}

void
MidiPort::prepare_for_processing_impl (
  const graph::GraphNode * node,
  units::sample_rate_t     sample_rate,
  units::sample_u32_t      max_block_length)
{
  assert (
    node == nullptr
    || std::addressof (node->get_processable ())
         == static_cast<graph::IProcessable *> (this));

  if (node != nullptr && flow () == PortFlow::Input)
    {
      auto source_ports =
        node->depends () | std::views::transform ([] (const auto &child_node) {
          return dynamic_cast<MidiPort *> (
            &child_node.get ().get_processable ());
        })
        | utils::views::filter_null;
      set_port_sources (source_ports);
    }

  buffer_.reserve (
    static_cast<size_t> (max_block_length.in (units::samples)) * 4);
}

void
MidiPort::release_resources ()
{
}

void
MidiPort::clear_buffer (std::size_t offset, std::size_t nframes)
{
  buffer_.remove_if ([offset, nframes] (const MidiEventView &ev) {
    return ev.time () >= units::samples (offset)
           && ev.time () < units::samples (offset + nframes);
  });
}

void
MidiPort::process_block (
  dsp::graph::ProcessBlockInfo time_nfo,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  /* Input ports: clear and aggregate from sources. */
  if (flow () == PortFlow::Input)
    {
      buffer_.clear ();

      for (const auto &[src_port, conn] : port_sources ())
        {
          if (!conn->enabled_)
            continue;

          dsp::midi_event::append_in_range (
            buffer_, src_port->buffer_,
            std::pair{
              time_nfo.buffer_offset_,
              time_nfo.buffer_offset_ + time_nfo.nframes_ });
        }
    }

  /* Sort by time (both input and output). */
  buffer_.sort ([] (const auto &a, const auto &b) {
    return a.time () < b.time ();
  });
}
} // namespace zrythm::dsp
