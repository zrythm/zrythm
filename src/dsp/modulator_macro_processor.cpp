// SPDX-FileCopyrightText: © 2021-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/modulator_macro_processor.h"
#include "dsp/port_all.h"

#include <fmt/format.h>

namespace zrythm::dsp
{

ModulatorMacroProcessor::ModulatorMacroProcessor (
  ProcessorBaseDependencies dependencies,
  int                       idx,
  QObject *                 parent)
    : QObject (parent),
      ProcessorBase (
        dependencies,
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format (
            "{} Modulator Macro Processor",
            format_qstr (QObject::tr ("Macro {}"), idx + 1)))),
      dependencies_ (dependencies),
      name_ (
        utils::Utf8String::from_qstring (
          format_qstr (QObject::tr ("Macro {}"), idx + 1)))
{
  add_parameter (dependencies.param_registry_.create_object<dsp::ProcessorParameter> (
    dependencies.port_registry_,
    dsp::ProcessorParameter::UniqueId (
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("macro_{}", idx + 1))),
    dsp::ParameterRange{
      dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.75f },
    name_));

  {
    add_input_port (dependencies.port_registry_.create_object<dsp::CVPort> (
      utils::Utf8String::from_qstring (
        format_qstr (QObject::tr ("Macro {} CV In"), idx + 1)),
      dsp::PortFlow::Input));
    auto &cv_in = get_cv_in_port ();
    cv_in.set_full_designation_provider (this);
    cv_in.set_symbol (
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("macro_cv_in_{}", idx + 1)));
  }

  {
    add_output_port (dependencies.port_registry_.create_object<dsp::CVPort> (
      utils::Utf8String::from_qstring (
        format_qstr (QObject::tr ("Macro {} CV Out"), idx + 1)),
      dsp::PortFlow::Output));
    auto &cv_out = get_cv_out_port ();
    cv_out.set_full_designation_provider (this);
    cv_out.set_symbol (
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("macro_cv_out_{}", idx + 1)));
  }
}

void
ModulatorMacroProcessor::custom_process_block (
  EngineProcessTimeInfo  time_nfo,
  const dsp::ITransport &transport,
  const dsp::TempoMap   &tempo_map) noexcept
{
  /* if there are inputs, multiply by the knob value */
  if (!processing_caches_->cv_in_->port_sources ().empty ())
    {
      utils::float_ranges::copy (
        &processing_caches_->cv_out_->buf_[time_nfo.local_offset_],
        &processing_caches_->cv_in_->buf_[time_nfo.local_offset_],
        time_nfo.nframes_);
      utils::float_ranges::mul_k2 (
        &processing_caches_->cv_out_->buf_[time_nfo.local_offset_],
        processing_caches_->macro_param_->currentValue (), time_nfo.nframes_);
    }
  /* else if there are no inputs, set the knob value as the output */
  else
    {
      utils::float_ranges::fill (
        &processing_caches_->cv_out_->buf_[time_nfo.local_offset_],
        processing_caches_->macro_param_->currentValue (), time_nfo.nframes_);
    }
}

void
ModulatorMacroProcessor::custom_prepare_for_processing (
  const graph::GraphNode * node,
  units::sample_rate_t     sample_rate,
  nframes_t                max_block_length)
{
  processing_caches_ = std::make_unique<ProcessingCaches> ();
  processing_caches_->macro_param_ = &get_macro_param ();
  processing_caches_->cv_in_ = &get_cv_in_port ();
  processing_caches_->cv_out_ = &get_cv_out_port ();
}

void
ModulatorMacroProcessor::custom_release_resources ()
{
  processing_caches_.reset ();
}

utils::Utf8String
ModulatorMacroProcessor::get_full_designation_for_port (
  const dsp::Port &port) const
{
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("Modulator Macro Processor/{}", port.get_label ()));
}

void
init_from (
  ModulatorMacroProcessor       &obj,
  const ModulatorMacroProcessor &other,
  utils::ObjectCloneType         clone_type)
{
  // TODO ProcessorBase
  obj.name_ = other.name_;
}
} // namespace zrythm::dsp
