// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_sample_processor.h"
#include "utils/registry_utils.h"
#include "utils/types.h"

namespace zrythm::dsp
{

AudioSampleProcessor::AudioSampleProcessor (utils::IObjectRegistry &registry)
    : dsp::ProcessorBase (registry)
{
  auto out_ref = utils::create_object<dsp::AudioPort> (
    registry, u8"Stereo Out", PortFlow::Output, AudioPort::BusLayout::Stereo, 2);
  out_ref.get_object_as<dsp::AudioPort> ()->set_symbol (u8"stereo_out");
  add_output_port (out_ref);
  set_name (u8"Audio Sample Processor");
}

void
AudioSampleProcessor::custom_process_block (
  dsp::graph::ProcessBlockInfo time_nfo,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  const auto cycle_offset = time_nfo.buffer_offset_;
  const auto nframes = time_nfo.nframes_;

  auto * out_port = get_output_audio_port_rt ();

  // Clear output
  out_port->clear_buffer (
    cycle_offset.in (units::samples), nframes.in (units::samples));

  // Process the samples in the queue
  for (auto it = samples_to_play_.begin (); it != samples_to_play_.end ();)
    {
      auto &sp = *it;

      // If sample starts after this cycle, update offset and skip processing
      if (sp.start_offset_ >= nframes)
        {
          sp.start_offset_ -= nframes;
          ++it;
          continue;
        }

      const auto process_samples =
        [&] (const auto fader_buf_offset, const auto len) {
          if (sp.channel_index_ < 2) [[likely]]
            {
              out_port->buffers ()->addFrom (
                sp.channel_index_,
                static_cast<int> (fader_buf_offset.in (units::samples)),
                &sp.buf_[sp.offset_.in (units::samples)],
                len.template in<int> (units::samples), sp.volume_);
            }
          sp.offset_ += len;
        };

      // If sample is already playing
      if (sp.offset_ > units::samples (0))
        {
          const auto max_frames =
            au::min (units::samples (sp.buf_.size ()) - sp.offset_, nframes);
          process_samples (cycle_offset, max_frames);
        }
      // If we can start playback in this cycle
      else if (sp.start_offset_ >= cycle_offset)
        {
          const auto max_frames = au::min (
            units::samples (sp.buf_.size ()),
            (cycle_offset + nframes) - sp.start_offset_);
          process_samples (sp.start_offset_, max_frames);
        }

      // If the sample is finished playing, remove it
      if (sp.offset_ >= units::samples (sp.buf_.size ()))
        {
          it = samples_to_play_.erase (it);
        }
      else
        {
          ++it;
        }
    }
}
} // namespace zrythm::dsp
