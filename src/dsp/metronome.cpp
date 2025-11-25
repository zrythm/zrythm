// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "dsp/metronome.h"

namespace zrythm::dsp
{
Metronome::Metronome (
  ProcessorBaseDependencies dependencies,
  const dsp::TempoMap      &tempo_map,
  juce::AudioSampleBuffer   emphasis_sample,
  juce::AudioSampleBuffer   normal_sample,
  bool                      initially_enabled,
  float                     initial_volume,
  QObject *                 parent)
    : QObject (parent), dsp::AudioSampleProcessor (dependencies),
      tempo_map_ (tempo_map),
      emphasis_sample_buffer_ (std::move (emphasis_sample)),
      normal_sample_buffer_ (std::move (normal_sample)),
      volume_ (initial_volume), enabled_ (initially_enabled)
{
  set_name (u8"Metronome");
}

void
Metronome::find_and_queue_metronome_samples (
  const units::sample_t start_pos,
  const units::sample_t end_pos,
  const units::sample_t loffset)
{
  /* special case */
  if (start_pos == end_pos) [[unlikely]]
    return;

  const auto frame_to_musical_position = [this] (const units::sample_t frame) {
    return tempo_map_.samples_to_musical_position (frame);
  };

  if (
    frame_to_musical_position (start_pos)
    == frame_to_musical_position (end_pos - units::samples (1))) [[likely]]
    {
      // avoid expensive computations below if musical position doesn't change
      // from start to end
      return;
    }

  for (
    const auto cur_position_in_frames : std::views::iota (
      start_pos.in (units::samples), end_pos.in (units::samples)))
    {
      const auto current_cycle_offset =
        loffset + (units::samples (cur_position_in_frames) - start_pos);
      const auto musical_pos =
        frame_to_musical_position (units::samples (cur_position_in_frames));
      if (cur_position_in_frames == 0) [[unlikely]]
        {
          queue_metronome (true, current_cycle_offset);
        }
      else
        {
          const auto prev_musical_pos = frame_to_musical_position (
            units::samples (cur_position_in_frames) - units::samples (1));
          if (prev_musical_pos.bar != musical_pos.bar)
            {
              queue_metronome (true, current_cycle_offset);
            }
          else if (prev_musical_pos.beat != musical_pos.beat)
            {
              queue_metronome (false, current_cycle_offset);
            }
        }
    }
}

void
Metronome::custom_process_block (
  EngineProcessTimeInfo  time_nfo,
  const dsp::ITransport &transport) noexcept
{
  if (!enabled_.load ())
    {
      // Still need to call parent to clear the output buffer
      AudioSampleProcessor::custom_process_block (time_nfo, transport);
      return;
    }

  // find and queue normal metronome events
  if (
    transport.get_play_state () == dsp::ITransport::PlayState::Rolling
    && transport.metronome_countin_frames_remaining () == units::samples (0))
    {
      const auto loffset = units::samples (time_nfo.local_offset_);
      const auto nframes = units::samples (time_nfo.nframes_);
      const auto playhead_before =
        units::samples (time_nfo.g_start_frame_w_offset_);
      const auto playhead_after_ignoring_loops = playhead_before + nframes;
      const auto playhead_after_taking_loops_into_account =
        transport.get_playhead_position_after_adding_frames_in_audio_thread (
          playhead_before, nframes);
      const bool loop_crossed =
        playhead_after_ignoring_loops
        != playhead_after_taking_loops_into_account;
      if (loop_crossed)
        {
          const auto loop_points = transport.get_loop_range_positions ();
          const auto loop_start = loop_points.first;
          const auto loop_end = loop_points.second;
          /* find each bar / beat change until loop end */
          find_and_queue_metronome_samples (playhead_before, loop_end, loffset);

          /* find each bar / beat change after loop start */
          find_and_queue_metronome_samples (
            loop_start, playhead_after_taking_loops_into_account,
            loffset + loop_points.second - playhead_before);
        }
      else /* loop not crossed */
        {
          /* find each bar / beat change from start to finish */
          find_and_queue_metronome_samples (
            playhead_before, playhead_after_taking_loops_into_account, loffset);
        }
    }
  // find and queue events for countin
  else if (transport.metronome_countin_frames_remaining () > units::samples (0))
    {
      queue_metronome_countin (time_nfo, transport);
    }

  AudioSampleProcessor::custom_process_block (time_nfo, transport);
}

void
Metronome::custom_prepare_for_processing (
  const graph::GraphNode * node,
  sample_rate_t            sample_rate,
  nframes_t                max_block_length)
{
  AudioSampleProcessor::custom_prepare_for_processing (
    node, sample_rate, max_block_length);
}

void
Metronome::queue_metronome (
  bool                 emphasis,
  units::sample_t      offset,
  std::source_location loc)
{
  juce::AudioSampleBuffer &buffer_to_use =
    emphasis ? emphasis_sample_buffer_ : normal_sample_buffer_;

  const auto num_channels = buffer_to_use.getNumChannels ();
  const auto volume = volume_.load ();
  if (num_channels == 1) // convert to stereo
    {
      add_sample_to_process (
        dsp::AudioSampleProcessor::PlayableSampleSingleChannel (
          std::span (
            buffer_to_use.getReadPointer (0), buffer_to_use.getNumSamples ()),
          0, volume, offset.in (units::samples), loc));
      add_sample_to_process (
        dsp::AudioSampleProcessor::PlayableSampleSingleChannel (
          std::span (
            buffer_to_use.getReadPointer (0), buffer_to_use.getNumSamples ()),
          1, volume, offset.in (units::samples), loc));
    }
  else
    {
      for (const auto channel_index : std::views::iota (0, num_channels))
        {
          add_sample_to_process (
            dsp::AudioSampleProcessor::PlayableSampleSingleChannel (
              std::span (
                buffer_to_use.getReadPointer (channel_index),
                buffer_to_use.getNumSamples ()),
              channel_index, volume, offset.in (units::samples), loc));
        }
    }
}

void
Metronome::queue_metronome_countin (
  const EngineProcessTimeInfo &time_nfo,
  const dsp::ITransport       &transport)
{
  const auto countin_frames_remaining =
    transport.metronome_countin_frames_remaining ();

  const auto frame_to_tick = [this] (const auto frame) {
    return static_cast<signed_frame_t> (std::round (
      tempo_map_.samples_to_tick (units::samples (static_cast<double> (frame)))
        .in (units::tick)));
  };
  const auto tick_to_frame = [this] (const units::tick_t tick) {
    const units::sample_t s = tempo_map_.tick_to_samples_rounded (
      static_cast<units::precise_tick_t> (tick));
    return static_cast<signed_frame_t> (s.in (units::samples));
  };

  signed_frame_t frames_per_beat{};
  signed_frame_t frames_per_bar{};
  {
    const auto current_musical_pos = tempo_map_.tick_to_musical_position (
      units::ticks (frame_to_tick (time_nfo.g_start_frame_)));
    const auto tick_at_bar_start = tempo_map_.musical_position_to_tick (
      TempoMap::MusicalPosition{
        .bar = current_musical_pos.bar, .beat = 1, .sixteenth = 1, .tick = 0 });
    const auto frame_at_bar_start = tick_to_frame (tick_at_bar_start);
    const auto tick_at_bar_end = tempo_map_.musical_position_to_tick (
      TempoMap::MusicalPosition{
        .bar = current_musical_pos.bar + 1, .beat = 1, .sixteenth = 1, .tick = 0 });
    const auto frame_at_bar_end = tick_to_frame (tick_at_bar_end);
    frames_per_bar = frame_at_bar_end - frame_at_bar_start;
    const auto tick_at_beat_start = tempo_map_.musical_position_to_tick (
      TempoMap::MusicalPosition{
        .bar = current_musical_pos.bar, .beat = 1, .sixteenth = 1, .tick = 0 });
    const auto frame_at_beat_start = tick_to_frame (tick_at_beat_start);
    const auto tick_at_beat_end = tempo_map_.musical_position_to_tick (
      TempoMap::MusicalPosition{
        .bar = current_musical_pos.bar, .beat = 2, .sixteenth = 1, .tick = 0 });
    const auto frame_at_beat_end = tick_to_frame (tick_at_beat_end);
    frames_per_beat = frame_at_beat_end - frame_at_beat_start;
  }

  const auto full_block_size = time_nfo.local_offset_ + time_nfo.nframes_;

  // used to avoid appending beat events where we already have bar events
  boost::container::static_vector<signed_frame_t, 16> queued_events;

  const auto queue_events =
    [this, &time_nfo, countin_frames_remaining, full_block_size,
     &queued_events] (const auto &frames_per_unit, bool emphasis) {
      const auto offset0 = -countin_frames_remaining.in (
        units::samples); // time of first sample in block

      for (int k = 1;; ++k)
        {
          const auto barTime = -k * frames_per_unit; // always negative
          if (barTime < offset0 - full_block_size)
            break; // gone too far left
          if (barTime < offset0)
            continue; // already passed

          const auto idx = barTime - offset0; // index inside block
          if (idx < full_block_size)
            {
              // render click at out[idx]
              if (
                idx >= time_nfo.local_offset_
                && idx < (time_nfo.local_offset_ + time_nfo.nframes_)
                && !std::ranges::contains (queued_events, idx))
                {
                  queue_metronome (emphasis, units::samples (idx));
                  queued_events.push_back (idx);
                }
            }
        }
    };

  // bars & beats
  queue_events (frames_per_bar, true);
  queue_events (frames_per_beat, false);
}
}
