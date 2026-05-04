// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "dsp/parameter.h"
#include "dsp/port_all.h"
#include "utils/enum_utils.h"

#include <nlohmann/json.hpp>

namespace zrythm::dsp
{
utils::Utf8String
ParameterRange::unit_to_string (Unit unit)
{
  constexpr std::array<std::u8string_view, 8> port_unit_strings = {
    u8"none", u8"Hz", u8"MHz", u8"dB", u8"°", u8"s", u8"ms", u8"μs",
  };
  return port_unit_strings.at (ENUM_VALUE_TO_INT (unit));
}

float
ParameterRange::convertFrom0To1 (float normalized_val) const
{
  if (type_ == Type::Logarithmic)
    {
      const auto minf = utils::math::floats_equal (minf_, 0.f) ? 1e-20f : minf_;
      const auto maxf = utils::math::floats_equal (maxf_, 0.f) ? 1e-20f : maxf_;
      normalized_val =
        utils::math::floats_equal (normalized_val, 0.f) ? 1e-20f : normalized_val;

      /* see http://lv2plug.in/ns/ext/port-props/port-props.html#rangeSteps */
      return minf * std::pow (maxf / minf, normalized_val);
    }
  if (type_ == Type::Toggle)
    {
      return normalized_val >= 0.001f ? 1.f : 0.f;
    }
  if (type_ == Type::GainAmplitude)
    {
      return utils::math::get_amp_val_from_fader (normalized_val);
    }

  return minf_ + (normalized_val * (maxf_ - minf_));
}

float
ParameterRange::convertTo0To1 (float real_val) const
{
  if (type_ == Type::Logarithmic)
    {
      if (real_val <= 0.f)
        return 0.f;

      const auto minf = utils::math::floats_equal (minf_, 0.f) ? 1e-20f : minf_;
      const auto maxf = utils::math::floats_equal (maxf_, 0.f) ? 1e-20f : maxf_;

      /* see http://lv2plug.in/ns/ext/port-props/port-props.html#rangeSteps */
      const float normalized =
        std::log (real_val / minf) / std::log (maxf / minf);
      return std::clamp (normalized, 0.f, 1.f);
    }
  if (type_ == Type::Toggle)
    {
      return real_val;
    }
  if (type_ == Type::GainAmplitude)
    {
      return utils::math::get_fader_val_from_amp (real_val);
    }

  const auto sizef = maxf_ - minf_;
  return (sizef - (maxf_ - real_val)) / sizef;
}

void
to_json (nlohmann::json &j, const ParameterRange &p)
{
  j["type"] =
    static_cast<std::underlying_type_t<ParameterRange::Type>> (p.type_);
  j["min"] = p.minf_;
  j["max"] = p.maxf_;
  j["def"] = p.deff_;
  j["zero"] = p.zerof_;
  if (p.type_ == ParameterRange::Type::Enumeration)
    {
      j["enumLabels"] = p.enum_labels_;
    }
}

void
from_json (const nlohmann::json &j, ParameterRange &p)
{
  p.type_ = static_cast<ParameterRange::Type> (
    j.at ("type").get<std::underlying_type_t<ParameterRange::Type>> ());
  j.at ("min").get_to (p.minf_);
  j.at ("max").get_to (p.maxf_);
  j.at ("def").get_to (p.deff_);
  if (j.contains ("zero"))
    j.at ("zero").get_to (p.zerof_);
  if (p.type_ == ParameterRange::Type::Enumeration)
    {
      if (!j.contains ("enumLabels"))
        {
          throw std::runtime_error (
            "Enumeration parameter missing required 'enumLabels'");
        }
      j.at ("enumLabels").get_to (p.enum_labels_);
      if (p.enum_labels_.empty ())
        {
          throw std::runtime_error (
            "Enumeration parameter has empty enumLabels");
        }
      const auto expected_count =
        static_cast<size_t> (std::round (p.maxf_ - p.minf_ + 1.f));
      if (p.enum_labels_.size () != expected_count)
        {
          throw std::runtime_error (
            fmt::format (
              "Enumeration label count ({}) does not match range ({}-{}+1={})",
              p.enum_labels_.size (), p.minf_, p.maxf_, expected_count));
        }
    }
}

ProcessorParameter::ProcessorParameter (
  PortRegistry     &port_registry,
  UniqueId          unique_id,
  ParameterRange    range,
  utils::Utf8String label,
  QObject *         parent)
    : QObject (parent), unique_id_ (std::move (unique_id)),
      range_ (std::move (range)), label_ (std::move (label)),
      base_value_ (range_.convertTo0To1 (range_.deff_)),
      modulation_input_uuid_ (
        { port_registry.create_object<
          CVPort> (label_ + u8" Mod In", PortFlow::Input) })
{
}

void
ProcessorParameter::process_block (
  dsp::graph::EngineProcessTimeInfo time_nfo,
  const dsp::ITransport            &transport,
  const dsp::TempoMap              &tempo_map) noexcept
{
  float current_val = base_value_.load ();

  if (during_gesture_.load ())
    {
      last_automated_value_.store (current_val);
      last_modulated_value_.store (current_val);
      return;
    }

  /* calculate value from automation track */
  if (automation_value_provider_)
    {
      const auto val = std::invoke (
        automation_value_provider_.value (), time_nfo.g_start_frame_w_offset_);
      if (val.has_value ())
        {
          current_val = std::clamp (val.value (), 0.f, 1.f);
          last_automated_value_.store (current_val);
        }
    }
  else
    {
      last_automated_value_.store (current_val);
    }

  /* whether this is the first CV processed on this control port */
  const auto * cv_mod_in = modulation_input_;
  for (const auto &[src_port, conn] : cv_mod_in->port_sources ())
    {
      if (!conn->enabled_) [[unlikely]]
        continue;

      // modulation value [0, 1]
      auto modulation_base_val =
        src_port->buf_[time_nfo.local_offset_.in (units::samples)];

      // if bipolar, convert to [-1, 1]
      if (conn->bipolar_)
        {
          modulation_base_val = modulation_base_val * 2.f - 1.f;
        }

      current_val = std::clamp<float> (
        current_val + (modulation_base_val * conn->multiplier_), 0.f, 1.f);
    }

  last_modulated_value_.store (current_val);
}

void
ProcessorParameter::prepare_for_processing (
  const graph::GraphNode * node,
  units::sample_rate_t     sample_rate,
  units::sample_u32_t      max_block_length)
{
  modulation_input_ = modulation_input_uuid_.get_object_as<dsp::CVPort> ();
}
void
ProcessorParameter::release_resources ()
{
  modulation_input_ = nullptr;
}

utils::Utf8String
ProcessorParameter::get_node_name () const
{
  return label_;
}

void
to_json (nlohmann::json &j, const ProcessorParameter &p)
{
  to_json (
    j, static_cast<const ProcessorParameter::UuidIdentifiableObject &> (p));
  j[ProcessorParameter::kUniqueIdKey] = p.unique_id_;
  j[ProcessorParameter::kRangeKey] = p.range_;
  j[ProcessorParameter::kLabelKey] = p.label_;
  if (p.symbol_.has_value ())
    j[ProcessorParameter::kSymbolKey] = *p.symbol_;
  if (p.description_.has_value ())
    j[ProcessorParameter::kDescriptionKey] = *p.description_;
  j[ProcessorParameter::kBaseValueKey] = p.base_value_.load ();
  j[ProcessorParameter::kAutomatableKey] = p.automatable_;
  j[ProcessorParameter::kHiddenKey] = p.hidden_;
  j[ProcessorParameter::kModulationSourcePortIdKey] = p.modulation_input_uuid_;
}

void
from_json (const nlohmann::json &j, ProcessorParameter &p)
{
  from_json (j, static_cast<ProcessorParameter::UuidIdentifiableObject &> (p));
  j.at (ProcessorParameter::kUniqueIdKey).get_to (p.unique_id_);
  j.at (ProcessorParameter::kRangeKey).get_to (p.range_);
  j.at (ProcessorParameter::kLabelKey).get_to (p.label_);
  if (j.contains (ProcessorParameter::kSymbolKey))
    j.at (ProcessorParameter::kSymbolKey).get_to (p.symbol_);
  if (j.contains (ProcessorParameter::kDescriptionKey))
    {
      p.description_ = utils::Utf8String::from_utf8_encoded_string (
        j.at (ProcessorParameter::kDescriptionKey).get<std::string> ());
    }
  float base_val{};
  j.at (ProcessorParameter::kBaseValueKey).get_to (base_val);
  p.base_value_.store (base_val);
  if (j.contains (ProcessorParameter::kAutomatableKey))
    j.at (ProcessorParameter::kAutomatableKey).get_to (p.automatable_);
  if (j.contains (ProcessorParameter::kHiddenKey))
    j.at (ProcessorParameter::kHiddenKey).get_to (p.hidden_);
  j.at (ProcessorParameter::kModulationSourcePortIdKey)
    .get_to (p.modulation_input_uuid_);
}
} // namespace zrythm::dsp
