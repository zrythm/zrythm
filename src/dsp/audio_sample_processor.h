// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <source_location>

#include "dsp/processor_base.h"
#include "utils/dsp.h"
#include "utils/types.h"

#include <boost/container/static_vector.hpp>

namespace zrythm::dsp
{

class AudioSampleProcessor : public dsp::ProcessorBase
{
public:
  AudioSampleProcessor (
    dsp::ProcessorBase::ProcessorBaseDependencies dependencies)
      : dsp::ProcessorBase (dependencies)
  {

    auto out_ref = dependencies.port_registry_.create_object<dsp::AudioPort> (
      u8"Stereo Out", PortFlow::Output, AudioPort::BusLayout::Stereo, 2);
    out_ref.get_object_as<dsp::AudioPort> ()->set_symbol (u8"stereo_out");
    add_output_port (out_ref);
    set_name (u8"Audio Sample Processor");
  }

  /**
   * A sample playback handle to be used by the engine.
   *
   * Audio with multiple channels should call this once for each channel.
   */
  struct PlayableSampleSingleChannel
  {
    using UnmutableSampleSpan = std::span<const float>;
    PlayableSampleSingleChannel (
      UnmutableSampleSpan  buf,
      channels_t           channel_index,
      float                volume,
      nframes_t            start_offset,
      std::source_location source_location)
        : buf_ (buf), channel_index_ (channel_index), volume_ (volume),
          start_offset_ (start_offset), source_location_ (source_location)
    {
    }
    /** Samples to play for a single channel. */
    UnmutableSampleSpan buf_;

    /**
     * @brief Channel index to play this sample at.
     */
    channels_t channel_index_;

    /** The current offset in the buffer. */
    unsigned_frame_t offset_ = 0;

    /** The volume to play the sample at (ratio from
     * 0.0 to 2.0, where 1.0 is the normal volume). */
    float volume_ = 1.0f;

    /** Offset relative to the current processing cycle
     * to start playing the sample. */
    nframes_t start_offset_ = 0;

    // For debugging purposes
    std::source_location source_location_;
  };

  using QueueSingleChannelSampleCallback =
    std::function<void (PlayableSampleSingleChannel)>;

  /**
   * @brief Adds a sample to the queue.
   *
   * @warning This must only be called during the processing cycle before
   * AudioSampleProcessor gets processed (eg, at the start of the processing
   * cycle by the graph dispatcher, or in a node that is guaranteed to be
   * processed before this). It is not thread-safe otherwise.
   */
  void add_sample_to_process (PlayableSampleSingleChannel sample)
  {
    if (sample.buf_.empty ()) [[unlikely]]
      {
        // ignore empty samples
        return;
      }

    samples_to_play_.emplace_back (sample);
    if (queue_sample_cb_.has_value ())
      {
        std::invoke (queue_sample_cb_.value (), sample);
      }
  }

  void set_queue_sample_callback (QueueSingleChannelSampleCallback cb)
  {
    queue_sample_cb_ = std::move (cb);
  }

  auto get_output_audio_port_non_rt () const
  {
    return get_output_ports ()[0].get_object_as<dsp::AudioPort> ();
  }

  auto get_output_audio_port_rt () const { return audio_out_; }

  void custom_process_block (
    EngineProcessTimeInfo  time_nfo,
    const dsp::ITransport &transport) noexcept override
  {
    const auto cycle_offset = time_nfo.local_offset_;
    const auto nframes = time_nfo.nframes_;

    auto * out_port = get_output_audio_port_rt ();

    // Clear output
    out_port->clear_buffer (cycle_offset, nframes);

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
          [&] (nframes_t fader_buf_offset, nframes_t len) {
            if (sp.channel_index_ < 2) [[likely]]
              {
                out_port->buffers ()->addFrom (
                  sp.channel_index_, static_cast<int> (fader_buf_offset),
                  &sp.buf_[sp.offset_], static_cast<int> (len), sp.volume_);
              }
            sp.offset_ += len;
          };

        // If sample is already playing
        if (sp.offset_ > 0)
          {
            const auto max_frames =
              std::min ((nframes_t) (sp.buf_.size () - sp.offset_), nframes);
            process_samples (cycle_offset, max_frames);
          }
        // If we can start playback in this cycle
        else if (sp.start_offset_ >= cycle_offset)
          {
            const auto max_frames = std::min (
              (nframes_t) sp.buf_.size (),
              (cycle_offset + nframes) - sp.start_offset_);
            process_samples (sp.start_offset_, max_frames);
          }

        // If the sample is finished playing, remove it
        if (sp.offset_ >= (unsigned_frame_t) sp.buf_.size ())
          {
            it = samples_to_play_.erase (it);
          }
        else
          {
            ++it;
          }
      }
  }

  void custom_prepare_for_processing (
    sample_rate_t sample_rate,
    nframes_t     max_block_length) override
  {
    samples_to_play_.clear ();
    audio_out_ = get_output_ports ()[0].get_object_as<dsp::AudioPort> ();
  }

  void custom_release_resources () override
  {
    samples_to_play_.clear ();
    audio_out_ = nullptr;
  }

private:
  /**
   * @brief Currently active samples to play back.
   *
   * These will be added/removed dynamically as needed.
   */
  boost::container::static_vector<PlayableSampleSingleChannel, 128>
    samples_to_play_;

  /**
   * @brief Optional callback when adding samples.
   *
   * Used for debugging.
   */
  std::optional<QueueSingleChannelSampleCallback> queue_sample_cb_;

  AudioPort * audio_out_{};
};
} // namespace zrythm::dsp
