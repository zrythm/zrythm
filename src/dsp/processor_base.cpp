// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/processor_base.h"
#include "utils/dsp.h"

namespace zrythm::dsp
{

ProcessorBase::ProcessorBase (
  ProcessorBaseDependencies dependencies,
  utils::Utf8String         name)
    : dependencies_ (dependencies), name_ (std::move (name))
{
}

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
  const graph::GraphNode * node,
  units::sample_rate_t     sample_rate,
  nframes_t                max_block_length)
{
  processing_caches_ = std::make_unique<BaseProcessingCache> ();
  processing_caches_->sample_rate_ = sample_rate;
  processing_caches_->max_block_length_ = max_block_length;

  processing_caches_->live_params_.clear ();
  processing_caches_->live_input_ports_.clear ();
  processing_caches_->live_output_ports_.clear ();

  const auto port_visitor = [&] (auto &&port) {
    port->prepare_for_processing (node, sample_rate, max_block_length);
  };
  for (const auto &in_ref : input_ports_)
    {
      std::visit (port_visitor, in_ref.get_object ());
      processing_caches_->live_input_ports_.push_back (in_ref.get_object ());
    }
  for (const auto &out_ref : output_ports_)
    {
      std::visit (port_visitor, out_ref.get_object ());
      processing_caches_->live_output_ports_.push_back (out_ref.get_object ());
    }
  for (const auto &param_ref : params_)
    {
      processing_caches_->live_params_.push_back (
        param_ref.get_object_as<dsp::ProcessorParameter> ());
      processing_caches_->live_params_.back ()->prepare_for_processing (
        node, sample_rate, max_block_length);
    }

  custom_prepare_for_processing (node, sample_rate, max_block_length);
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

  custom_release_resources ();

  processing_caches_.reset ();
}

void
ProcessorBase::process_block (
  EngineProcessTimeInfo  time_nfo,
  const dsp::ITransport &transport,
  const dsp::TempoMap   &tempo_map) noexcept
{
  // correct invalid time info
  if (
    time_nfo.local_offset_ + time_nfo.nframes_
    > processing_caches_->max_block_length_) [[unlikely]]
    {
      auto local_offset = time_nfo.local_offset_;
      auto nframes = time_nfo.nframes_;
      if (local_offset >= processing_caches_->max_block_length_)
        {
          local_offset = 0;
          nframes = 0;
        }
      else
        nframes = processing_caches_->max_block_length_ - local_offset;

      time_nfo.local_offset_ = local_offset;
      time_nfo.g_start_frame_w_offset_ = time_nfo.g_start_frame_ + local_offset;
      time_nfo.nframes_ = nframes;
    }

  // process all parameters first
  for (const auto &param : processing_caches_->live_params_)
    {
      param->process_block (time_nfo, transport, tempo_map);
    }

  // do processor logic
  custom_process_block (time_nfo, transport, tempo_map);

  // clear input ports for next cycle
  for (const auto &in_port_var : processing_caches_->live_input_ports_)
    {
      std::visit (
        [&] (auto &&in_port) {
          in_port->clear_buffer (time_nfo.local_offset_, time_nfo.nframes_);
        },
        in_port_var);
    }
}

void
ProcessorBase::custom_process_block (
  EngineProcessTimeInfo  time_nfo,
  const dsp::ITransport &transport,
  const dsp::TempoMap   &tempo_map) noexcept
{
  using ObjectView = utils::UuidIdentifiableObjectView<PortRegistry>;

  const auto &in_ports = processing_caches_->live_input_ports_;
  const auto &out_ports = processing_caches_->live_output_ports_;

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
      out_port->clear_buffer (time_nfo.local_offset_, time_nfo.nframes_);
      out_port->midi_events_.queued_events_.append (
        in_port->midi_events_.active_events_, time_nfo.local_offset_,
        time_nfo.nframes_);
    }
  for (
    const auto &[in_port, out_port] :
    std::views::zip (audio_in_ports, audio_out_ports))
    {
      out_port->copy_source_rt (*in_port, time_nfo);
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
  ProcessorBase     &processor)
{
  const auto add_node_for_processable = [&] (auto &processable) {
    return graph.add_node_for_processable (processable);
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

void
to_json (nlohmann::json &j, const ProcessorBase &p)
{
  // j[ProcessorBase::kProcessorNameKey] = p.name_;
  j[ProcessorBase::kInputPortsKey] = p.input_ports_;
  j[ProcessorBase::kOutputPortsKey] = p.output_ports_;
  j[ProcessorBase::kParametersKey] = p.params_;
}
void
from_json (const nlohmann::json &j, ProcessorBase &p)
{
  // j.at (ProcessorBase::kProcessorNameKey).get_to (p.name_);
  p.input_ports_.clear ();
  for (const auto &input_port : j.at (ProcessorBase::kInputPortsKey))
    {
      auto port_ref = dsp::PortUuidReference{ p.dependencies_.port_registry_ };
      from_json (input_port, port_ref);
      p.input_ports_.emplace_back (std::move (port_ref));
    }
  p.output_ports_.clear ();
  for (const auto &output_port : j.at (ProcessorBase::kOutputPortsKey))
    {
      auto port_ref = dsp::PortUuidReference{ p.dependencies_.port_registry_ };
      from_json (output_port, port_ref);
      p.output_ports_.emplace_back (std::move (port_ref));
    }
  p.params_.clear ();
  for (const auto &param : j.at (ProcessorBase::kParametersKey))
    {
      auto param_ref =
        dsp::ProcessorParameterUuidReference{ p.dependencies_.param_registry_ };
      from_json (param, param_ref);
      p.params_.emplace_back (std::move (param_ref));
    }
}

ProcessorBase::~ProcessorBase () = default;

} // namespace zrythm::dsp
