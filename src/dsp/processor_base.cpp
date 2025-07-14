// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/processor_base.h"
#include "utils/dsp.h"

namespace zrythm::dsp
{
void
ProcessorBase::set_name (const utils::Utf8String &name)
{
  name_ = name;

  const auto full_designation_provider =
    [this] (const Port &port) -> utils::Utf8String {
    return name_ + u8"/" + port.get_label ();
  };

  const auto port_visitor = [&] (auto &&port) {
    port->set_full_designation_provider (full_designation_provider);
  };
  for (const auto &in_ref : input_ports_)
    {
      std::visit (port_visitor, in_ref.get_object ());
    }
  for (const auto &out_ref : output_ports_)
    {
      std::visit (port_visitor, out_ref.get_object ());
    }
}

void
ProcessorBase::add_input_port (const dsp::PortUuidReference &uuid)
{
  input_ports_.push_back (uuid);
}

void
ProcessorBase::add_output_port (const dsp::PortUuidReference &uuid)
{
  output_ports_.push_back (uuid);
}

void
ProcessorBase::add_parameter (const dsp::ProcessorParameterUuidReference &uuid)
{
  params_.push_back (uuid);
}

void
ProcessorBase::prepare_for_processing (
  sample_rate_t sample_rate,
  nframes_t     max_block_length)
{
  const auto port_visitor = [&] (auto &&port) {
    port->prepare_for_processing (sample_rate, max_block_length);
  };
  for (const auto &in_ref : input_ports_)
    {
      std::visit (port_visitor, in_ref.get_object ());
    }
  for (const auto &out_ref : output_ports_)
    {
      std::visit (port_visitor, out_ref.get_object ());
    }
}

void
ProcessorBase::release_resources ()
{
  const auto port_visitor = [&] (auto &&port) { port->release_resources (); };
  for (const auto &in_ref : input_ports_)
    {
      std::visit (port_visitor, in_ref.get_object ());
    }
  for (const auto &out_ref : output_ports_)
    {
      std::visit (port_visitor, out_ref.get_object ());
    }
}

void
ProcessorBase::process_block (EngineProcessTimeInfo time_nfo)
{
  // process all parameters first
  for (const auto &param_ref : params_)
    {
      param_ref.get_object_as<dsp::ProcessorParameter> ()->process_block (
        time_nfo);
    }

  // do processor logic
  custom_process_block (time_nfo);
}

void
ProcessorBase::custom_process_block (EngineProcessTimeInfo time_nfo)
{
  using ObjectView = utils::UuidIdentifiableObjectView<PortRegistry>;
  const auto object_getter = [] (auto &&port_ref) {
    return port_ref.get_object ();
  };

  const auto in_ports = input_ports_ | std::views::transform (object_getter);
  const auto out_ports = output_ports_ | std::views::transform (object_getter);

  auto midi_in_ports =
    in_ports | std::views::filter (ObjectView::type_projection<MidiPort>)
    | std::views::transform (ObjectView::type_transformation<MidiPort>);
  auto midi_out_ports =
    out_ports | std::views::filter (ObjectView::type_projection<MidiPort>)
    | std::views::transform (ObjectView::type_transformation<MidiPort>);
  auto audio_in_ports =
    in_ports | std::views::filter (ObjectView::type_projection<AudioPort>)
    | std::views::transform (ObjectView::type_transformation<AudioPort>);
  auto audio_out_ports =
    out_ports | std::views::filter (ObjectView::type_projection<AudioPort>)
    | std::views::transform (ObjectView::type_transformation<AudioPort>);
  auto cv_in_ports =
    in_ports | std::views::filter (ObjectView::type_projection<CVPort>)
    | std::views::transform (ObjectView::type_transformation<CVPort>);
  auto cv_out_ports =
    out_ports | std::views::filter (ObjectView::type_projection<CVPort>)
    | std::views::transform (ObjectView::type_transformation<CVPort>);
  for (
    const auto &[in_port, out_port] :
    std::views::zip (midi_in_ports, midi_out_ports))
    {
      out_port->midi_events_.active_events_.append (
        in_port->midi_events_.active_events_, time_nfo.local_offset_,
        time_nfo.nframes_);
    }
  for (
    const auto &[in_port, out_port] :
    std::views::zip (audio_in_ports, audio_out_ports))
    {
      utils::float_ranges::copy (
        &out_port->buf_[time_nfo.local_offset_],
        &in_port->buf_[time_nfo.local_offset_], time_nfo.nframes_);
    }
  for (
    const auto &[in_port, out_port] :
    std::views::zip (cv_in_ports, cv_out_ports))
    {
      utils::float_ranges::copy (
        &out_port->buf_[time_nfo.local_offset_],
        &in_port->buf_[time_nfo.local_offset_], time_nfo.nframes_);
    }
}

void
ProcessorGraphBuilder::add_nodes (
  dsp::graph::Graph &graph,
  dsp::ITransport   &transport,
  ProcessorBase     &processor)
{
  const auto add_node_for_processable = [&] (auto &processable) {
    return graph.add_node_for_processable (processable, transport);
  };
  const auto port_visitor = [&] (auto * port) -> void {
    add_node_for_processable (*port);
  };

  add_node_for_processable (processor);
  for (const auto &port_ref : processor.get_input_ports ())
    {
      std::visit (port_visitor, port_ref.get_object ());
    }
  for (const auto &port_ref : processor.get_output_ports ())
    {
      std::visit (port_visitor, port_ref.get_object ());
    }
}

void
ProcessorGraphBuilder::add_connections (
  dsp::graph::Graph &graph,
  ProcessorBase     &processor)
{
  auto * processor_node =
    graph.get_nodes ().find_node_for_processable (processor);
  z_return_if_fail (processor_node);
  for (const auto &port_ref : processor.get_input_ports ())
    {
      std::visit (
        [&] (auto &&port) {
          auto * port_node =
            graph.get_nodes ().find_node_for_processable (*port);
          assert (port_node);
          port_node->connect_to (*processor_node);
        },
        port_ref.get_object ());
    }
  for (const auto &port_ref : processor.get_output_ports ())
    {
      std::visit (
        [&] (auto &&port) {
          auto * port_node =
            graph.get_nodes ().find_node_for_processable (*port);
          z_return_if_fail (port_node);
          processor_node->connect_to (*port_node);
        },
        port_ref.get_object ());
    }
}
} // namespace zrythm::dsp
