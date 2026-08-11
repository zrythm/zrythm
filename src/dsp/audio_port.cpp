// SPDX-FileCopyrightText: © 2018-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/panning.h"
#include "dsp/port_all.h"
#include "utils/float_ranges.h"
#include "utils/logger.h"
#include "utils/math_utils.h"
#include "utils/views.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace zrythm::dsp
{
AudioPort::AudioPort (
  utils::Utf8String  label,
  PortFlow           flow,
  SpeakerArrangement arrangement,
  Purpose            purpose)
    : Port (std::move (label), PortType::Audio, flow),
      arrangement_ (arrangement), purpose_ (purpose)
{
}

void
AudioPort::set_arrangement (SpeakerArrangement new_arrangement)
{
  if (arrangement_ == new_arrangement)
    return;
  arrangement_ = new_arrangement;
  // drop the buffer so that a missed re-prepare trips the null-buffer
  // assertions in the processing path instead of indexing a buffer with the
  // old channel count
  buf_.reset ();
}

void
init_from (
  AudioPort             &obj,
  const AudioPort       &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Port &> (obj), static_cast<const Port &> (other), clone_type);
  obj.arrangement_ = other.arrangement_;
  obj.purpose_ = other.purpose_;
}

void
AudioPort::add_source_rt (
  const AudioPort              &src,
  const AudioBusChannelRouting &routing,
  dsp::graph::ProcessBlockInfo  time_nfo,
  float                         multiplier)
{
  assert (buf_ != nullptr && src.buf_ != nullptr);

  const auto offset = time_nfo.buffer_offset_.in<int> (units::samples);
  const auto nframes = time_nfo.nframes_.in<int> (units::samples);

  // destination channels with no contribution keep the signal already summed
  // into them by other sources
  routing.for_each_route (
    src.arrangement_, arrangement_,
    [&] (unsigned dest_ch, unsigned src_ch, float gain) {
      assert (dest_ch < num_channels () && src_ch < src.num_channels ());
      buf_->addFrom (
        static_cast<int> (dest_ch), offset, *src.buf_,
        static_cast<int> (src_ch), offset, nframes, gain * multiplier);
    });
}

void
AudioPort::copy_source_rt (
  const AudioPort              &src,
  const AudioBusChannelRouting &routing,
  dsp::graph::ProcessBlockInfo  time_nfo,
  float                         multiplier)
{
  assert (buf_ != nullptr && src.buf_ != nullptr);

  const auto offset = time_nfo.buffer_offset_.in<int> (units::samples);
  const auto nframes = time_nfo.nframes_.in<int> (units::samples);

  // a channel for channel routing writes every destination channel exactly
  // once, so it can overwrite in place
  if (routing.is_channel_for_channel (src.arrangement_, arrangement_))
    {
      const auto unity_gain =
        utils::math::floats_near (multiplier, 1.f, 0.00001f);
      for (const auto ch : std::views::iota (0u, num_channels ()))
        {
          if (unity_gain)
            {
              buf_->copyFrom (
                static_cast<int> (ch), offset, *src.buf_, static_cast<int> (ch),
                offset, nframes);
            }
          else
            {
              buf_->copyFrom (
                static_cast<int> (ch), offset,
                src.buf_->getReadPointer (static_cast<int> (ch), offset),
                nframes, multiplier);
            }
        }
      return;
    }

  // otherwise a destination channel may take several contributions or none at
  // all, so start from silence and accumulate
  for (const auto ch : std::views::iota (0u, num_channels ()))
    {
      buf_->clear (static_cast<int> (ch), offset, nframes);
    }
  routing.for_each_route (
    src.arrangement_, arrangement_,
    [&] (unsigned dest_ch, unsigned src_ch, float gain) {
      assert (dest_ch < num_channels () && src_ch < src.num_channels ());
      buf_->addFrom (
        static_cast<int> (dest_ch), offset, *src.buf_,
        static_cast<int> (src_ch), offset, nframes, gain * multiplier);
    });
}

void
AudioPort::clear_buffer (std::size_t offset, std::size_t nframes)
{
  assert (buf_ != nullptr);
  buf_->clear (static_cast<int> (offset), static_cast<int> (nframes));
}

void
AudioPort::prepare_for_processing_impl (
  const graph::GraphNode * node,
  units::sample_rate_t     sample_rate,
  units::sample_u32_t      max_block_length)
{
  assert (
    node == nullptr
    || std::addressof (node->get_processable ())
         == static_cast<graph::IProcessable *> (this));

  if (node != nullptr && flow () == PortFlow::Input)
    {
      auto source_audio_ports =
        node->depends () | std::views::transform ([] (const auto &child_node) {
          return dynamic_cast<AudioPort *> (
            &child_node.get ().get_processable ());
        })
        | utils::views::filter_null;
      set_port_sources (source_audio_ports);
    }

  auto max = std::max (max_block_length, units::samples (1u));
  buf_ = std::make_unique<juce::AudioSampleBuffer> (
    num_channels (), max.in<int> (units::samples));
  buf_->clear ();
}

void
AudioPort::release_resources ()
{
  buf_.reset ();
}

void
AudioPort::process_block (
  dsp::graph::ProcessBlockInfo time_nfo,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  // detached ports are excluded from the graph and from their owner's
  // processing, so reaching this is a programmer error
  assert (!detached ());

  /* Input ports: aggregate from sources. */
  if (flow () == PortFlow::Input)
    {
      for (const auto &[_src_port, conn] : port_sources ())
        {
          if (!conn->enabled_)
            continue;

          const auto * src_port = dynamic_cast<const AudioPort *> (_src_port);

          add_source_rt (
            *src_port, conn->audio_bus_channel_routing_, time_nfo,
            conn->multiplier_);
        }
    }

  /* Limiting + ring buffer (both input and output). */
  if (requires_limiting_)
    {
      constexpr float max_allowed_peak = 2.f;
      float           abs_peak = buf_->getMagnitude (
        time_nfo.buffer_offset_.in<int> (units::samples),
        time_nfo.nframes_.in<int> (units::samples));
      if (abs_peak > max_allowed_peak)
        {
          for (const auto ch : std::views::iota (0u, num_channels ()))
            {
              /* this limiting wastes around 50% of port processing so only do
               * it if we exceed maxf */
              utils::float_ranges::clip (
                { buf_->getWritePointer (
                    static_cast<int> (ch),
                    time_nfo.buffer_offset_.in<int> (units::samples)),
                  time_nfo.nframes_.in (units::samples) },
                -max_allowed_peak, max_allowed_peak);
            }
        }
    }
}

void
to_json (nlohmann::json &j, const AudioPort &port)
{
  to_json (j, static_cast<const Port &> (port));
  j[AudioPort::kSpeakerArrangementId] = port.arrangement_;
  j[AudioPort::kPurposeId] = port.purpose_;
  j[AudioPort::kRequiresLimitingId] = port.requires_limiting_;
}

void
from_json (const nlohmann::json &j, AudioPort &port)
{
  from_json (j, static_cast<Port &> (port));
  j.at (AudioPort::kSpeakerArrangementId).get_to (port.arrangement_);
  j.at (AudioPort::kPurposeId).get_to (port.purpose_);
  j.at (AudioPort::kRequiresLimitingId).get_to (port.requires_limiting_);
}
} // namespace zrythm::dsp
