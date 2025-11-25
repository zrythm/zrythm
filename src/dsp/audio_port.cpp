// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/panning.h"
#include "dsp/port_all.h"
#include "utils/dsp.h"

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
  const AudioPort      &src,
  EngineProcessTimeInfo time_nfo,
  float                 multiplier)
{
  const auto add_src =
    [time_nfo, &src, this] (const auto dest_ch, const auto src_ch, float gain) {
      buf_->addFrom (
        static_cast<int> (dest_ch), static_cast<int> (time_nfo.local_offset_),
        *src.buf_, static_cast<int> (src_ch),
        static_cast<int> (time_nfo.local_offset_),
        static_cast<int> (time_nfo.nframes_), gain);
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
  const AudioPort      &src,
  EngineProcessTimeInfo time_nfo,
  float                 multiplier)
{
  const auto add_src =
    [time_nfo, &src, this] (const auto dest_ch, const auto src_ch, float gain) {
      if (utils::math::floats_near (gain, 1.f, 0.00001f))
        {
          buf_->copyFrom (
            static_cast<int> (dest_ch),
            static_cast<int> (time_nfo.local_offset_), *src.buf_,
            static_cast<int> (src_ch), static_cast<int> (time_nfo.local_offset_),
            static_cast<int> (time_nfo.nframes_));
        }
      else
        {
          buf_->copyFrom (
            static_cast<int> (dest_ch),
            static_cast<int> (time_nfo.local_offset_),
            src.buf_->getReadPointer (
              static_cast<int> (src_ch),
              static_cast<int> (time_nfo.local_offset_)),
            static_cast<int> (time_nfo.nframes_), gain);
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
  sample_rate_t            sample_rate,
  nframes_t                max_block_length)
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

  size_t max = std::max (max_block_length, 1u);
  buf_ = std::make_unique<juce::AudioSampleBuffer> (
    num_channels_, static_cast<int> (max));
  buf_->clear ();

  // 8 cycles
  audio_ring_.clear ();
  std::ranges::for_each (
    std::views::iota (0u, num_channels_),
    [&] (const auto &) { audio_ring_.emplace_back (max * 8); });
}

void
AudioPort::release_resources ()
{
  buf_.reset ();
  audio_ring_.clear ();
}

void
AudioPort::process_block (
  EngineProcessTimeInfo  time_nfo,
  const dsp::ITransport &transport) noexcept
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
            static_cast<int> (time_nfo.local_offset_), *src_port->buf_,
            static_cast<int> (source_ch),
            static_cast<int> (time_nfo.local_offset_),
            static_cast<int> (time_nfo.nframes_), multiplier);
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
        static_cast<int> (time_nfo.local_offset_),
        static_cast<int> (time_nfo.nframes_));
      if (abs_peak > max_allowed_peak)
        {
          for (const auto ch : std::views::iota (0u, num_channels_))
            {
              /* this limiting wastes around 50% of port processing so only do
               * it if we exceed maxf */
              utils::float_ranges::clip (
                buf_->getWritePointer (
                  static_cast<int> (ch),
                  static_cast<int> (time_nfo.local_offset_)),
                -max_allowed_peak, max_allowed_peak, time_nfo.nframes_);
            }
        }
    }

  if (num_ring_buffer_readers_ > 0)
    {
      for (const auto ch : std::views::iota (0u, num_channels_))
        {
          audio_ring_[ch].force_write_multiple (
            buf_->getReadPointer (
              static_cast<int> (ch), static_cast<int> (time_nfo.local_offset_)),
            time_nfo.nframes_);
        }
    }
}
} // namespace zrythm::dsp
