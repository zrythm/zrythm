// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/processor_base.h"
#include "utils/float_ranges.h"
#include "utils/logger.h"
#include "utils/raii_utils.h"
#include "utils/views.h"

namespace zrythm::dsp
{

void
ProcessorBase::ParameterChangeTracker::prepare (size_t count)
{
  prev_values_.assign (count, -1.f);
  changes_.clear ();
  changes_.reserve (count);
}

ProcessorBase::ProcessorBase (
  utils::IObjectRegistry &registry,
  utils::Utf8String       name)
    : registry_ (registry), name_ (std::move (name))
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

  for (const auto &in_ref : input_ports_)
    {
      if (auto * port = in_ref.get ())
        port->set_full_designation_provider (full_designation_provider);
    }
  for (const auto &out_ref : output_ports_)
    {
      if (auto * port = out_ref.get ())
        port->set_full_designation_provider (full_designation_provider);
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
  units::sample_u32_t      max_block_length)
{
  processing_caches_ = std::make_unique<BaseProcessingCache> ();
  processing_caches_->sample_rate_ = sample_rate;
  processing_caches_->max_block_length_ = max_block_length;

  processing_caches_->live_params_.clear ();
  processing_caches_->live_input_ports_.clear ();
  processing_caches_->live_output_ports_.clear ();

  const auto call_prepare_for_processing = [&] (auto &&processable) {
    processable->prepare_for_processing (node, sample_rate, max_block_length);
  };
  for (const auto &in_ref : input_ports_)
    {
      auto * raw = in_ref.get ();
      auto   var = utils::convert_to_variant_qobj<dsp::PortPtrVariant> (raw);
      call_prepare_for_processing (raw);
      processing_caches_->live_input_ports_.push_back (var);
    }
  for (const auto &out_ref : output_ports_)
    {
      auto * raw = out_ref.get ();
      auto   var = utils::convert_to_variant_qobj<dsp::PortPtrVariant> (raw);
      call_prepare_for_processing (raw);
      processing_caches_->live_output_ports_.push_back (var);
    }
  for (const auto &param_ref : params_)
    {
      processing_caches_->live_params_.push_back (param_ref.get ());
      call_prepare_for_processing (processing_caches_->live_params_.back ());
    }

  processing_caches_->change_tracker_.prepare (
    processing_caches_->live_params_.size ());

  custom_prepare_for_processing (node, sample_rate, max_block_length);
}

void
ProcessorBase::release_resources ()
{
  for (const auto &in_var : input_ports_)
    {
      in_var.get ()->release_resources ();
    }
  for (const auto &out_var : output_ports_)
    {
      out_var.get ()->release_resources ();
    }

  custom_release_resources ();

  processing_caches_.reset ();
}

void
ProcessorBase::process_block (
  dsp::graph::ProcessBlockInfo time_nfo,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  if (processing_caches_ == nullptr)
    {
      z_warning (
        "processing_caches_ is null — prepare_for_processing() not called?");
      assert (false);
      return;
    }

  // correct invalid time info
  if (
    time_nfo.buffer_offset_ + time_nfo.nframes_
    > processing_caches_->max_block_length_) [[unlikely]]
    {
      auto local_offset = time_nfo.buffer_offset_;
      auto nframes = time_nfo.nframes_;
      if (local_offset >= processing_caches_->max_block_length_)
        {
          local_offset = units::samples (0);
          nframes = units::samples (0);
        }
      else
        nframes = processing_caches_->max_block_length_ - local_offset;

      time_nfo.buffer_offset_ = local_offset;
      time_nfo.nframes_ = nframes;
    }

  // process all parameters first and detect changes
  const ScopedBool processing_guard (processing_caches_->is_processing_);
  for (
    const auto &[i, param] :
    utils::views::enumerate (processing_caches_->live_params_))
    {
      param->process_block (time_nfo, transport, tempo_map);
      processing_caches_->change_tracker_.record_if_changed (i, param);
    }

  // do processor logic
  custom_process_block (time_nfo, transport, tempo_map);

  // clear changes for next cycle
  processing_caches_->change_tracker_.clear ();

  // clear input ports for next cycle
  for (const auto &in_var : processing_caches_->live_input_ports_)
    {
      std::visit (
        [&] (auto * in_port) {
          in_port->clear_buffer (
            time_nfo.buffer_offset_.in (units::samples),
            time_nfo.nframes_.in (units::samples));
        },
        in_var);
    }
}

void
ProcessorBase::custom_process_block (
  dsp::graph::ProcessBlockInfo time_nfo,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  const auto &in_ports = processing_caches_->live_input_ports_;
  const auto &out_ports = processing_caches_->live_output_ports_;

  for (const auto &[in_var, out_var] : std::views::zip (in_ports, out_ports))
    {
      std::visit (
        [&] (auto * in_port, auto * out_port) {
          using InT = std::decay_t<decltype (in_port)>;
          using OutT = std::decay_t<decltype (out_port)>;
          if constexpr (
            std::is_same_v<InT, dsp::MidiPort *>
            && std::is_same_v<OutT, dsp::MidiPort *>)
            {
              out_port->clear_buffer (
                time_nfo.buffer_offset_.in (units::samples),
                time_nfo.nframes_.in (units::samples));
              out_port->midi_events_.queued_events_.append (
                in_port->midi_events_.active_events_, time_nfo.buffer_offset_,
                time_nfo.nframes_);
            }
          else if constexpr (
            std::is_same_v<InT, dsp::AudioPort *>
            && std::is_same_v<OutT, dsp::AudioPort *>)
            {
              out_port->copy_source_rt (*in_port, time_nfo);
            }
          else if constexpr (
            std::is_same_v<InT, dsp::CVPort *>
            && std::is_same_v<OutT, dsp::CVPort *>)
            {
              const auto sub_offset =
                time_nfo.buffer_offset_.in (units::samples);
              const auto sub_nframes = time_nfo.nframes_.in (units::samples);
              utils::float_ranges::copy (
                std::span (out_port->buf_).subspan (sub_offset, sub_nframes),
                std::span (in_port->buf_).subspan (sub_offset, sub_nframes));
            }
        },
        in_var, out_var);
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

  add_node_for_processable (processor);
  for (const auto &port_ref : processor.get_input_ports ())
    {
      add_node_for_processable (*port_ref.get ());
    }
  for (const auto &port_ref : processor.get_output_ports ())
    {
      add_node_for_processable (*port_ref.get ());
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
      auto * port_node =
        graph.get_nodes ().find_node_for_processable (*port_ref.get ());
      assert (port_node);
      port_node->connect_to (*processor_node);
    }
  for (const auto &port_ref : processor.get_output_ports ())
    {
      auto * port_node =
        graph.get_nodes ().find_node_for_processable (*port_ref.get ());
      z_return_if_fail (port_node);
      processor_node->connect_to (*port_node);
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
      auto port_ref = dsp::PortUuidReference{ p.registry_ };
      from_json (input_port, port_ref);
      p.input_ports_.emplace_back (std::move (port_ref));
    }
  p.output_ports_.clear ();
  for (const auto &output_port : j.at (ProcessorBase::kOutputPortsKey))
    {
      auto port_ref = dsp::PortUuidReference{ p.registry_ };
      from_json (output_port, port_ref);
      p.output_ports_.emplace_back (std::move (port_ref));
    }
  p.params_.clear ();
  for (const auto &param : j.at (ProcessorBase::kParametersKey))
    {
      auto param_ref = dsp::ProcessorParameterUuidReference{ p.registry_ };
      from_json (param, param_ref);
      p.params_.emplace_back (std::move (param_ref));
    }
}

ProcessorBase::~ProcessorBase () = default;

} // namespace zrythm::dsp
