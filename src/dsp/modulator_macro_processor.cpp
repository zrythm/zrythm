// SPDX-FileCopyrightText: © 2021-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/modulator_macro_processor.h"
#include "dsp/port_all.h"

#include <fmt/format.h>

namespace zrythm::dsp
{

ModulatorMacroProcessor::ModulatorMacroProcessor (
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  int                              idx,
  QObject *                        parent)
    : QObject (parent),
      ProcessorBase (
        port_registry,
        param_registry,
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format (
            "{} Modulator Macro Processor",
            format_qstr (QObject::tr ("Macro {}"), idx + 1)))),
      port_registry_ (port_registry), param_registry_ (param_registry),
      name_ (
        utils::Utf8String::from_qstring (
          format_qstr (QObject::tr ("Macro {}"), idx + 1)))
{
  add_parameter (param_registry.create_object<dsp::ProcessorParameter> (
    port_registry,
    dsp::ProcessorParameter::UniqueId (
      utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("macro_{}", idx + 1))),
    dsp::ParameterRange{
      dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.75f },
    name_));

  {
    add_input_port (port_registry.create_object<dsp::CVPort> (
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
    add_output_port (port_registry.create_object<dsp::CVPort> (
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
  const EngineProcessTimeInfo time_nfo)
{
  auto &macro = get_macro_param ();
  auto &cv_in = get_cv_in_port ();
  auto &cv_out = get_cv_out_port ();

  /* if there are inputs, multiply by the knob value */
  if (!cv_in.port_sources_.empty ())
    {
      utils::float_ranges::copy (
        &cv_out.buf_[time_nfo.local_offset_],
        &cv_in.buf_[time_nfo.local_offset_], time_nfo.nframes_);
      utils::float_ranges::mul_k2 (
        &cv_out.buf_[time_nfo.local_offset_], macro.currentValue (),
        time_nfo.nframes_);
    }
  /* else if there are no inputs, set the knob value as the output */
  else
    {
      utils::float_ranges::fill (
        &cv_out.buf_[time_nfo.local_offset_], macro.currentValue (),
        time_nfo.nframes_);
    }
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
