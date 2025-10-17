// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "dsp/parameter.h"
#include "dsp/port_all.h"

namespace zrythm::dsp
{

ProcessorParameter::ProcessorParameter (
  PortRegistry     &port_registry,
  UniqueId          unique_id,
  ParameterRange    range,
  utils::Utf8String label,
  QObject *         parent)
    : QObject (parent), unique_id_ (std::move (unique_id)), range_ (range),
      label_ (std::move (label)), base_value_ (range.convertTo0To1 (range.deff_)),
      modulation_input_uuid_ (
        { port_registry.create_object<
          CVPort> (label_ + u8" Mod In", PortFlow::Input) })
{
}

void
ProcessorParameter::process_block (const EngineProcessTimeInfo time_nfo) noexcept
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
        automation_value_provider_.value (),
        units::samples (time_nfo.g_start_frame_w_offset_));
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
  for (const auto &[src_port, conn] : cv_mod_in->port_sources_)
    {
      if (!conn->enabled_) [[unlikely]]
        continue;

      // modulation value [0, 1]
      auto modulation_base_val = src_port->buf_[time_nfo.local_offset_];

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
  sample_rate_t sample_rate,
  nframes_t     max_block_length)
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
} // namespace zrythm::dsp
