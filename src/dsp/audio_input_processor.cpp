// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_input_processor.h"
#include "dsp/audio_port.h"
#include "utils/float_ranges.h"

namespace zrythm::dsp
{

AudioInputProcessor::AudioInputProcessor (
  InputDataProvider         provider,
  units::channel_count_t    hw_input_channel_count,
  ProcessorBaseDependencies dependencies)
    : ProcessorBase (
        dependencies,
        utils::Utf8String::from_utf8_encoded_string ("Audio Input Processor")),
      provider_ (std::move (provider))
{
  const auto hw_chans = hw_input_channel_count.in<int> (units::channels);
  for (int i = 0; i + 1 < hw_chans; i += 2)
    {
      auto port_ref = dependencies.port_registry_.create_object<AudioPort> (
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("Input {}-{}", i + 1, i + 2)),
        PortFlow::Output, AudioPort::BusLayout::Stereo, 2);
      add_output_port (port_ref);
      port_mappings_.push_back ({ port_ref.get_object_as<AudioPort> (), i, 2 });
    }

  for (int i = 0; i < hw_chans; ++i)
    {
      auto port_ref = dependencies.port_registry_.create_object<AudioPort> (
        utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("Input {}", i + 1)),
        PortFlow::Output, AudioPort::BusLayout::Mono, 1);
      add_output_port (port_ref);
      port_mappings_.push_back ({ port_ref.get_object_as<AudioPort> (), i, 1 });
    }
}

void
AudioInputProcessor::custom_process_block (
  dsp::graph::EngineProcessTimeInfo time_nfo,
  const dsp::ITransport            &transport,
  const dsp::TempoMap              &tempo_map) noexcept
{
  auto channels = provider_ ();

  if (channels.empty ())
    return;

  const auto nframes = time_nfo.nframes_.in<size_t> (units::samples);
  const auto offset = time_nfo.local_offset_.in<int> (units::samples);
  const auto num_channels = static_cast<int> (channels.size ());

  for (const auto &mapping : port_mappings_)
    {
      auto * buf = mapping.port->buffers ().get ();
      for (int ch = 0; ch < mapping.src_channel_count; ++ch)
        {
          const int src_ch = mapping.src_channel_start + ch;
          if (src_ch < num_channels && channels[src_ch] != nullptr)
            {
              utils::float_ranges::copy (
                { buf->getWritePointer (ch, offset), nframes },
                { channels[src_ch] + offset, nframes });
            }
        }
    }
}

}
