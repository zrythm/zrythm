// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/timeline_data_provider.h"

namespace zrythm::structure::arrangement
{

// ========== MidiTimelineDataProvider Implementation ==========

void
MidiTimelineDataProvider::set_midi_events (
  const juce::MidiMessageSequence &events)
{
  decltype (active_midi_playback_sequence_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    rt_events{ active_midi_playback_sequence_ };
  *rt_events = events;
}

void
MidiTimelineDataProvider::clear_all_caches ()
{
  midi_cache_.clear ();
  decltype (active_midi_playback_sequence_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    rt_events{ active_midi_playback_sequence_ };
  rt_events->clear ();
}

void
MidiTimelineDataProvider::remove_sequences_matching_interval_from_all_caches (
  IntervalType interval)
{
  midi_cache_.remove_sequences_matching_interval (interval);
}

const juce::MidiMessageSequence &
MidiTimelineDataProvider::get_midi_events () const
{
  return midi_cache_.get_midi_events ();
}

void
MidiTimelineDataProvider::process_midi_events (
  const EngineProcessTimeInfo &time_nfo,
  dsp::ITransport::PlayState   transport_state,
  dsp::MidiEventVector        &output_buffer) noexcept
{
  const bool transport_rolling =
    transport_state == dsp::ITransport::PlayState::Rolling;
  const auto current_transport_position =
    units::samples (static_cast<int64_t> (time_nfo.g_start_frame_w_offset_));

  // Check for transport position jump (seek, loop, etc.)
  const bool transport_position_jumped =
    (next_expected_transport_position_ != units::samples (0))
    && (current_transport_position != next_expected_transport_position_);

  // Check for transport state change (rolling -> stopped)
  const bool transport_stopped_rolling =
    last_seen_transport_state_ == dsp::ITransport::PlayState::Rolling
    && !transport_rolling;

  // Send all-notes-off if transport stopped or position jumped
  if (
    transport_stopped_rolling
    || (transport_rolling && transport_position_jumped))
    {
      for (int channel = 1; channel <= 16; ++channel)
        {
          output_buffer.add_all_notes_off (
            channel, time_nfo.local_offset_, true); // All notes off
        }
    }

  // Only process MIDI events if transport is rolling
  if (transport_rolling)
    {
      decltype (active_midi_playback_sequence_)::ScopedAccess<
        farbot::ThreadType::realtime>
                 midi_seq{ active_midi_playback_sequence_ };
      const auto first_idx = midi_seq->getNextIndexAtTime (
        static_cast<double> (time_nfo.g_start_frame_w_offset_));
      for (int i = first_idx; i < midi_seq->getNumEvents (); ++i)
        {
          const auto * event = midi_seq->getEventPointer (i);
          const auto   timestamp_dbl = event->message.getTimeStamp ();
          if (
            timestamp_dbl >= static_cast<double> (
              time_nfo.g_start_frame_w_offset_ + time_nfo.nframes_))
            {
              break;
            }

          const auto local_timestamp =
            static_cast<decltype (time_nfo.g_start_frame_)> (timestamp_dbl)
            - time_nfo.g_start_frame_;
          output_buffer.add_raw (
            event->message.getRawData (), event->message.getRawDataSize (),
            static_cast<midi_time_t> (local_timestamp));
        }
    }

  // Update tracking for next time
  next_expected_transport_position_ =
    current_transport_position + units::samples (time_nfo.nframes_);
  last_seen_transport_state_ = transport_state;
}

// ========== AudioTimelineDataProvider Implementation ==========

void
AudioTimelineDataProvider::set_audio_regions (
  const std::vector<dsp::AudioTimelineDataCache::AudioRegionEntry> &regions)
{
  decltype (active_audio_regions_)::ScopedAccess<farbot::ThreadType::nonRealtime>
    rt_regions{ active_audio_regions_ };
  *rt_regions = regions;
}

void
AudioTimelineDataProvider::clear_all_caches ()
{
  audio_cache_.clear ();
  decltype (active_audio_regions_)::ScopedAccess<farbot::ThreadType::nonRealtime>
    rt_regions{ active_audio_regions_ };
  rt_regions->clear ();
}

void
AudioTimelineDataProvider::remove_sequences_matching_interval_from_all_caches (
  IntervalType interval)
{
  audio_cache_.remove_sequences_matching_interval (interval);
}

const std::vector<dsp::AudioTimelineDataCache::AudioRegionEntry> &
AudioTimelineDataProvider::get_audio_regions () const
{
  return audio_cache_.get_audio_regions ();
}

void
AudioTimelineDataProvider::process_audio_events (
  const EngineProcessTimeInfo &time_nfo,
  dsp::ITransport::PlayState   transport_state,
  std::span<float>             output_left,
  std::span<float>             output_right) noexcept
{
  // Set to true to enable debug logging for timeline data provider
  static constexpr bool TIMELINE_DATA_PROVIDER_DEBUG = false;

  const bool transport_rolling =
    transport_state == dsp::ITransport::PlayState::Rolling;
  const auto current_transport_position =
    units::samples (static_cast<int64_t> (time_nfo.g_start_frame_w_offset_));

  // Only process audio events if transport is rolling
  if (!transport_rolling)
    {
      // Update tracking for next time
      next_expected_transport_position_ =
        current_transport_position + units::samples (time_nfo.nframes_);
      return;
    }

  decltype (active_audio_regions_)::ScopedAccess<farbot::ThreadType::realtime>
    audio_regions{ active_audio_regions_ };

  const auto start_frame =
    units::samples (static_cast<int64_t> (time_nfo.g_start_frame_w_offset_));
  const auto end_frame =
    start_frame + units::samples (static_cast<int64_t> (time_nfo.nframes_));

  if constexpr (TIMELINE_DATA_PROVIDER_DEBUG)
    {
      z_debug (
        "process_audio_events: start_frame={}, end_frame={}, nframes_={}",
        start_frame, end_frame, time_nfo.nframes_);
      z_debug ("Processing {} audio regions", audio_regions->size ());
    }

  // Process each audio region that overlaps with the current time range
  for (const auto &region : *audio_regions)
    {
      if constexpr (TIMELINE_DATA_PROVIDER_DEBUG)
        {
          z_debug (
            "Region: start_sample={}, end_sample={}, buffer_size={}",
            region.start_sample, region.end_sample,
            region.audio_buffer.getNumSamples ());
        }

      // Check if region overlaps with current time range
      if (region.end_sample <= start_frame || region.start_sample >= end_frame)
        {
          if constexpr (TIMELINE_DATA_PROVIDER_DEBUG)
            z_debug ("Region does not overlap with time range, skipping");
          continue;
        }

      // Calculate the overlap range
      const auto overlap_start = std::max (region.start_sample, start_frame);
      const auto overlap_end = std::min (region.end_sample, end_frame);
      const auto overlap_length = overlap_end - overlap_start;

      if constexpr (TIMELINE_DATA_PROVIDER_DEBUG)
        {
          z_debug (
            "Overlap: overlap_start={}, overlap_end={}, overlap_length={}",
            overlap_start, overlap_end, overlap_length);
        }

      if (overlap_length <= units::samples (0))
        {
          if constexpr (TIMELINE_DATA_PROVIDER_DEBUG)
            z_debug ("Non-positive overlap length, skipping");
          continue;
        }

      // Calculate the offset into the audio buffer
      const auto buffer_offset = overlap_start - region.start_sample;
      const auto output_offset = overlap_start - start_frame;

      if constexpr (TIMELINE_DATA_PROVIDER_DEBUG)
        {
          z_debug (
            "Offsets: buffer_offset={}, output_offset={}", buffer_offset,
            output_offset);
        }

      // Get the audio buffer from the region
      const auto &audio_buffer = region.audio_buffer;
      if (audio_buffer.getNumSamples () == 0)
        {
          if constexpr (TIMELINE_DATA_PROVIDER_DEBUG)
            z_debug ("Empty audio buffer, skipping");
          continue;
        }

      // Mix the audio into the output buffers
      const auto num_channels =
        std::min (audio_buffer.getNumChannels (), 2); // Max 2 channels
      const auto buffer_samples =
        units::samples (static_cast<int64_t> (audio_buffer.getNumSamples ()));

      if constexpr (TIMELINE_DATA_PROVIDER_DEBUG)
        {
          z_debug (
            "Buffer info: num_channels={}, buffer_samples={}", num_channels,
            buffer_samples);
        }

      for (int channel = 0; channel < num_channels; ++channel)
        {
          const auto * channel_data = audio_buffer.getReadPointer (channel);
          auto &output_data = (channel == 0) ? output_left : output_right;

          // Copy samples with bounds checking
          // Calculate the actual number of samples we can process
          // Ensure we don't exceed either the input buffer or output buffer
          // bounds
          const auto input_buffer_limit =
            au::max (buffer_samples - buffer_offset, units::samples (0));
          const auto output_buffer_limit = au::max (
            units::samples (time_nfo.nframes_) - output_offset,
            units::samples (0));

          // Also ensure we don't exceed the actual span size
          const auto actual_output_span_size =
            units::samples (static_cast<int64_t> (output_data.size ()));
          const auto span_based_limit = au::max (
            actual_output_span_size - output_offset, units::samples (0));

          const auto actual_overlap_length = std::min (
            { overlap_length, input_buffer_limit, output_buffer_limit,
              span_based_limit });

          if (actual_overlap_length <= units::samples (0))
            {
              if constexpr (TIMELINE_DATA_PROVIDER_DEBUG)
                z_debug ("Non-positive actual overlap length, skipping");
              continue;
            }

          if (TIMELINE_DATA_PROVIDER_DEBUG)
            {
              z_debug (
                "Adjusted overlap length: {} -> {} (buffer_offset={}, buffer_samples={}, output_offset={}, nframes_={}, span_size={})",
                overlap_length, actual_overlap_length, buffer_offset,
                buffer_samples, output_offset, time_nfo.nframes_,
                actual_output_span_size);
              z_debug (
                "Limits: input_buffer_limit={}, output_buffer_limit={}, span_based_limit={}",
                input_buffer_limit, output_buffer_limit, span_based_limit);

              // Check if output access would exceed buffer bounds
              if (
                output_offset + actual_overlap_length
                > units::samples (static_cast<int64_t> (time_nfo.nframes_)))
                {
                  z_debug (
                    "WARNING: Output access would exceed buffer bounds! {} + {} > {}",
                    output_offset, actual_overlap_length, time_nfo.nframes_);
                }

              if (
                output_offset + actual_overlap_length > actual_output_span_size)
                {
                  z_debug (
                    "WARNING: Output access would exceed span bounds! {} + {} > {}",
                    output_offset, actual_overlap_length,
                    actual_output_span_size);
                }
            }

          for (
            const auto i :
            std::views::iota (0, actual_overlap_length.in (units::samples)))
            {
              const auto i_samples = units::samples (i);
              const auto buffer_idx = buffer_offset + i_samples;
              const auto output_idx = output_offset + i_samples;

              if (TIMELINE_DATA_PROVIDER_DEBUG && i < 5)
                {
                  z_debug (
                    "Sample {}: buffer_idx={}, output_idx={}, buffer_samples={}, nframes_={}",
                    i, buffer_idx, output_idx, buffer_samples,
                    time_nfo.nframes_);
                }

              // Ensure we're within both buffer bounds
              if (
                buffer_idx >= units::samples (0) && buffer_idx < buffer_samples
                && output_idx >= units::samples (0)
                && output_idx < units::samples (
                     static_cast<int64_t> (time_nfo.nframes_)))
                {
                  output_data[output_idx.in (units::samples)] +=
                    channel_data[buffer_idx.in (units::samples)];
                }
              else
                {
                  if (TIMELINE_DATA_PROVIDER_DEBUG && i < 5)
                    {
                      z_debug (
                        "Skipping sample {}: buffer_idx={}, output_idx={}, buffer_samples={}, nframes_={}",
                        i, buffer_idx, output_idx, buffer_samples,
                        time_nfo.nframes_);
                    }
                }
            }
        }
    }

  // Update tracking for next time
  next_expected_transport_position_ =
    current_transport_position + units::samples (time_nfo.nframes_);
}

// ========== AutomationTimelineDataProvider Implementation ==========

void
AutomationTimelineDataProvider::set_automation_sequences (
  const std::vector<dsp::AutomationTimelineDataCache::AutomationCacheEntry>
    &sequences)
{
  decltype (active_automation_sequences_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    rt_sequences{ active_automation_sequences_ };
  *rt_sequences = sequences;
}

void
AutomationTimelineDataProvider::clear_all_caches ()
{
  automation_cache_.clear ();
  decltype (active_automation_sequences_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    rt_sequences{ active_automation_sequences_ };
  rt_sequences->clear ();
}

void
AutomationTimelineDataProvider::
  remove_sequences_matching_interval_from_all_caches (IntervalType interval)
{
  automation_cache_.remove_sequences_matching_interval (interval);
}

const std::vector<dsp::AutomationTimelineDataCache::AutomationCacheEntry> &
AutomationTimelineDataProvider::get_automation_sequences () const
{
  return automation_cache_.get_automation_sequences ();
}

std::optional<float>
AutomationTimelineDataProvider::get_automation_value_rt (
  units::sample_t sample_position) noexcept
{
  decltype (active_automation_sequences_)::ScopedAccess<
    farbot::ThreadType::realtime>
    sequences{ active_automation_sequences_ };

  std::optional<float> last_known_value;

  // Find the automation sequence that contains the sample position
  // or the last sequence before this position
  for (const auto &sequence : *sequences)
    {
      // If the sample position is within this sequence, return the value
      if (
        sample_position >= sequence.start_sample
        && sample_position < sequence.end_sample)
        {
          // Calculate the offset into the automation values array
          const auto offset = sample_position - sequence.start_sample;
          const auto idx = static_cast<size_t> (offset.in (units::samples));

          // Ensure we're within bounds
          if (idx < sequence.automation_values.size ())
            {
              return sequence.automation_values[idx];
            }
        }

      // If this sequence ends before the sample position,
      // update the last known value from the end of this sequence
      if (
        sequence.end_sample <= sample_position
        && !sequence.automation_values.empty ())
        {
          last_known_value = sequence.automation_values.back ();
        }
    }

  // Return the last known value if no automation found at this position
  return last_known_value;
}

void
AutomationTimelineDataProvider::process_automation_events (
  const EngineProcessTimeInfo &time_nfo,
  dsp::ITransport::PlayState   transport_state,
  std::span<float>             output_values) noexcept
{
  const bool transport_rolling =
    transport_state == dsp::ITransport::PlayState::Rolling;

  // Only process automation events if transport is rolling
  if (!transport_rolling)
    {
      return;
    }

  const auto start_frame =
    units::samples (static_cast<int64_t> (time_nfo.g_start_frame_w_offset_));
  const auto nframes = units::samples (static_cast<int64_t> (time_nfo.nframes_));

  // Generate automation values for each sample in the time range
  for (const auto i : std::views::iota (0, nframes.in (units::samples)))
    {
      const auto sample_position = start_frame + units::samples (i);
      const auto automation_value = get_automation_value_rt (sample_position);
      output_values[i] = automation_value.value_or (-1.f);
    }
}

} // namespace zrythm::structure::arrangement
