// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/region_renderer.h"
#include "structure/tracks/clip_playback_data_provider.h"

namespace zrythm::structure::tracks
{

// Define to enable debug logging for clip launcher event provider
static constexpr bool CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG = false;

ClipPlaybackDataProvider::ClipPlaybackDataProvider (
  const dsp::TempoMap &tempo_map)
    : tempo_map_ (tempo_map)
{
  internal_clip_buffer_position_.store (units::samples (0));
  last_seen_timeline_position_ = units::samples (0);
  clip_launch_timeline_position_ = units::samples (0);
}

void
ClipPlaybackDataProvider::clear_active_buffers ()
{
  decltype (active_midi_playback_sequence_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    rt_events{ active_midi_playback_sequence_ };
  *rt_events = std::nullopt;

  decltype (active_audio_playback_buffer_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    rt_audio{ active_audio_playback_buffer_ };
  *rt_audio = std::nullopt;
}

void
ClipPlaybackDataProvider::generate_midi_events (
  const arrangement::MidiRegion        &midi_region,
  structure::tracks::ClipQuantizeOption quantize_option)
{
  juce::MidiMessageSequence region_seq;

  // Serialize region (timings in ticks)
  arrangement::RegionRenderer::serialize_to_sequence (
    midi_region, region_seq, std::nullopt, std::nullopt, false, true);

  // Convert timings to samples
  for (auto &event : region_seq)
    {
      event->message.setTimeStamp (
        static_cast<double> (
          tempo_map_
            .tick_to_samples_rounded (
              units::ticks (event->message.getTimeStamp ()))
            .in (units::samples)));
    }

  decltype (active_midi_playback_sequence_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    rt_events{ active_midi_playback_sequence_ };
  *rt_events = MidiCache{
    std::move (region_seq), quantize_option,
    units::ticks (midi_region.bounds ()->length ()->ticks ())
  };
}

void
ClipPlaybackDataProvider::generate_audio_events (
  const arrangement::AudioRegion       &audio_region,
  structure::tracks::ClipQuantizeOption quantize_option)
{
  // Serialize region
  juce::AudioSampleBuffer audio_buffer;
  arrangement::RegionRenderer::serialize_to_buffer (audio_region, audio_buffer);

  decltype (active_audio_playback_buffer_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    rt_audio{ active_audio_playback_buffer_ };
  *rt_audio = AudioCache{
    std::move (audio_buffer), quantize_option,
    units::ticks (audio_region.bounds ()->length ()->ticks ())
  };
}

void
ClipPlaybackDataProvider::queue_stop_playback (
  structure::tracks::ClipQuantizeOption quantize_option)
{
  // For now, we'll just clear the events immediately
  // In a full implementation, this would respect the quantization option
  // and stop at the appropriate quantized position
  clear_active_buffers ();
  internal_clip_buffer_position_.store (units::samples (0));
  last_seen_timeline_position_ = units::samples (0);
  clip_launch_timeline_position_ = units::samples (0);
}

void
ClipPlaybackDataProvider::process_midi_events (
  const EngineProcessTimeInfo &time_nfo,
  dsp::MidiEventVector        &output_buffer) noexcept
{
  [&] () {
    decltype (active_midi_playback_sequence_)::ScopedAccess<
      farbot::ThreadType::realtime>
      cache_opt{ active_midi_playback_sequence_ };
    if (!cache_opt->has_value ())
      {
        playing_.store (false);
        return;
      }
    const auto &cache = cache_opt->value ();
    const auto &midi_seq = cache.midi_seq_;
    const auto  clip_loop_end_samples =
      tempo_map_.tick_to_samples_rounded (cache.end_position_);

    if constexpr (CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG)
      z_debug (
        "Clip loop end: {} samples ({} ticks)",
        clip_loop_end_samples.in (units::samples),
        cache.end_position_.in (units::ticks));

    // Update playback position based on timeline changes
    update_playback_position (time_nfo, clip_loop_end_samples);

    // Handle quantization and get start position and samples to process
    auto [internal_buffer_start_offset, samples_to_process] =
      handle_quantization_and_start (
        time_nfo, clip_loop_end_samples, cache.quantize_opt_);

    if (samples_to_process == units::samples (0))
      {
        // Not time to start yet
        return;
      }

    // Process MIDI chunks with looping using the common template method
    process_chunks_with_looping (
      internal_buffer_start_offset, samples_to_process, clip_loop_end_samples,
      units::samples (time_nfo.local_offset_),
      [&] (
        units::sample_t current_internal_buffer_offset,
        units::sample_t samples_to_process_in_chunk,
        units::sample_t total_samples_processed,
        units::sample_t output_buffer_timestamp_offset) {
        if constexpr (CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG)
          {
            // Debug: Print all events in the sequence
            z_debug ("Total events in sequence: {}", midi_seq.getNumEvents ());
            for (int j = 0; j < midi_seq.getNumEvents (); ++j)
              {
                const auto * event = midi_seq.getEventPointer (j);
                if (event->message.isNoteOn ())
                  {
                    z_debug (
                      "Sequence Event {}: note_on={}, timestamp={}", j,
                      event->message.getNoteNumber (),
                      event->message.getTimeStamp ());
                  }
              }
          }

        // Calculate events, offsetting from current position
        const auto first_idx = midi_seq.getNextIndexAtTime (
          static_cast<double> (
            current_internal_buffer_offset.in (units::samples)));
        if constexpr (CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG)
          z_debug (
            "Processing chunk: current_internal_buffer_offset={}, samples_to_process_in_chunk={}, first_idx={}",
            current_internal_buffer_offset.in (units::samples),
            samples_to_process_in_chunk.in (units::samples), first_idx);

        for (int i = first_idx; i < midi_seq.getNumEvents (); ++i)
          {
            const auto * event = midi_seq.getEventPointer (i);
            const auto   timestamp_dbl = event->message.getTimeStamp ();

            if constexpr (CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG)
              {
                // Debug output for each event
                if (event->message.isNoteOn ())
                  {
                    z_debug (
                      "Event {}: note_on={}, timestamp={}", i,
                      event->message.getNoteNumber (), timestamp_dbl);
                  }
              }

            if (
              timestamp_dbl
              >= (current_internal_buffer_offset + samples_to_process_in_chunk)
                   .in<double> (units::samples))
              {
                break;
              }

            // Calculate the timestamp in the output buffer
            // This is the event's timestamp relative to the start of this
            // processing chunk plus the total samples already processed
            const auto event_offset_in_chunk =
              static_cast<unsigned_frame_t> (timestamp_dbl)
              - current_internal_buffer_offset.in<unsigned_frame_t> (
                units::samples);
            const auto local_timestamp =
              total_samples_processed.in<unsigned_frame_t> (units::samples)
              + event_offset_in_chunk
              + output_buffer_timestamp_offset.in<unsigned_frame_t> (
                units::samples);

            output_buffer.add_raw (
              event->message.getRawData (), event->message.getRawDataSize (),
              static_cast<midi_time_t> (local_timestamp));
            if constexpr (CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG)
              z_debug ("Adding event with local_timestamp={}", local_timestamp);
          }
      });
  }();

  // Check for transition from playing to stopped and send all-notes-off
  bool current_playing = playing_.load ();

  if (was_playing_ && !current_playing)
    {
      // We just transitioned from playing to stopped - send all-notes-off
      for (int channel = 1; channel <= 16; ++channel)
        {
          output_buffer.add_all_notes_off (
            channel, time_nfo.local_offset_, true); // All notes off
        }
    }

  // Update was_playing for next iteration
  was_playing_ = current_playing;
}

void
ClipPlaybackDataProvider::update_playback_position (
  const EngineProcessTimeInfo &time_nfo,
  units::sample_t              clip_loop_end_samples)
{
  const auto current_timeline_position =
    units::samples (time_nfo.g_start_frame_w_offset_);
  auto previous_timeline_position = last_seen_timeline_position_;

  // If the timeline position moved (either forward or backward), recalculate
  // our internal buffer position
  if (
    current_timeline_position != previous_timeline_position && playing_.load ())
    {
      if constexpr (CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG)
        z_debug (
          "Detected timeline position change: {} -> {}, recalculating internal buffer position",
          previous_timeline_position.in (units::samples),
          current_timeline_position.in (units::samples));

      // Calculate how long we've been playing since launch
      auto time_since_clip_launch =
        current_timeline_position - clip_launch_timeline_position_;

      // Calculate position within the clip (modulo for looping)
      auto new_internal_buffer_position =
        time_since_clip_launch % clip_loop_end_samples;

      if constexpr (CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG)
        z_debug (
          "Time since clip launch: {}, clip loop end: {}, new internal buffer position: {}",
          time_since_clip_launch.in (units::samples),
          clip_loop_end_samples.in (units::samples),
          new_internal_buffer_position.in (units::samples));

      internal_clip_buffer_position_.store (new_internal_buffer_position);
    }

  // Update the last seen timeline position
  last_seen_timeline_position_ = current_timeline_position;
}

std::pair<units::sample_t, units::sample_t>
ClipPlaybackDataProvider::handle_quantization_and_start (
  const EngineProcessTimeInfo          &time_nfo,
  units::sample_t                       clip_loop_end_samples,
  structure::tracks::ClipQuantizeOption quantize_opt)
{
  const auto current_timeline_position =
    units::samples (time_nfo.g_start_frame_w_offset_);
  const auto timeline_end_position =
    units::samples (time_nfo.g_start_frame_w_offset_ + time_nfo.nframes_);
  const auto current_timeline_tick_position =
    tempo_map_.samples_to_tick (current_timeline_position);
  const auto timeline_end_tick_position =
    tempo_map_.samples_to_tick (timeline_end_position);

  // Position we started playing from in our own buffer
  auto internal_buffer_start_offset = internal_clip_buffer_position_.load ();
  auto samples_to_process = units::samples (time_nfo.nframes_);

  // Offset to apply when writing events to the output buffer
  auto output_buffer_timestamp_offset = units::samples (time_nfo.local_offset_);

  if constexpr (CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG)
    z_debug (
      "Initial state: internal_buffer_start_offset={}, samples_to_process={}, output_buffer_timestamp_offset={}",
      internal_buffer_start_offset.in (units::samples),
      samples_to_process.in (units::samples),
      output_buffer_timestamp_offset.in (units::samples));

  if (!playing_.load ())
    {
      if (quantize_opt == structure::tracks::ClipQuantizeOption::Immediate)
        {
          // For Immediate quantization, start playing immediately
          playing_.store (true);

          clip_launch_timeline_position_ = current_timeline_position;
          internal_buffer_start_offset = units::samples (0);
          // Keep the output_buffer_offset as set from local_offset_
          // Don't adjust samples_to_play
        }
      else
        {
          // For other quantize options, do musical position calculations
          // Check if it's time to play yet based on quantize option
          const auto musical_pos_at_start = tempo_map_.tick_to_musical_position (
            au::round_as<int64_t> (units::ticks, current_timeline_tick_position));
          const auto musical_pos_at_end = tempo_map_.tick_to_musical_position (
            au::round_as<int64_t> (units::ticks, timeline_end_tick_position));

          bool                           should_start = false;
          dsp::TempoMap::MusicalPosition musical_pos_to_start_playing_at{};

          switch (quantize_opt)
            {
            case structure::tracks::ClipQuantizeOption::NextBar:
              // Check if we're at or have crossed a bar boundary
              if (musical_pos_at_end.bar > musical_pos_at_start.bar)
                {
                  // We crossed a bar boundary during this buffer
                  should_start = true;
                  // Start at the beginning of the new bar (beat 1, sixteenth 1)
                  musical_pos_to_start_playing_at = {
                    .bar = musical_pos_at_end.bar,
                    .beat = 1,
                    .sixteenth = 1,
                    .tick = 0
                  };
                }
              else if (
                musical_pos_at_start.bar == musical_pos_at_end.bar
                && musical_pos_at_start.beat == 1
                && musical_pos_at_start.sixteenth == 1
                && musical_pos_at_start.tick == 0)
                {
                  // We're exactly at the start of a bar
                  should_start = true;
                  musical_pos_to_start_playing_at = musical_pos_at_start;
                }
              break;

            case structure::tracks::ClipQuantizeOption::NextBeat:
              // Check if we're at or have crossed a beat boundary
              if (musical_pos_at_end.bar > musical_pos_at_start.bar)
                {
                  // We crossed to a new bar, start at beat 1 of the new bar
                  should_start = true;
                  musical_pos_to_start_playing_at = {
                    .bar = musical_pos_at_end.bar,
                    .beat = 1,
                    .sixteenth = 1,
                    .tick = 0
                  };
                }
              else if (musical_pos_at_end.beat > musical_pos_at_start.beat)
                {
                  // We crossed to a new beat within the same bar
                  should_start = true;
                  musical_pos_to_start_playing_at = {
                    .bar = musical_pos_at_end.bar,
                    .beat = musical_pos_at_end.beat,
                    .sixteenth = 1,
                    .tick = 0
                  };
                }
              else if (
                musical_pos_at_start.bar == musical_pos_at_end.bar
                && musical_pos_at_start.beat == musical_pos_at_end.beat
                && musical_pos_at_start.sixteenth == 1
                && musical_pos_at_start.tick == 0)
                {
                  // We're exactly at the start of a beat
                  should_start = true;
                  musical_pos_to_start_playing_at = musical_pos_at_start;
                }
              break;

            default:
              // For other quantize options, default to immediate for now
              should_start = true;
              musical_pos_to_start_playing_at = musical_pos_at_start;
              break;
            }

          if (!should_start)
            {
              // Not time to start yet
              return { units::samples (0), units::samples (0) };
            }

          playing_.store (true);

          const auto quantized_timeline_position =
            tempo_map_.tick_to_samples_rounded (
              tempo_map_.musical_position_to_tick (
                musical_pos_to_start_playing_at));

          clip_launch_timeline_position_ = quantized_timeline_position;

          // Calculate where to start in our own buffer
          internal_buffer_start_offset = units::samples (0);

          // Calculate the offset to apply when writing to the output buffer
          output_buffer_timestamp_offset =
            quantized_timeline_position - current_timeline_position;
          if constexpr (CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG)
            z_debug (
              "Calculated output_buffer_timestamp_offset: {} samples",
              output_buffer_timestamp_offset.in (units::samples));

          // Adjust how many samples we need to play
          samples_to_process =
            samples_to_process - output_buffer_timestamp_offset;
          if constexpr (CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG)
            z_debug (
              "Adjusted samples_to_process: {} samples",
              samples_to_process.in (units::samples));
        }
    }

  return { internal_buffer_start_offset, samples_to_process };
}

template <typename ProcessFunc>
void
ClipPlaybackDataProvider::process_chunks_with_looping (
  units::sample_t internal_buffer_start_offset,
  units::sample_t samples_to_process,
  units::sample_t clip_loop_end_samples,
  units::sample_t output_buffer_timestamp_offset,
  ProcessFunc     process_chunk)
{
  // Process events with looping
  auto current_internal_buffer_offset = internal_buffer_start_offset;
  auto remaining_samples_to_process = samples_to_process;

  // Track the total samples processed for output buffer timestamp calculation
  auto total_samples_processed = units::samples (0);

  while (remaining_samples_to_process > units::samples (0))
    {
      // Calculate how much we can process before hitting the loop point or end
      // of buffer
      auto samples_until_loop =
        clip_loop_end_samples - current_internal_buffer_offset;
      auto samples_to_process_in_chunk = remaining_samples_to_process;

      if (
        samples_until_loop > units::samples (0)
        && samples_until_loop < remaining_samples_to_process)
        {
          samples_to_process_in_chunk = samples_until_loop;
        }

      // Process the chunk using the provided function
      process_chunk (
        current_internal_buffer_offset, samples_to_process_in_chunk,
        total_samples_processed, output_buffer_timestamp_offset);

      // Update position and remaining samples
      current_internal_buffer_offset += samples_to_process_in_chunk;
      total_samples_processed += samples_to_process_in_chunk;
      remaining_samples_to_process -= samples_to_process_in_chunk;

      // Check if we need to loop
      if (remaining_samples_to_process > units::samples (0))
        {
          // Reset to beginning of loop
          current_internal_buffer_offset = units::samples (0);
          if constexpr (CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG)
            z_debug ("Looping: resetting current_internal_buffer_offset to 0");
        }
    }

  internal_clip_buffer_position_ = current_internal_buffer_offset;
}

void
ClipPlaybackDataProvider::process_audio_events (
  const EngineProcessTimeInfo &time_nfo,
  std::span<float>             left_buffer,
  std::span<float>             right_buffer) noexcept
{
  [&] () {
    decltype (active_audio_playback_buffer_)::ScopedAccess<
      farbot::ThreadType::realtime>
      cache_opt{ active_audio_playback_buffer_ };
    if (!cache_opt->has_value ())
      {
        playing_.store (false);
        return;
      }

    const auto &cache = cache_opt->value ();
    const auto &audio_buffer = cache.audio_buffer_;
    if (audio_buffer.getNumSamples () == 0)
      {
        return;
      }

    const auto clip_loop_end_samples =
      tempo_map_.tick_to_samples_rounded (cache.end_position_);

    if constexpr (CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG)
      z_debug (
        "Audio clip loop end: {} samples ({} ticks)",
        clip_loop_end_samples.in (units::samples),
        cache.end_position_.in (units::ticks));

    // Update playback position based on timeline changes
    update_playback_position (time_nfo, clip_loop_end_samples);

    // Handle quantization and get start position and samples to process
    auto [internal_buffer_start_offset, samples_to_process] =
      handle_quantization_and_start (
        time_nfo, clip_loop_end_samples, cache.quantize_opt_);

    if (samples_to_process == units::samples (0))
      {
        // Not time to start yet
        return;
      }

    // Process audio chunks with looping using the common template method
    process_chunks_with_looping (
      internal_buffer_start_offset, samples_to_process, clip_loop_end_samples,
      units::samples (time_nfo.local_offset_),
      [&] (
        units::sample_t current_internal_buffer_offset,
        units::sample_t samples_to_process_in_chunk,
        units::sample_t total_samples_processed,
        units::sample_t output_buffer_timestamp_offset) {
        // Calculate the number of samples to copy
        const auto samples_to_copy = std::min (
          samples_to_process_in_chunk.as<int64_t> (units::samples),
          units::samples (audio_buffer.getNumSamples ())
            - current_internal_buffer_offset.as<int64_t> (units::samples));

        if (samples_to_copy > units::samples (0))
          {
            // Copy audio data to output buffers
            const auto output_start_idx =
              total_samples_processed + output_buffer_timestamp_offset;

            const auto output_start_idx_samples =
              output_start_idx.in<size_t> (units::samples);
            if (output_start_idx_samples < left_buffer.size ())
              {
                const auto actual_samples_to_copy = std::min (
                  samples_to_copy,
                  static_cast<units::sample_t> (units::samples (
                    left_buffer.size () - output_start_idx_samples)));

                // Copy left channel
                utils::float_ranges::add2 (
                  &left_buffer[output_start_idx_samples],
                  audio_buffer.getReadPointer (
                    0, current_internal_buffer_offset.in<int> (units::samples)),
                  actual_samples_to_copy.in (units::samples));

                // Copy right channel
                if (
                  audio_buffer.getNumChannels () > 1
                  && right_buffer.size () > output_start_idx_samples)
                  {
                    utils::float_ranges::add2 (
                      &right_buffer[output_start_idx_samples],
                      audio_buffer.getReadPointer (
                        1,
                        current_internal_buffer_offset.in<int> (units::samples)),
                      actual_samples_to_copy.in (units::samples));
                  }
              }
          }
      });
  }();

  // Update was_playing for next iteration
  was_playing_ = playing_.load ();
}
}
