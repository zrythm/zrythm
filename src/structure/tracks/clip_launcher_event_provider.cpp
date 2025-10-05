// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/midi_region_serializer.h"
#include "structure/tracks/clip_launcher_event_provider.h"

namespace zrythm::structure::tracks
{

// Define to enable debug logging for clip launcher event provider
static constexpr bool CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG = false;

ClipLauncherMidiEventProvider::ClipLauncherMidiEventProvider (
  const dsp::TempoMap &tempo_map)
    : tempo_map_ (tempo_map)
{
  internal_clip_buffer_position_.store (units::samples (0));
  last_seen_timeline_position_ = units::samples (0);
  clip_launch_timeline_position_ = units::samples (0);
}

void
ClipLauncherMidiEventProvider::clear_events ()
{
  decltype (active_midi_playback_sequence_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    rt_events{ active_midi_playback_sequence_ };
  *rt_events = std::nullopt;
}

void
ClipLauncherMidiEventProvider::generate_events (
  const arrangement::MidiRegion        &midi_region,
  structure::tracks::ClipQuantizeOption quantize_option)
{
  juce::MidiMessageSequence region_seq;

  // Serialize region (timings in ticks)
  arrangement::MidiRegionSerializer::serialize_to_sequence (
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
  *rt_events = Cache{
    std::move (region_seq), quantize_option,
    units::ticks (midi_region.bounds ()->length ()->ticks ())
  };
}

void
ClipLauncherMidiEventProvider::queue_stop_playback (
  structure::tracks::ClipQuantizeOption quantize_option)
{
  // For now, we'll just clear the events immediately
  // In a full implementation, this would respect the quantization option
  // and stop at the appropriate quantized position
  clear_events ();
  playing_.store (false);
  internal_clip_buffer_position_.store (units::samples (0));
  last_seen_timeline_position_ = units::samples (0);
  clip_launch_timeline_position_ = units::samples (0);
}

void
ClipLauncherMidiEventProvider::process_events (
  const EngineProcessTimeInfo &time_nfo,
  dsp::MidiEventVector        &output_buffer) noexcept
{
  decltype (active_midi_playback_sequence_)::ScopedAccess<
    farbot::ThreadType::realtime>
    cache_opt{ active_midi_playback_sequence_ };
  if (!cache_opt->has_value ())
    {
      return;
    }
  const auto &cache = cache_opt->value ();
  const auto &midi_seq = cache.midi_seq_;
  // const auto& quantize_opt = active_pair.second;
  const auto clip_loop_end_samples =
    tempo_map_.tick_to_samples_rounded (cache.end_position_);
  if constexpr (CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG)
    z_debug (
      "Clip loop end: {} samples ({} ticks)",
      clip_loop_end_samples.in (units::samples),
      cache.end_position_.in (units::ticks));

  const auto current_timeline_position =
    units::samples (time_nfo.g_start_frame_w_offset_);
  const auto timeline_end_position =
    units::samples (time_nfo.g_start_frame_w_offset_ + time_nfo.nframes_);
  const auto current_timeline_tick_position =
    tempo_map_.samples_to_tick (current_timeline_position);
  const auto timeline_end_tick_position =
    tempo_map_.samples_to_tick (timeline_end_position);

  // Check for transport position changes
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
      if (
        cache.quantize_opt_ == structure::tracks::ClipQuantizeOption::Immediate)
        {
          // For Immediate quantization, start playing immediately
          // without any musical position calculations
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

          switch (cache.quantize_opt_)
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
              return;
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
        static_cast<double> (current_internal_buffer_offset.in (units::samples)));
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

      // Update position and remaining samples
      current_internal_buffer_offset += samples_to_process_in_chunk;
      total_samples_processed += samples_to_process_in_chunk;
      remaining_samples_to_process -= samples_to_process_in_chunk;

      if constexpr (CLIP_LAUNCHER_EVENT_PROVIDER_DEBUG)
        z_debug (
          "After processing chunk: current_internal_buffer_offset={}, remaining_samples_to_process={}",
          current_internal_buffer_offset.in (units::samples),
          remaining_samples_to_process.in (units::samples));

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

}
