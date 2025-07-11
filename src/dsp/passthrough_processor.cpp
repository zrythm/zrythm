// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/passthrough_processor.h"
#include "utils/dsp.h"

namespace zrythm::dsp
{

void
SameSignalProcessorBase::set_name (const utils::Utf8String &name)
{
  name_ = name;

  const auto full_designation_provider =
    [this] (const Port &port) -> utils::Utf8String {
    return name_ + u8"/" + port.get_label ();
  };

  for (auto port_ref_tuple : std::views::zip (input_ports_, output_ports_))
    {
      const auto port_var = std::get<0> (port_ref_tuple).get_object ();
      std::visit (
        [&] (auto &&port) {
          port->set_full_designation_provider (full_designation_provider);
        },
        port_var);
    }
}

void
SameSignalProcessorBase::add_port_pair (
  std::pair<dsp::PortUuidReference, dsp::PortUuidReference> port_pair)
{
  input_ports_.push_back (port_pair.first);
  output_ports_.push_back (port_pair.second);
}

void
SameSignalProcessorBase::prepare_for_processing (
  sample_rate_t sample_rate,
  nframes_t     max_block_length)
{
  for (
    const auto &[in_port_ref, out_port_ref] :
    std::views::zip (input_ports_, output_ports_))
    {
      std::visit (
        [&] (auto &&in_port) {
          using PortT = base_type<decltype (in_port)>;
          auto * out_port = out_port_ref.get_object_as<PortT> ();
          in_port->prepare_for_processing (sample_rate, max_block_length);
          out_port->prepare_for_processing (sample_rate, max_block_length);
        },
        in_port_ref.get_object ());
    }
}

void
SameSignalProcessorBase::release_resources ()
{
  for (
    const auto &[in_port_ref, out_port_ref] :
    std::views::zip (input_ports_, output_ports_))
    {
      std::visit (
        [&] (auto &&in_port) {
          using PortT = base_type<decltype (in_port)>;
          auto * out_port = out_port_ref.get_object_as<PortT> ();
          in_port->release_resources ();
          out_port->release_resources ();
        },
        in_port_ref.get_object ());
    }
}

void
SameSignalProcessorBase::process_block (EngineProcessTimeInfo time_nfo)
{
  for (
    const auto &[in_port_ref, out_port_ref] :
    std::views::zip (input_ports_, output_ports_))
    {
      std::visit (
        [&] (auto &&in_port) {
          using PortT = base_type<decltype (in_port)>;
          auto * out_port = out_port_ref.get_object_as<PortT> ();
          if constexpr (std::is_same_v<PortT, MidiPort>)
            {
              out_port->midi_events_.active_events_.append (
                in_port->midi_events_.active_events_, time_nfo.local_offset_,
                time_nfo.nframes_);
            }
          else
            {
              utils::float_ranges::copy (
                &out_port->buf_[time_nfo.local_offset_],
                &in_port->buf_[time_nfo.local_offset_], time_nfo.nframes_);
            }
        },
        in_port_ref.get_object ());
    }
}
} // namespace zrythm::dsp
