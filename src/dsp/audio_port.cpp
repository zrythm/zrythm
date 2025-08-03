// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_all.h"
#include "utils/dsp.h"

#include <fmt/format.h>

namespace zrythm::dsp
{
AudioPort::AudioPort (utils::Utf8String label, PortFlow flow, bool is_stereo)
    : Port (std::move (label), PortType::Audio, flow), is_stereo_ (is_stereo)
{
}

void
init_from (
  AudioPort             &obj,
  const AudioPort       &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Port &> (obj), static_cast<const Port &> (other), clone_type);
}

void
AudioPort::clear_buffer (std::size_t offset, std::size_t nframes)
{
  utils::float_ranges::fill (&buf_[offset], 0.f, nframes);
}

void
AudioPort::process_block (const EngineProcessTimeInfo time_nfo)
{
  for (const auto &[_src_port, conn] : port_sources_)
    {
      if (!conn->enabled_)
        continue;

      const auto * src_port = dynamic_cast<const AudioPort *> (_src_port);
      const float  multiplier = conn->multiplier_;

      /* sum the signals */
      if (utils::math::floats_near (multiplier, 1.f, 0.00001f)) [[likely]]
        {
          utils::float_ranges::add2 (
            &buf_[time_nfo.local_offset_],
            &src_port->buf_[time_nfo.local_offset_], time_nfo.nframes_);
        }
      else
        {
          utils::float_ranges::mix_product (
            &buf_[time_nfo.local_offset_],
            &src_port->buf_[time_nfo.local_offset_], multiplier,
            time_nfo.nframes_);
        }

      if (requires_limiting_)
        {
          constexpr float minf = -2.f;
          constexpr float maxf = 2.f;
          float           abs_peak = utils::float_ranges::abs_max (
            &buf_[time_nfo.local_offset_], time_nfo.nframes_);
          if (abs_peak > maxf)
            {
              /* this limiting wastes around 50% of port processing so only do
               * it if we exceed maxf */
              utils::float_ranges::clip (
                &buf_[time_nfo.local_offset_], minf, maxf, time_nfo.nframes_);
            }
        }
    }

  if (num_ring_buffer_readers_ > 0)
    {
      audio_ring_->force_write_multiple (
        &buf_[time_nfo.local_offset_], time_nfo.nframes_);
    }
}

std::pair<PortUuidReference, PortUuidReference>
StereoPorts::create_stereo_ports (
  PortRegistry     &port_registry,
  bool              input,
  utils::Utf8String name,
  utils::Utf8String symbol)
{
  auto l_names = get_name_and_symbols (true, name, symbol);
  auto r_names = get_name_and_symbols (false, name, symbol);
  auto l_port_ref = port_registry.create_object<AudioPort> (
    l_names.first, input ? dsp::PortFlow::Input : dsp::PortFlow::Output, true);
  auto r_port_ref = port_registry.create_object<AudioPort> (
    r_names.first, input ? dsp::PortFlow::Input : dsp::PortFlow::Output, true);
  {
    auto * l_port = std::get<AudioPort *> (l_port_ref.get_object ());
    auto * r_port = std::get<AudioPort *> (r_port_ref.get_object ());
    l_port->set_symbol (l_names.second);
    r_port->set_symbol (r_names.second);
  }
  return std::make_pair (l_port_ref, r_port_ref);
}
} // namespace zrythm::dsp
