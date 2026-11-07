// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cassert>
#include <cmath>
#include <ranges>

#include "plugins/modulators/engines/envelope_follower_engine.h"
#include "plugins/modulators/modulator_runtime_context.h"
#include "utils/hash.h"

namespace zrythm::plugins::modulators
{

std::unique_ptr<PluginDescriptor>
EnvelopeFollowerEngine::make_descriptor ()
{
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"Envelope Follower";
  descriptor->author_ = u8"Zrythm";
  descriptor->category_ = PluginCategory::MODULATOR;
  descriptor->category_str_ =
    PluginDescriptor::category_to_string (descriptor->category_);
  descriptor->protocol_ = Protocol::ProtocolType::Internal;
  descriptor->path_or_id_ =
    utils::Utf8String::from_utf8_encoded_string (kDescriptorId);
  descriptor->unique_id_ = static_cast<int64_t> (utils::hash::get_custom_hash (
    kDescriptorId.data (), kDescriptorId.size ()));
  descriptor->num_audio_ins_ = 1;
  descriptor->num_cv_outs_ = 1;
  descriptor->num_ctrl_ins_ = 2;
  return descriptor;
}

ModulatorLayout
EnvelopeFollowerEngine::layout () const
{
  using Type = dsp::ParameterRange::Type;
  return ModulatorLayout{
    .input_ports_ = { ModulatorPortSpec{
      .symbol_ = utils::Utf8String::from_utf8_encoded_string (kInputSymbol),
      .label_ = u8"Audio In",
      .type_ = dsp::PortType::Audio,
      .audio_arrangement_ = dsp::SpeakerArrangement::mono () } },
    .output_ports_ = { ModulatorPortSpec{
      .symbol_ = utils::Utf8String::from_utf8_encoded_string (kOutputSymbol),
      .label_ = u8"CV Out",
      .type_ = dsp::PortType::CV,
    } },
    .parameters_ = {
      ModulatorParameterSpec{
        .unique_id_ = utils::Utf8String::from_utf8_encoded_string (
          kAttackParameterId),
        .label_ = u8"Attack",
        .range_ = dsp::ParameterRange{
          Type::Logarithmic, 0.001f, 1.f, 0.001f, 0.01f } },
      ModulatorParameterSpec{
        .unique_id_ = utils::Utf8String::from_utf8_encoded_string (
          kReleaseParameterId),
        .label_ = u8"Release",
        .range_ = dsp::ParameterRange{
          Type::Logarithmic, 0.01f, 5.f, 0.01f, 0.2f } },
    },
  };
}

void
EnvelopeFollowerEngine::prepare (const ModulatorRuntimeContext &context) noexcept
{
  attack_param_ = context.parameter (0);
  release_param_ = context.parameter (1);
  audio_in_ = context.audio_input (0);
  cv_out_ = context.cv_output (0);
  sample_rate_ = context.sample_rate ().in<double> (units::sample_rate);
  envelope_ = 0.f;
  assert (attack_param_ != nullptr && release_param_ != nullptr);
  assert (audio_in_ != nullptr && cv_out_ != nullptr);
}

void
EnvelopeFollowerEngine::release_resources () noexcept
{
  attack_param_ = nullptr;
  release_param_ = nullptr;
  audio_in_ = nullptr;
  cv_out_ = nullptr;
  sample_rate_ = 0.;
  envelope_ = 0.f;
}

void
EnvelopeFollowerEngine::process (
  dsp::graph::ProcessBlockInfo time_info,
  const dsp::ITransport &,
  const dsp::TempoMap &) noexcept
{
  const auto offset = time_info.buffer_offset_.in (units::samples);
  const auto nframes = time_info.nframes_.in (units::samples);
  auto       output = std::span (cv_out_->buf_).subspan (offset, nframes);

  const auto attack_seconds = static_cast<double> (
    attack_param_->range ().convertFrom0To1 (attack_param_->currentValue ()));
  const auto release_seconds = static_cast<double> (
    release_param_->range ().convertFrom0To1 (release_param_->currentValue ()));

  // One-pole smoothing: the coefficient is the fraction of the previous
  // envelope kept after one sample at the given time constant.
  const double attack_coeff =
    std::exp (-1. / std::max (attack_seconds * sample_rate_, 1.));
  const double release_coeff =
    std::exp (-1. / std::max (release_seconds * sample_rate_, 1.));

  for (const auto i : std::views::iota (size_t{ 0 }, nframes))
    {
      const float level =
        std::abs (audio_in_->buffers ()->getSample (0, offset + i));
      const auto coeff = level > envelope_ ? attack_coeff : release_coeff;
      envelope_ = static_cast<float> (coeff * envelope_ + (1. - coeff) * level);
      output[i] = std::clamp (envelope_, 0.f, 1.f);
    }
}

} // namespace zrythm::plugins::modulators
