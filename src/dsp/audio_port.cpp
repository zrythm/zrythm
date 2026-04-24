// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/panning.h"
#include "dsp/port_all.h"
#include "utils/float_ranges.h"

#include <fmt/format.h>

namespace zrythm::dsp
{
AudioPort::AudioPort (
  utils::Utf8String label,
  PortFlow          flow,
  BusLayout         layout,
  uint8_t           num_channels,
  Purpose           purpose)
    : Port (std::move (label), PortType::Audio, flow), layout_ (layout),
      purpose_ (purpose), num_channels_ (num_channels)
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
  obj.layout_ = other.layout_;
  obj.purpose_ = other.purpose_;
}

void
AudioPort::add_source_rt (
  const AudioPort                  &src,
  dsp::graph::EngineProcessTimeInfo time_nfo,
  float                             multiplier)
{
  const auto add_src =
    [time_nfo, &src, this] (const auto dest_ch, const auto src_ch, float gain) {
      buf_->addFrom (
        static_cast<int> (dest_ch),
        time_nfo.local_offset_.in<int> (units::samples), *src.buf_,
        static_cast<int> (src_ch),
        time_nfo.local_offset_.in<int> (units::samples),
        time_nfo.nframes_.in<int> (units::samples), gain);
    };

  if (src.num_channels_ == num_channels_)
    {
      // Copy each port
      for (const auto ch : std::views::iota (0u, num_channels_))
        {
          add_src (ch, ch, multiplier);
        }
    }
  else if (src.layout_ == BusLayout::Mono && layout_ == BusLayout::Stereo)
    {
      // Copy mono signal to both L and R
      for (const auto ch : std::views::iota (0u, num_channels_))
        {
          add_src (ch, 0, multiplier);
        }
    }
  else if (src.layout_ == BusLayout::Stereo && layout_ == BusLayout::Mono)
    {
      // Sum L+R at -6dB
      const auto multipliers =
        calculate_panning (PanLaw::Minus6dB, PanAlgorithm::SquareRoot, 0.5f);
      add_src (0, 0, multipliers.first * multiplier);
      add_src (0, 1, multipliers.second * multiplier);
    }
  else
    {
      // unsupported...
      buf_->clear ();
    }
}

void
AudioPort::copy_source_rt (
  const AudioPort                  &src,
  dsp::graph::EngineProcessTimeInfo time_nfo,
  float                             multiplier)
{
  const auto add_src =
    [time_nfo, &src, this] (const auto dest_ch, const auto src_ch, float gain) {
      if (utils::math::floats_near (gain, 1.f, 0.00001f))
        {
          buf_->copyFrom (
            static_cast<int> (dest_ch),
            time_nfo.local_offset_.in<int> (units::samples), *src.buf_,
            static_cast<int> (src_ch),
            time_nfo.local_offset_.in<int> (units::samples),
            time_nfo.nframes_.in<int> (units::samples));
        }
      else
        {
          buf_->copyFrom (
            static_cast<int> (dest_ch),
            time_nfo.local_offset_.in<int> (units::samples),
            src.buf_->getReadPointer (
              static_cast<int> (src_ch),
              time_nfo.local_offset_.in<int> (units::samples)),
            time_nfo.nframes_.in<int> (units::samples), gain);
        }
    };

  if (src.num_channels_ == num_channels_)
    {
      // Copy each port
      for (const auto ch : std::views::iota (0u, num_channels_))
        {
          add_src (ch, ch, multiplier);
        }
    }
  else if (src.layout_ == BusLayout::Mono && layout_ == BusLayout::Stereo)
    {
      // Copy mono signal to both L and R
      for (const auto ch : std::views::iota (0u, num_channels_))
        {
          add_src (ch, 0, multiplier);
        }
    }
  else if (src.layout_ == BusLayout::Stereo && layout_ == BusLayout::Mono)
    {
      // Sum L+R at -6dB
      const auto multipliers =
        calculate_panning (PanLaw::Minus6dB, PanAlgorithm::SquareRoot, 0.5f);
      add_src (0, 0, multipliers.first * multiplier);
      add_src (0, 1, multipliers.second * multiplier);
    }
  else
    {
      // unsupported...
      buf_->clear ();
    }
}

void
AudioPort::clear_buffer (std::size_t offset, std::size_t nframes)
{
  assert (buf_ != nullptr);
  buf_->clear (static_cast<int> (offset), static_cast<int> (nframes));
}

void
AudioPort::prepare_for_processing (
  const graph::GraphNode * node,
  units::sample_rate_t     sample_rate,
  units::sample_u32_t      max_block_length)
{
  if (node != nullptr)
    {
      auto source_audio_ports =
        node->depends () | std::views::transform ([] (const auto &child_node) {
          return dynamic_cast<AudioPort *> (
            &child_node.get ().get_processable ());
        })
        | std::views::filter ([] (const auto * port) { return port != nullptr; });
      set_port_sources (source_audio_ports);
    }

  auto max = std::max (max_block_length, units::samples (1u));
  buf_ = std::make_unique<juce::AudioSampleBuffer> (
    num_channels_, max.in<int> (units::samples));
  buf_->clear ();

  // 8 cycles
  audio_ring_.clear ();
  std::ranges::for_each (std::views::iota (0u, num_channels_), [&] (const auto &) {
    audio_ring_.emplace_back (max.in (units::samples) * 8);
  });
}

void
AudioPort::release_resources ()
{
  buf_.reset ();
  audio_ring_.clear ();
}

void
AudioPort::process_block (
  dsp::graph::EngineProcessTimeInfo time_nfo,
  const dsp::ITransport            &transport,
  const dsp::TempoMap              &tempo_map) noexcept
{
  for (const auto &[_src_port, conn] : port_sources ())
    {
      if (!conn->enabled_)
        continue;

      const auto * src_port = dynamic_cast<const AudioPort *> (_src_port);
      const float  multiplier = conn->multiplier_;

      if (conn->source_ch_to_destination_ch_mapping_.has_value ())
        {
          const auto [source_ch, dest_ch] =
            conn->source_ch_to_destination_ch_mapping_.value ();

          /* sum the signals */
          buf_->addFrom (
            static_cast<int> (dest_ch),
            time_nfo.local_offset_.in<int> (units::samples), *src_port->buf_,
            static_cast<int> (source_ch),
            time_nfo.local_offset_.in<int> (units::samples),
            time_nfo.nframes_.in<int> (units::samples), multiplier);
        }
      else
        {
          add_source_rt (*src_port, time_nfo, multiplier);
        }
    }

  if (requires_limiting_)
    {
      constexpr float max_allowed_peak = 2.f;
      float           abs_peak = buf_->getMagnitude (
        time_nfo.local_offset_.in<int> (units::samples),
        time_nfo.nframes_.in<int> (units::samples));
      if (abs_peak > max_allowed_peak)
        {
          for (const auto ch : std::views::iota (0u, num_channels_))
            {
              /* this limiting wastes around 50% of port processing so only do
               * it if we exceed maxf */
              utils::float_ranges::clip (
                { buf_->getWritePointer (
                    static_cast<int> (ch),
                    time_nfo.local_offset_.in<int> (units::samples)),
                  time_nfo.nframes_.in (units::samples) },
                -max_allowed_peak, max_allowed_peak);
            }
        }
    }

  if (num_ring_buffer_readers_ > 0)
    {
      for (const auto ch : std::views::iota (0u, num_channels_))
        {
          audio_ring_[ch].force_write_multiple (
            buf_->getReadPointer (
              static_cast<int> (ch),
              time_nfo.local_offset_.in<int> (units::samples)),
            time_nfo.nframes_.in (units::samples));
        }
    }
}

void
to_json (nlohmann::json &j, const AudioPort &port)
{
  to_json (j, static_cast<const Port &> (port));
  j[AudioPort::kBusLayoutId] = port.layout_;
  j[AudioPort::kPurposeId] = port.purpose_;
  j[AudioPort::kRequiresLimitingId] = port.requires_limiting_;
  j[AudioPort::kChannels] = port.num_channels_;
}

void
from_json (const nlohmann::json &j, AudioPort &port)
{
  from_json (j, static_cast<Port &> (port));
  j.at (AudioPort::kBusLayoutId).get_to (port.layout_);
  j.at (AudioPort::kPurposeId).get_to (port.purpose_);
  j.at (AudioPort::kRequiresLimitingId).get_to (port.requires_limiting_);
  j.at (AudioPort::kChannels).get_to (port.num_channels_);
}
} // namespace zrythm::dsp
