// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>

#include "plugins/modulators/engines/macro_engine.h"
#include "plugins/modulators/modulator_runtime_context.h"
#include "utils/float_ranges.h"
#include "utils/hash.h"

namespace zrythm::plugins::modulators
{

std::unique_ptr<PluginDescriptor>
MacroEngine::make_descriptor ()
{
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Macro";
  descriptor->author_ = u8"Zrythm";
  descriptor->category_ = PluginCategory::MODULATOR;
  descriptor->category_str_ =
    PluginDescriptor::category_to_string (descriptor->category_);
  descriptor->protocol_ = Protocol::ProtocolType::Internal;
  descriptor->path_or_id_ =
    utils::Utf8String::from_utf8_encoded_string (kDescriptorId);
  descriptor->unique_id_ = static_cast<int64_t> (utils::hash::get_custom_hash (
    kDescriptorId.data (), kDescriptorId.size ()));
  descriptor->num_cv_ins_ = 1;
  descriptor->num_cv_outs_ = 1;
  descriptor->num_ctrl_ins_ = 1;
  return descriptor;
}

ModulatorLayout
MacroEngine::layout () const
{
  return ModulatorLayout{
    .input_ports_ = { ModulatorPortSpec{
      .symbol_ = utils::Utf8String::from_utf8_encoded_string (kInputSymbol),
      .label_ = u8"CV In",
      .type_ = dsp::PortType::CV,
    } },
    .output_ports_ = { ModulatorPortSpec{
      .symbol_ = utils::Utf8String::from_utf8_encoded_string (kOutputSymbol),
      .label_ = u8"CV Out",
      .type_ = dsp::PortType::CV,
    } },
    .parameters_ = { ModulatorParameterSpec{
      .unique_id_ = utils::Utf8String::from_utf8_encoded_string (kParameterId),
      .label_ = u8"Macro",
      .range_ = dsp::
        ParameterRange{ dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.75f },
    } },
  };
}

void
MacroEngine::prepare (const ModulatorRuntimeContext &context) noexcept
{
  macro_param_ = context.parameter (0);
  cv_in_ = context.cv_input (0);
  cv_out_ = context.cv_output (0);
  assert (macro_param_ != nullptr);
  assert (cv_in_ != nullptr);
  assert (cv_out_ != nullptr);
}

void
MacroEngine::release_resources () noexcept
{
  macro_param_ = nullptr;
  cv_in_ = nullptr;
  cv_out_ = nullptr;
}

void
MacroEngine::process (
  dsp::graph::ProcessBlockInfo time_info,
  const dsp::ITransport &,
  const dsp::TempoMap &) noexcept
{
  const auto offset = time_info.buffer_offset_.in (units::samples);
  const auto nframes = time_info.nframes_.in (units::samples);
  auto       output = std::span (cv_out_->buf_).subspan (offset, nframes);
  const auto macro_value = macro_param_->currentValue ();

  if (!cv_in_->port_sources ().empty ())
    {
      utils::float_ranges::copy (
        output, std::span (cv_in_->buf_).subspan (offset, nframes));
      utils::float_ranges::mul_k2 (output, macro_value);
    }
  else
    {
      utils::float_ranges::fill (output, macro_value);
    }
}

} // namespace zrythm::plugins::modulators
