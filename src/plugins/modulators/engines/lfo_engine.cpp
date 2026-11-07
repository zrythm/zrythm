// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numbers>

#include "plugins/modulators/engines/lfo_engine.h"
#include "plugins/modulators/modulator_runtime_context.h"
#include "utils/hash.h"

namespace zrythm::plugins::modulators
{

std::unique_ptr<PluginDescriptor>
LfoEngine::make_descriptor ()
{
  auto descriptor = std::make_unique<PluginDescriptor> ();
  descriptor->name_ = u8"LFO";
  descriptor->author_ = u8"Zrythm";
  descriptor->category_ = PluginCategory::MODULATOR;
  descriptor->category_str_ =
    PluginDescriptor::category_to_string (descriptor->category_);
  descriptor->protocol_ = Protocol::ProtocolType::Internal;
  descriptor->path_or_id_ =
    utils::Utf8String::from_utf8_encoded_string (kDescriptorId);
  descriptor->unique_id_ = static_cast<int64_t> (utils::hash::get_custom_hash (
    kDescriptorId.data (), kDescriptorId.size ()));
  descriptor->num_cv_outs_ = 1;
  descriptor->num_ctrl_ins_ = 4;
  return descriptor;
}

ModulatorLayout
LfoEngine::layout () const
{
  using Type = dsp::ParameterRange::Type;
  return ModulatorLayout{
    .input_ports_ = {},
    .output_ports_ = { ModulatorPortSpec{
      .symbol_ = utils::Utf8String::from_utf8_encoded_string (kOutputSymbol),
      .label_ = u8"CV Out",
      .type_ = dsp::PortType::CV,
    } },
    .parameters_ = {
      ModulatorParameterSpec{
        .unique_id_ = utils::Utf8String::from_utf8_encoded_string (
          kRateParameterId),
        .label_ = u8"Rate",
        .range_ = dsp::ParameterRange{
          Type::Logarithmic, 0.1f, 20.f, 0.1f, 1.f } },
      ModulatorParameterSpec{
        .unique_id_ =
          utils::Utf8String::from_utf8_encoded_string (kSyncParameterId),
        .label_ = u8"Tempo Sync",
        .range_ = dsp::ParameterRange::make_toggle (false) },
      ModulatorParameterSpec{
        .unique_id_ = utils::Utf8String::from_utf8_encoded_string (
          kDivisionParameterId),
        .label_ = u8"Division",
        .range_ = dsp::ParameterRange::make_enumeration (
          { u8"1/1", u8"1/2", u8"1/4", u8"1/8", u8"1/16", u8"1/32" }, 2) },
      ModulatorParameterSpec{
        .unique_id_ = utils::Utf8String::from_utf8_encoded_string (
          kWaveformParameterId),
        .label_ = u8"Waveform",
        .range_ = dsp::ParameterRange::make_enumeration (
          { u8"Sine", u8"Triangle", u8"Saw", u8"Square" }, 0) },
    },
  };
}

void
LfoEngine::prepare (const ModulatorRuntimeContext &context) noexcept
{
  rate_param_ = context.parameter (0);
  sync_param_ = context.parameter (1);
  division_param_ = context.parameter (2);
  waveform_param_ = context.parameter (3);
  cv_out_ = context.cv_output (0);
  sample_rate_ = context.sample_rate ().in<double> (units::sample_rate);
  phase_ = 0.;
  assert (rate_param_ != nullptr && sync_param_ != nullptr);
  assert (division_param_ != nullptr && waveform_param_ != nullptr);
  assert (cv_out_ != nullptr);
}

void
LfoEngine::release_resources () noexcept
{
  rate_param_ = nullptr;
  sync_param_ = nullptr;
  division_param_ = nullptr;
  waveform_param_ = nullptr;
  cv_out_ = nullptr;
  sample_rate_ = 0.;
  phase_ = 0.;
}

void
LfoEngine::process (
  dsp::graph::ProcessBlockInfo time_info,
  const dsp::ITransport &,
  const dsp::TempoMap &tempo_map) noexcept
{
  const auto offset = time_info.buffer_offset_.in (units::samples);
  const auto nframes = time_info.nframes_.in (units::samples);
  auto       output = std::span (cv_out_->buf_).subspan (offset, nframes);

  const auto waveform_index =
    waveform_param_->range ().enumIndex (waveform_param_->currentValue ());
  const bool tempo_synced =
    sync_param_->range ().isToggled (sync_param_->currentValue ());

  double cycles_per_second = 0.;
  double phase = phase_;
  if (tempo_synced)
    {
      // One cycle spans the selected note division: 1/1 is a whole note
      // (4 beats), 1/4 is one beat, and so on.
      static constexpr std::array<double, 6> beats_per_division{
        4., 2., 1., 0.5, 0.25, 0.125
      };
      const auto division_index =
        division_param_->range ().enumIndex (division_param_->currentValue ());
      const auto beats = beats_per_division[std::min (
        division_index, beats_per_division.size () - 1)];
      const auto transport_samples =
        static_cast<double> (time_info.transport_position_.in (units::samples));
      const auto timeline_tick =
        tempo_map.samples_to_tick (units::samples (transport_samples));
      const auto bpm =
        tempo_map
          .tempo_at_tick (
            units::ticks (static_cast<int64_t> (timeline_tick.asDouble ())))
          .in (units::bpm);
      cycles_per_second = bpm / 60. / beats;

      // The phase is a function of the timeline position: one cycle spans
      // one note division and cycles align to division boundaries. The
      // tempo is sampled at the block start, so a tempo change within a
      // block advances the phase at the block-start rate until the next
      // block.
      const double ticks_per_division =
        beats
        * static_cast<double> (dsp::TempoMap::get_ppq ().in (units::ticks));
      phase = std::fmod (timeline_tick.asDouble () / ticks_per_division, 1.);
    }
  else
    {
      cycles_per_second =
        rate_param_->range ().convertFrom0To1 (rate_param_->currentValue ());
    }

  const double increment = cycles_per_second / sample_rate_;
  for (auto &sample : output)
    {
      phase += increment;
      if (phase >= 1.)
        phase -= std::floor (phase);

      switch (waveform_index)
        {
        case 1: // Triangle
          sample =
            static_cast<float> (phase < 0.5 ? 2. * phase : 2. - 2. * phase);
          break;
        case 2: // Saw
          sample = static_cast<float> (phase);
          break;
        case 3: // Square
          sample = phase < 0.5 ? 0.f : 1.f;
          break;
        default: // Sine
          sample = static_cast<float> (
            0.5 - 0.5 * std::cos (2. * std::numbers::pi * phase));
          break;
        }
    }
  phase_ = phase;
}

} // namespace zrythm::plugins::modulators
