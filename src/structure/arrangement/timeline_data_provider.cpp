// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/timeline_data_provider.h"

namespace zrythm::structure::arrangement
{

// ========== MidiTimelineDataProvider Implementation ==========

void
MidiTimelineDataProvider::set_midi_events (
  std::span<const dsp::SampleBasedMidiEvent> events)
{
  decltype (active_midi_playback_sequence_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    rt_events{ active_midi_playback_sequence_ };
  rt_events->assign (events.begin (), events.end ());
}

void
MidiTimelineDataProvider::clear_all_caches ()
{
  midi_cache_->clear ();
  decltype (active_midi_playback_sequence_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    rt_events{ active_midi_playback_sequence_ };
  rt_events->clear ();
}

void
MidiTimelineDataProvider::remove_sequences_matching_interval_from_all_caches (
  IntervalType interval)
{
  midi_cache_->remove_sequences_matching_interval (interval);
}

std::span<const dsp::SampleBasedMidiEvent>
MidiTimelineDataProvider::midi_events () const
{
  return midi_cache_->midi_events ();
}

void
MidiTimelineDataProvider::process_midi_events (
  const dsp::graph::ProcessBlockInfo &time_nfo,
  dsp::ITransport::PlayState          transport_state,
  dsp::MidiEventBuffer               &output_buffer) noexcept
{
  const bool transport_rolling =
    transport_state == dsp::ITransport::PlayState::Rolling;
  const auto current_transport_position = time_nfo.transport_position_;

  // Check for transport position jump (seek, loop, etc.)
  const bool transport_position_jumped =
    (next_expected_transport_position_ != units::samples (0))
    && (current_transport_position != next_expected_transport_position_);

  // Check for transport state change (rolling -> stopped)
  const bool transport_stopped_rolling =
    last_seen_transport_state_ == dsp::ITransport::PlayState::Rolling
    && !transport_rolling;

  // Send all-notes-off if transport stopped or position jumped
  // TODO: keep track of current note-ons and send note offs instead
  if (
    transport_stopped_rolling
    || (transport_rolling && transport_position_jumped))
    {
      for (const auto channel : std::views::iota (0, 16))
        {
          const auto ev = dsp::midi_event::make_all_notes_off (
            static_cast<midi_byte_t> (channel), time_nfo.buffer_offset_);
          output_buffer.push_back (ev.time_, ev.data ());
        }
    }

  // Only process MIDI events if transport is rolling
  if (transport_rolling)
    {
      decltype (active_midi_playback_sequence_)::ScopedAccess<
        farbot::ThreadType::realtime>
           midi_events_rt{ active_midi_playback_sequence_ };
      auto it = std::ranges::lower_bound (
        *midi_events_rt, current_transport_position, {},
        &dsp::SampleBasedMidiEvent::time_);
      for (; it != midi_events_rt->end (); ++it)
        {
          if (it->time_ >= time_nfo.end_position ())
            break;

          const auto local_timestamp =
            time_nfo.buffer_offset_ + (it->time_ - current_transport_position);
          const auto ts =
            au::floor_as<uint32_t> (units::samples, local_timestamp);
          output_buffer.push_back (ts, it->data ());
        }
    }

  // Update tracking for next time
  next_expected_transport_position_ =
    current_transport_position + time_nfo.nframes_;
  last_seen_transport_state_ = transport_state;
}

// ========== AudioTimelineDataProvider Implementation ==========

void
AudioTimelineDataProvider::set_audio_clips (
  std::span<const dsp::AudioTimelineDataCache::AudioClipEntry> clips)
{
  decltype (active_audio_clips_)::ScopedAccess<farbot::ThreadType::nonRealtime>
    rt_clips{ active_audio_clips_ };
  rt_clips->assign (clips.begin (), clips.end ());
}

void
AudioTimelineDataProvider::clear_all_caches ()
{
  audio_cache_->clear ();
  decltype (active_audio_clips_)::ScopedAccess<farbot::ThreadType::nonRealtime>
    rt_clips{ active_audio_clips_ };
  rt_clips->clear ();
}

void
AudioTimelineDataProvider::cache_audio_clip (const arrangement::AudioClip &clip)
{
  // Audio clip processing
  auto audio_buffer = std::make_unique<juce::AudioSampleBuffer> ();

  // Serialize the audio clip
  arrangement::ClipRenderer::serialize_to_buffer (clip, *audio_buffer);

  // Add to cache with proper timing
  audio_cache_->add_audio_clip (
    std::make_pair (
      clip.get_tempo_map ().tick_to_samples_rounded (clip.position ()->asTick ()),
      clip.get_end_position_samples (true)),
    *audio_buffer);
}

void
AudioTimelineDataProvider::remove_sequences_matching_interval_from_all_caches (
  IntervalType interval)
{
  audio_cache_->remove_sequences_matching_interval (interval);
}

std::span<const dsp::AudioTimelineDataCache::AudioClipEntry>
AudioTimelineDataProvider::audio_clips () const
{
  return audio_cache_->audio_clips ();
}

void
AudioTimelineDataProvider::process_audio_events (
  const dsp::graph::ProcessBlockInfo &time_nfo,
  dsp::ITransport::PlayState          transport_state,
  std::span<float>                    output_left,
  std::span<float>                    output_right) noexcept
{
  // Set to true to enable debug logging for timeline data provider
  static constexpr bool TIMELINE_DATA_PROVIDER_DEBUG = false;

  const bool transport_rolling =
    transport_state == dsp::ITransport::PlayState::Rolling;
  const auto current_transport_position = time_nfo.transport_position_;

  // Only process audio events if transport is rolling
  if (!transport_rolling)
    {
      // Update tracking for next time
      next_expected_transport_position_ =
        current_transport_position + time_nfo.nframes_;
      return;
    }

  decltype (active_audio_clips_)::ScopedAccess<farbot::ThreadType::realtime>
    audio_clips{ active_audio_clips_ };

  const auto start_frame = time_nfo.transport_position_;
  const auto end_frame = start_frame + time_nfo.nframes_;

  if constexpr (TIMELINE_DATA_PROVIDER_DEBUG)
    {
      z_debug (
        "process_audio_events: start_frame={}, end_frame={}, nframes_={}",
        start_frame, end_frame, time_nfo.nframes_);
      z_debug ("Processing {} audio clips", audio_clips->size ());
    }

  // Process each audio clip that overlaps with the current time range
  for (const auto &clip : *audio_clips)
    {
      if constexpr (TIMELINE_DATA_PROVIDER_DEBUG)
        {
          z_debug (
            "Clip: start_sample={}, end_sample={}, buffer_size={}",
            clip.start_sample, clip.end_sample,
            clip.audio_buffer.getNumSamples ());
        }

      // Check if clip overlaps with current time range
      if (clip.end_sample <= start_frame || clip.start_sample >= end_frame)
        {
          if constexpr (TIMELINE_DATA_PROVIDER_DEBUG)
            z_debug ("Clip does not overlap with time range, skipping");
          continue;
        }

      // Calculate the overlap range
      const auto overlap_start = au::max (clip.start_sample, start_frame);
      const auto overlap_end = au::min (clip.end_sample, end_frame);
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
      const auto buffer_offset = overlap_start - clip.start_sample;
      const auto output_offset = overlap_start - start_frame;

      if constexpr (TIMELINE_DATA_PROVIDER_DEBUG)
        {
          z_debug (
            "Offsets: buffer_offset={}, output_offset={}", buffer_offset,
            output_offset);
        }

      // Get the audio buffer from the clip
      const auto &audio_buffer = clip.audio_buffer;
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
          const auto output_buffer_limit =
            au::max (time_nfo.nframes_ - output_offset, units::samples (0));

          // Also ensure we don't exceed the actual span size
          const auto actual_output_span_size =
            units::samples (static_cast<int64_t> (output_data.size ()));
          const auto span_based_limit =
            (output_offset >= actual_output_span_size)
              ? units::samples (static_cast<uint64_t> (0))
              : actual_output_span_size - output_offset;

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
              if (output_offset + actual_overlap_length > time_nfo.nframes_)
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
            std::views::iota (0zu, actual_overlap_length.in (units::samples)))
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
                && output_idx < time_nfo.nframes_)
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
    current_transport_position + time_nfo.nframes_;
}

// ========== AutomationTimelineDataProvider Implementation ==========

void
AutomationTimelineDataProvider::set_automation_sequences (
  std::span<const dsp::AutomationTimelineDataCache::AutomationCacheEntry>
    sequences)
{
  decltype (active_automation_sequences_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    rt_sequences{ active_automation_sequences_ };
  rt_sequences->assign (sequences.begin (), sequences.end ());
}

void
AutomationTimelineDataProvider::clear_all_caches ()
{
  automation_cache_->clear ();
  decltype (active_automation_sequences_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    rt_sequences{ active_automation_sequences_ };
  rt_sequences->clear ();
}

void
AutomationTimelineDataProvider::
  remove_sequences_matching_interval_from_all_caches (IntervalType interval)
{
  automation_cache_->remove_sequences_matching_interval (interval);
}

std::span<const dsp::AutomationTimelineDataCache::AutomationCacheEntry>
AutomationTimelineDataProvider::automation_sequences () const
{
  return automation_cache_->automation_sequences ();
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
              const auto value = sequence.automation_values[idx];
              return value >= 0.f ? std::make_optional (value) : std::nullopt;
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
  const dsp::graph::ProcessBlockInfo &time_nfo,
  dsp::ITransport::PlayState          transport_state,
  std::span<float>                    output_values) noexcept
{
  const bool transport_rolling =
    transport_state == dsp::ITransport::PlayState::Rolling;

  // Only process automation events if transport is rolling
  if (!transport_rolling)
    {
      return;
    }

  const auto start_frame = time_nfo.transport_position_;
  const auto nframes = time_nfo.nframes_;

  // Generate automation values for each sample in the time range
  for (const auto i : std::views::iota (0zu, nframes.in (units::samples)))
    {
      const auto sample_position = start_frame + units::samples (i);
      const auto automation_value = get_automation_value_rt (sample_position);
      output_values[i] = automation_value.value_or (-1.f);
    }
}

TimelineDataProvider::~TimelineDataProvider () = default;

// ========== Constructor Definitions ==========

MidiTimelineDataProvider::MidiTimelineDataProvider (QObject * parent)
    : TimelineDataProvider (parent),
      midi_cache_ (utils::make_qobject_unique<dsp::MidiTimelineDataCache> (this))
{
}

AudioTimelineDataProvider::AudioTimelineDataProvider (QObject * parent)
    : TimelineDataProvider (parent),
      audio_cache_ (utils::make_qobject_unique<dsp::AudioTimelineDataCache> (this))
{
}

AutomationTimelineDataProvider::AutomationTimelineDataProvider (QObject * parent)
    : TimelineDataProvider (parent),
      automation_cache_ (
        utils::make_qobject_unique<dsp::AutomationTimelineDataCache> (this))
{
}

// ========== AutomationTimelineDataProvider Private Methods ==========

void
AutomationTimelineDataProvider::cache_automation_clip (
  const arrangement::AutomationClip &clip,
  const dsp::TempoMap               &tempo_map)
{
  // Calculate number of samples needed
  const auto start_sample =
    tempo_map.tick_to_samples_rounded (clip.position ()->asTick ());
  const auto end_sample = clip.get_end_position_samples (true);
  const auto num_samples = end_sample - start_sample;

  std::vector<float> automation_values (num_samples.in (units::samples));

  // Serialize automation clip to sample-accurate values
  arrangement::ClipRenderer::serialize_to_automation_values (
    clip, automation_values);

  automation_cache_->add_automation_sequence (
    std::make_pair (start_sample, end_sample), automation_values);
}

} // namespace zrythm::structure::arrangement
