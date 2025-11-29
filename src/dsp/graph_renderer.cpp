// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_port.h"
#include "dsp/ditherer.h"
#include "dsp/graph_renderer.h"
#include "dsp/graph_scheduler.h"
#include "dsp/transport.h"

#include <QtConcurrentRun>

namespace zrythm::dsp
{
void
GraphRenderer::render (
  QPromise<juce::AudioSampleBuffer> &promise,
  RenderOptions                      options,
  graph::GraphNodeCollection       &&nodes,
  SampleRange                        range)
{
  z_debug ("Rendering range {}...", range);

  graph::GraphScheduler graph_scheduler (
    options.sample_rate_, options.block_length_.in (units::samples), false);

  graph_scheduler.rechain_from_node_collection (
    std::move (nodes), options.sample_rate_,
    options.block_length_.in (units::samples));
  graph_scheduler.start_threads (options.num_threads_);

  // Update latencies and get max latency for preroll
  graph_scheduler.get_nodes ().update_latencies ();
  auto max_latency_frames =
    graph_scheduler.get_nodes ().get_max_route_playback_latency ();

  const auto total_frames = range.second - range.first;
  const auto num_samples = total_frames.in<int> (units::samples);

  // Initialize output buffer with latency preroll added
  const auto total_samples_with_latency = num_samples + max_latency_frames;
  juce::AudioSampleBuffer output{
    2, static_cast<int> (total_samples_with_latency)
  };
  output.clear ();

  // Initialize ditherer if needed
  dsp::Ditherer ditherer;
  if (options.dither_)
    {
      ditherer.reset (utils::audio::bit_depth_enum_to_int (options.bit_depth_));
    }

  // Create temporary buffer for processing each block
  utils::audio::AudioBuffer temp_buffer{
    2, options.block_length_.in<int> (units::samples)
  };

  // Setup transport snapshot for rendering
  Transport::TransportSnapshot transport_snapshot{
    std::make_pair (units::samples (0), units::samples (0)), // loop_range
    std::make_pair (units::samples (0), units::samples (0)), // punch_range
    range.first,                         // playhead_position
    units::samples (0),                  // recording_preroll_frames_remaining
    units::samples (0),                  // metronome_countin_frames_remaining
    dsp::ITransport::PlayState::Rolling, // play_state
    false,                               // loop_enabled
    false,                               // punch_enabled
    false                                // recording_enabled
  };

  // Process in blocks
  signed_frame_t covered_frames = 0;
  auto           current_pos = range.first;
  auto           latency_preroll_frames = max_latency_frames;

  // Prepare for progress reporting
  promise.setProgressRange (0, output.getNumSamples ());

  while (current_pos < range.second)
    {
      promise.suspendIfRequested ();
      if (promise.isCanceled ())
        {
          return;
        }

      // Calculate number of frames to process in this block
      const auto frames_remaining = range.second - current_pos;
      const auto nframes = [&] () {
        const auto num_frames = std::min (
          frames_remaining.in<signed_frame_t> (units::samples),
          options.block_length_.in<signed_frame_t> (units::samples));
        return latency_preroll_frames > 0
                 ? std::min (
                     static_cast<signed_frame_t> (latency_preroll_frames), num_frames)
                 : num_frames;
      }();

      // Setup time info for this processing cycle
      EngineProcessTimeInfo time_nfo = {
        .g_start_frame_ = current_pos.in<unsigned_frame_t> (units::samples),
        .g_start_frame_w_offset_ =
          current_pos.in<unsigned_frame_t> (units::samples),
        .local_offset_ = 0,
        .nframes_ = static_cast<nframes_t> (nframes),
      };

      // Only update transport position after latency preroll is exhausted
      if (latency_preroll_frames <= 0)
        {
          transport_snapshot.set_position (current_pos);
        }

      // Process block with latency compensation
      graph_scheduler.run_cycle (
        time_nfo, latency_preroll_frames, transport_snapshot);

      // Collect audio from terminal nodes
      temp_buffer.clear ();
      for (const auto &node : graph_scheduler.get_nodes ().terminal_nodes_)
        {
          auto &processable = node.get ().get_processable ();
          if (auto * audio_port = dynamic_cast<dsp::AudioPort *> (&processable))
            {
              for (
                const auto channel_index : std::views::iota (
                  0,
                  std::min (
                    temp_buffer.getNumChannels (),
                    static_cast<int> (audio_port->num_channels ()))))
                {
                  temp_buffer.addFrom (
                    channel_index, 0, *audio_port->buffers (), channel_index, 0,
                    static_cast<int> (nframes));
                }
            }
        }

      // Apply dithering if enabled
      if (options.dither_)
        {
          ditherer.process (
            temp_buffer.getWritePointer (0), static_cast<int> (nframes));
          ditherer.process (
            temp_buffer.getWritePointer (1), static_cast<int> (nframes));
        }

      // Copy to output buffer
      const auto output_offset = covered_frames;
      for (int ch = 0; ch < output.getNumChannels (); ++ch)
        {
          if (ch < temp_buffer.getNumChannels ())
            {
              output.copyFrom (
                ch, static_cast<int> (output_offset), temp_buffer, ch, 0,
                static_cast<int> (nframes));
            }
        }

      // Update position and counters
      current_pos += units::samples (latency_preroll_frames > 0 ? 0 : nframes);
      covered_frames += nframes;

      // Update latency preroll frames for next iteration
      if (latency_preroll_frames > 0)
        {
          assert (latency_preroll_frames >= nframes);
          latency_preroll_frames =
            static_cast<nframes_t> (latency_preroll_frames - nframes);
        }

      // Update progress
      promise.setProgressValue (static_cast<int> (covered_frames));
    }

  z_debug ("Rendered range {}", range);

  promise.addResult (output);
}

QFuture<juce::AudioSampleBuffer>
GraphRenderer::render_run_async (
  RenderOptions                options,
  graph::GraphNodeCollection &&nodes,
  SampleRange                  range)
{
  // This is a hack to work around the fact that QtConcurrent::run only supports
  // copyable arguments, and GraphNodeCollection is not copyable
  return QtConcurrent::run (
    [inner_nodes = std::move (nodes)] (
      QPromise<juce::AudioSampleBuffer> &promise,
      GraphRenderer::RenderOptions       inner_options,
      GraphRenderer::SampleRange         inner_range) {
      GraphRenderer::render (
        promise, inner_options,
        std::move (const_cast<graph::GraphNodeCollection &> (inner_nodes)),
        inner_range);
    },
    options, range);
}
}
