// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <source_location>

#include "dsp/processor_base.h"

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
      uint8_t              channel_index,
      float                volume,
      units::sample_u32_t  start_offset,
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
    uint8_t channel_index_;

    /** The current offset in the buffer. */
    units::sample_u64_t offset_;

    /** The volume to play the sample at (ratio from
     * 0.0 to 2.0, where 1.0 is the normal volume). */
    float volume_ = 1.0f;

    /** Offset relative to the current processing cycle
     * to start playing the sample. */
    units::sample_u32_t start_offset_;

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
    dsp::graph::EngineProcessTimeInfo time_nfo,
    const dsp::ITransport            &transport,
    const dsp::TempoMap              &tempo_map) noexcept override;

  void custom_prepare_for_processing (
    const graph::GraphNode * node,
    units::sample_rate_t     sample_rate,
    units::sample_u32_t      max_block_length) override
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
