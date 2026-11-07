// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <ranges>

#include "plugins/modulators/engines/adsr_engine.h"
#include "plugins/modulators/modulator_runtime_context.h"
#include "utils/hash.h"
#include "utils/midi.h"

namespace zrythm::plugins::modulators
{

std::unique_ptr<PluginDescriptor>
AdsrEngine::make_descriptor ()
{
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"ADSR";
  descriptor->author_ = u8"Zrythm";
  descriptor->category_ = PluginCategory::MODULATOR;
  descriptor->category_str_ =
    PluginDescriptor::category_to_string (descriptor->category_);
  descriptor->protocol_ = Protocol::ProtocolType::Internal;
  descriptor->path_or_id_ =
    utils::Utf8String::from_utf8_encoded_string (kDescriptorId);
  descriptor->unique_id_ = static_cast<int64_t> (utils::hash::get_custom_hash (
    kDescriptorId.data (), kDescriptorId.size ()));
  descriptor->num_midi_ins_ = 1;
  descriptor->num_cv_outs_ = 1;
  descriptor->num_ctrl_ins_ = 4;
  return descriptor;
}

ModulatorLayout
AdsrEngine::layout () const
{
  using Type = dsp::ParameterRange::Type;
  return ModulatorLayout{
    .input_ports_ = { ModulatorPortSpec{
      .symbol_ = utils::Utf8String::from_utf8_encoded_string (kInputSymbol),
      .label_ = u8"MIDI In",
      .type_ = dsp::PortType::Midi,
    } },
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
          Type::Logarithmic, 0.001f, 5.f, 0.001f, 0.01f } },
      ModulatorParameterSpec{
        .unique_id_ = utils::Utf8String::from_utf8_encoded_string (
          kDecayParameterId),
        .label_ = u8"Decay",
        .range_ = dsp::ParameterRange{
          Type::Logarithmic, 0.001f, 5.f, 0.001f, 0.1f } },
      ModulatorParameterSpec{
        .unique_id_ = utils::Utf8String::from_utf8_encoded_string (
          kSustainParameterId),
        .label_ = u8"Sustain",
        .range_ = dsp::ParameterRange{
          Type::Linear, 0.f, 1.f, 0.f, 0.7f } },
      ModulatorParameterSpec{
        .unique_id_ = utils::Utf8String::from_utf8_encoded_string (
          kReleaseParameterId),
        .label_ = u8"Release",
        .range_ = dsp::ParameterRange{
          Type::Logarithmic, 0.001f, 10.f, 0.001f, 0.3f } },
    },
  };
}

void
AdsrEngine::prepare (const ModulatorRuntimeContext &context) noexcept
{
  attack_param_ = context.parameter (0);
  decay_param_ = context.parameter (1);
  sustain_param_ = context.parameter (2);
  release_param_ = context.parameter (3);
  midi_in_ = context.midi_input (0);
  cv_out_ = context.cv_output (0);
  sample_rate_ = context.sample_rate ().in<double> (units::sample_rate);
  tracker_.prepare_resources ();
  stage_ = Stage::Off;
  level_ = 0.f;
  held_note_ = -1;
  assert (attack_param_ != nullptr && decay_param_ != nullptr);
  assert (sustain_param_ != nullptr && release_param_ != nullptr);
  assert (midi_in_ != nullptr && cv_out_ != nullptr);
}

void
AdsrEngine::release_resources () noexcept
{
  attack_param_ = nullptr;
  decay_param_ = nullptr;
  sustain_param_ = nullptr;
  release_param_ = nullptr;
  midi_in_ = nullptr;
  cv_out_ = nullptr;
  tracker_.release_resources ();
  sample_rate_ = 0.;
  stage_ = Stage::Off;
  level_ = 0.f;
  held_note_ = -1;
}

void
AdsrEngine::process (
  dsp::graph::ProcessBlockInfo time_info,
  const dsp::ITransport &,
  const dsp::TempoMap &) noexcept
{
  const auto offset = time_info.buffer_offset_.in (units::samples);
  const auto nframes = time_info.nframes_.in (units::samples);
  auto       output = std::span (cv_out_->buf_).subspan (offset, nframes);

  const auto ramp_time_samples = [this] (dsp::ProcessorParameter * param) {
    const auto seconds = static_cast<double> (
      param->range ().convertFrom0To1 (param->currentValue ()));
    return std::max (seconds * sample_rate_, 1.);
  };
  const double attack_step = 1. / ramp_time_samples (attack_param_);
  const double decay_step = 1. / ramp_time_samples (decay_param_);
  const double release_step = 1. / ramp_time_samples (release_param_);
  const float  sustain = sustain_param_->currentValue ();

  tracker_.collect (*midi_in_, offset, nframes);

  std::size_t segment_start = 0;
  const auto  advance_through = [&] (std::size_t end) {
    for (const auto i : std::views::iota (segment_start, end))
      {
        switch (stage_)
          {
          case Stage::Attack:
            level_ = static_cast<float> (level_ + attack_step);
            if (level_ >= 1.f)
              {
                level_ = 1.f;
                stage_ = Stage::Decay;
              }
            break;
          case Stage::Decay:
            level_ = static_cast<float> (level_ - decay_step);
            if (level_ <= sustain)
              {
                level_ = sustain;
                stage_ = Stage::Sustain;
              }
            break;
          case Stage::Sustain:
            level_ = sustain;
            break;
          case Stage::Release:
            level_ = static_cast<float> (level_ - release_step);
            if (level_ <= 0.f)
              {
                level_ = 0.f;
                stage_ = Stage::Off;
              }
            break;
          case Stage::Off:
            level_ = 0.f;
            break;
          }
        output[i] = level_;
      }
    segment_start = end;
  };

  for (const auto &event : tracker_.events ())
    {
      const auto event_index = std::min<std::size_t> (
        event.time ().in<std::size_t> (units::samples) - offset, nframes - 1);
      advance_through (event_index);

      const auto data = event.data ();
      const bool gate_on = utils::midi::midi_is_note_on (data);
      const bool gate_off =
        (utils::midi::midi_is_note_off (data)
         && static_cast<int> (utils::midi::midi_get_note_number (data))
              == held_note_)
        || utils::midi::midi_is_all_notes_off (data);
      if (gate_on)
        {
          held_note_ =
            static_cast<int> (utils::midi::midi_get_note_number (data));
          stage_ = Stage::Attack;
        }
      else if (gate_off)
        {
          held_note_ = -1;
          stage_ = Stage::Release;
        }
    }
  advance_through (nframes);
}

} // namespace zrythm::plugins::modulators
