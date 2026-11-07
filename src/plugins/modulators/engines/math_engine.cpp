// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cassert>
#include <cmath>
#include <ranges>

#include "plugins/modulators/engines/math_engine.h"
#include "plugins/modulators/modulator_runtime_context.h"
#include "utils/hash.h"

namespace zrythm::plugins::modulators
{

std::unique_ptr<PluginDescriptor>
MathEngine::make_descriptor ()
{
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Math";
  descriptor->author_ = u8"Zrythm";
  descriptor->category_ = PluginCategory::MODULATOR;
  descriptor->category_str_ =
    PluginDescriptor::category_to_string (descriptor->category_);
  descriptor->protocol_ = Protocol::ProtocolType::Internal;
  descriptor->path_or_id_ =
    utils::Utf8String::from_utf8_encoded_string (kDescriptorId);
  descriptor->unique_id_ = static_cast<int64_t> (utils::hash::get_custom_hash (
    kDescriptorId.data (), kDescriptorId.size ()));
  descriptor->num_cv_ins_ = 2;
  descriptor->num_cv_outs_ = 1;
  descriptor->num_ctrl_ins_ = 1;
  return descriptor;
}

ModulatorLayout
MathEngine::layout () const
{
  return ModulatorLayout{
    .input_ports_ = {
      ModulatorPortSpec{
        .symbol_ = utils::Utf8String::from_utf8_encoded_string (kInputASymbol),
        .label_ = u8"A In",
        .type_ = dsp::PortType::CV,
      },
      ModulatorPortSpec{
        .symbol_ = utils::Utf8String::from_utf8_encoded_string (kInputBSymbol),
        .label_ = u8"B In",
        .type_ = dsp::PortType::CV,
      },
    },
    .output_ports_ = { ModulatorPortSpec{
      .symbol_ = utils::Utf8String::from_utf8_encoded_string (kOutputSymbol),
      .label_ = u8"CV Out",
      .type_ = dsp::PortType::CV,
    } },
    .parameters_ = { ModulatorParameterSpec{
      .unique_id_ = utils::Utf8String::from_utf8_encoded_string (
        kOperationParameterId),
      .label_ = u8"Operation",
      .range_ = dsp::ParameterRange::make_enumeration (
        { u8"Add", u8"Subtract", u8"Multiply", u8"Average", u8"Minimum",
          u8"Maximum", u8"Invert A" },
        0) } },
  };
}

void
MathEngine::prepare (const ModulatorRuntimeContext &context) noexcept
{
  operation_param_ = context.parameter (0);
  a_in_ = context.cv_input (0);
  b_in_ = context.cv_input (1);
  cv_out_ = context.cv_output (0);
  assert (operation_param_ != nullptr && a_in_ != nullptr);
  assert (b_in_ != nullptr && cv_out_ != nullptr);
}

void
MathEngine::release_resources () noexcept
{
  operation_param_ = nullptr;
  a_in_ = nullptr;
  b_in_ = nullptr;
  cv_out_ = nullptr;
}

void
MathEngine::process (
  dsp::graph::ProcessBlockInfo time_info,
  const dsp::ITransport &,
  const dsp::TempoMap &) noexcept
{
  const auto offset = time_info.buffer_offset_.in (units::samples);
  const auto nframes = time_info.nframes_.in (units::samples);
  auto       output = std::span (cv_out_->buf_).subspan (offset, nframes);

  // Inputs without sources read as zero instead of their (cleared) buffers
  // so operations stay meaningful with one input connected.
  const bool a_connected = !a_in_->port_sources ().empty ();
  const bool b_connected = !b_in_->port_sources ().empty ();
  const auto a_values = std::span (a_in_->buf_).subspan (offset, nframes);
  const auto b_values = std::span (b_in_->buf_).subspan (offset, nframes);

  const auto operation =
    operation_param_->range ().enumIndex (operation_param_->currentValue ());

  for (const auto i : std::views::iota (size_t{ 0 }, nframes))
    {
      const float a = a_connected ? a_values[i] : 0.f;
      const float b = b_connected ? b_values[i] : 0.f;
      float       result = 0.f;
      switch (operation)
        {
        case 1: // Subtract
          result = a - b;
          break;
        case 2: // Multiply
          result = a * b;
          break;
        case 3: // Average
          result = (a + b) / 2.f;
          break;
        case 4: // Minimum
          result = std::min (a, b);
          break;
        case 5: // Maximum
          result = std::max (a, b);
          break;
        case 6: // Invert A
          result = 1.f - a;
          break;
        default: // Add
          result = a + b;
          break;
        }
      output[i] = std::clamp (result, 0.f, 1.f);
    }
}

} // namespace zrythm::plugins::modulators
