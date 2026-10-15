// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cmath>
#include <vector>

#include "dsp/curve.h"
#include "dsp/tempo_map.h"
#include "dsp/tick_types.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/automation_clip.h"
#include "structure/arrangement/clip.h"
#include "structure/arrangement/timeline_data_provider.h"
#include "utils/logger.h"

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

  // Serialize the audio clip. A missing or corrupt block file skips the
  // clip: the cache builder keeps running with the rest of the clips.
  try
    {
      arrangement::ClipRenderer::serialize_to_buffer (clip, *audio_buffer);
    }
  catch (const std::exception &e)
    {
      z_warning (
        "Failed to cache audio clip {}: {}", clip.get_uuid (), e.what ());
      return;
    }

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

namespace
{
// Maximum relative error tolerated in the sample→tick ratio mapping within
// a single subdivided tempo-ramp segment. When the error exceeds this,
// the segment is split in half recursively.
constexpr double kSubdivisionThreshold = 0.001;

// Bounds the number of segments generated by recursive subdivision of a
// linear tempo-ramp region. Each level halves the span, so a depth of 8
// yields at most 2^8 = 256 split points per ramp region — generous for
// even the longest ramps while protecting against pathological tempos.
constexpr int kMaxSubdivisionDepth = 8;
/// Evaluates a single cached automation segment at the given ratio [0, 1].
/// Shared between the in-segment path and the latched last-known-value path.
float
eval_segment (
  const dsp::AutomationTimelineDataCache::CachedAutomationSegment &seg,
  double ratio) noexcept
{
  return dsp::evaluate_curve (
    seg.point_a_value, seg.point_b_value, seg.curve_algo, seg.curve_curviness,
    ratio);
}
} // namespace

std::optional<float>
AutomationTimelineDataProvider::evaluate_at_sample (
  const std::vector<dsp::AutomationTimelineDataCache::AutomationCacheEntry>
                 &sequences,
  units::sample_t sample_position) noexcept [[clang::nonblocking]]
{
  using Seg = dsp::AutomationTimelineDataCache::CachedAutomationSegment;

  std::optional<float> last_known_value;

  for (const auto &entry : sequences)
    {
      if (
        sample_position >= entry.start_sample
        && sample_position < entry.end_sample)
        {
          if (entry.segments.empty ())
            return std::nullopt;

          // Binary search: find the last segment whose start_sample <= query.
          auto it = std::ranges::upper_bound (
            entry.segments, sample_position, {}, &Seg::start_sample);
          if (it != entry.segments.begin ())
            --it;

          if (
            it != entry.segments.end () && sample_position >= it->start_sample
            && sample_position < it->end_sample)
            {
              const auto &seg = *it;
              const auto  seg_samples = static_cast<double> (
                (seg.end_sample - seg.start_sample).in (units::samples));
              if (seg_samples <= 0.0)
                return seg.point_a_value;

              const double sub_ratio = std::clamp (
                static_cast<double> (
                  (sample_position - seg.start_sample).in (units::samples))
                  / seg_samples,
                0.0, 1.0);
              const double full_ratio =
                seg.ratio_start + (seg.ratio_end - seg.ratio_start) * sub_ratio;

              return eval_segment (seg, full_ratio);
            }
        }

      // Track last known value for latched hold behavior.
      if (entry.end_sample <= sample_position && !entry.segments.empty ())
        {
          const auto &last_seg = entry.segments.back ();
          last_known_value = eval_segment (last_seg, last_seg.ratio_end);
        }
    }

  return last_known_value;
}

std::optional<float>
AutomationTimelineDataProvider::get_automation_value_rt (
  units::sample_t sample_position) noexcept
{
  decltype (active_automation_sequences_)::ScopedAccess<
    farbot::ThreadType::realtime>
    sequences{ active_automation_sequences_ };
  return evaluate_at_sample (*sequences, sample_position);
}

void
AutomationTimelineDataProvider::process_automation_events (
  const dsp::graph::ProcessBlockInfo &time_nfo,
  dsp::ITransport::PlayState          transport_state,
  std::span<float>                    output_values) noexcept
{
  if (transport_state != dsp::ITransport::PlayState::Rolling)
    return;

  const auto start_frame = time_nfo.transport_position_;
  const auto nframes = time_nfo.nframes_;

  // Acquire realtime access once for the entire block.
  decltype (active_automation_sequences_)::ScopedAccess<
    farbot::ThreadType::realtime>
              sequences{ active_automation_sequences_ };
  const auto &seq_ref = *sequences;

  for (const auto i : std::views::iota (0zu, nframes.in (units::samples)))
    {
      const auto sample_position = start_frame + units::samples (i);
      const auto automation_value =
        evaluate_at_sample (seq_ref, sample_position);
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

namespace
{

using Seg = dsp::AutomationTimelineDataCache::CachedAutomationSegment;

/// Recursively subdivides a linear-ramp region [ta, tb] until the linear
/// sample-space approximation error at the midpoint falls below the
/// threshold. This ensures the RT reader's linear interpolation within each
/// segment is accurate even when the tempo ramps continuously.
void
subdivide_ramp (
  std::vector<dsp::TimelineTick> &splits,
  const dsp::TempoMap            &tempo_map,
  dsp::TimelineTick               ta,
  dsp::TimelineTick               tb,
  double                          threshold = kSubdivisionThreshold,
  int                             depth = 0)
{
  if ((tb - ta).asDouble () < 1.0)
    return;

  if (depth >= kMaxSubdivisionDepth)
    return;

  const auto tick_mid =
    dsp::TimelineTick{ units::ticks ((ta.asDouble () + tb.asDouble ()) / 2.0) };

  const auto sample_at = [&] (dsp::TimelineTick tick) -> double {
    return static_cast<double> (
      tempo_map.tick_to_samples_rounded (tick).in (units::samples));
  };

  const double sa = sample_at (ta);
  const double sb = sample_at (tb);
  const double sm = sample_at (tick_mid);
  const double sample_range = sb - sa;
  if (sample_range <= 0.0)
    return;

  // The RT reader linearly maps sample position → tick ratio within each
  // segment. If the actual sample position at the tick-space midpoint
  // deviates significantly from the sample-space midpoint, the linear
  // approximation is inaccurate and we need to subdivide.
  const double linear_mid = (sa + sb) / 2.0;
  const double error = std::abs (sm - linear_mid) / sample_range;

  if (error > threshold)
    {
      splits.push_back (tick_mid);
      subdivide_ramp (splits, tempo_map, ta, tick_mid, threshold, depth + 1);
      subdivide_ramp (splits, tempo_map, tick_mid, tb, threshold, depth + 1);
    }
}

/// Builds constant-tempo segments for a pair of control points, splitting at
/// tempo change boundaries so the RT reader never needs the tempo map.
/// Ratios and endpoint values are computed against the original AP-pair domain
/// (@p domain_start / @p domain_end), not the potentially-truncated segment
/// span, so that non-linear curves are not reshaped when a boundary splits an
/// AP pair.
void
build_segments_for_pair (
  std::vector<Seg>            &segments,
  const dsp::TempoMap         &tempo_map,
  dsp::TimelineTick            tl_a,
  dsp::TimelineTick            tl_b,
  dsp::TimelineTick            domain_start,
  dsp::TimelineTick            domain_end,
  float                        domain_value_a,
  float                        domain_value_b,
  dsp::CurveOptions::Algorithm algo,
  float                        curviness)
{
  if (tl_b <= tl_a)
    return;

  const double domain_span = (domain_end - domain_start).asDouble ();
  if (domain_span <= 0.0)
    return;

  // Collect split points: start, tempo events within range, end.
  // For Linear curve events, additionally subdivide the ramp region so
  // each sub-segment is small enough for accurate linear interpolation.
  std::vector<dsp::TimelineTick> splits;
  splits.push_back (tl_a);

  const auto tempo_events = tempo_map.tempo_events ();
  for (size_t i = 0; i < tempo_events.size (); ++i)
    {
      const auto &te = tempo_events[i];
      const auto  te_tick = dsp::TimelineTick{ te.tick };

      if (te_tick > tl_a && te_tick < tl_b)
        splits.push_back (te_tick);

      // Subdivide linear ramp regions that overlap [tl_a, tl_b].
      if (
        te.curve == dsp::TempoMap::CurveType::Linear
        && i + 1 < tempo_events.size ())
        {
          const auto next_tick = dsp::TimelineTick{ tempo_events[i + 1].tick };
          const auto ramp_start = std::max (te_tick, tl_a);
          const auto ramp_end = std::min (next_tick, tl_b);
          if (ramp_end > ramp_start)
            subdivide_ramp (splits, tempo_map, ramp_start, ramp_end);
        }
    }

  splits.push_back (tl_b);
  std::ranges::sort (splits);

  for (size_t i = 0; i + 1 < splits.size (); ++i)
    {
      const auto &ta = splits[i];
      const auto &tb = splits[i + 1];

      const auto sa = tempo_map.tick_to_samples_rounded (ta);
      const auto sb = tempo_map.tick_to_samples_rounded (tb);
      if (sb <= sa)
        continue;

      Seg seg;
      seg.start_sample = sa;
      seg.end_sample = sb;
      seg.ratio_start =
        static_cast<float> ((ta - domain_start).asDouble () / domain_span);
      seg.ratio_end =
        static_cast<float> ((tb - domain_start).asDouble () / domain_span);
      seg.point_a_value = domain_value_a;
      seg.point_b_value = domain_value_b;
      seg.curve_algo = algo;
      seg.curve_curviness = curviness;
      segments.push_back (std::move (seg));
    }
}

} // anonymous namespace

void
AutomationTimelineDataProvider::cache_automation_clip (
  const arrangement::AutomationClip &clip,
  const dsp::TempoMap               &tempo_map)
{
  const auto start_sample =
    tempo_map.tick_to_samples_rounded (clip.position ()->asTick ());
  const auto end_sample = clip.get_end_position_samples (true);

  dsp::AutomationTimelineDataCache::AutomationCacheEntry entry;

  std::vector<arrangement::ClipRenderer::RenderedAutomationPoint> rendered_points;
  arrangement::ClipRenderer::serialize_to_points (clip, rendered_points);

  if (!rendered_points.empty ())
    {
      const auto clip_pos = clip.position ()->asTick ();

      // Build constant-tempo segments between adjacent control points.
      for (size_t i = 0; i + 1 < rendered_points.size (); ++i)
        {
          const auto &a = rendered_points[i];
          const auto &b = rendered_points[i + 1];
          build_segments_for_pair (
            entry.segments, tempo_map, clip_pos + a.position,
            clip_pos + b.position, clip_pos + a.curve_origin_start,
            clip_pos + a.curve_origin_end, a.curve_origin_value_a,
            a.curve_origin_value_b, a.curve_algo, a.curve_curviness);
        }

      // Flat hold after the last point to clip end.
      const auto &last = rendered_points.back ();
      const auto  last_sample =
        tempo_map.tick_to_samples_rounded (clip_pos + last.position);
      if (end_sample > last_sample)
        {
          Seg seg;
          seg.start_sample = last_sample;
          seg.end_sample = end_sample;
          seg.ratio_start = 1.0f;
          seg.ratio_end = 1.0f;
          seg.point_a_value = last.value;
          seg.point_b_value = last.value;
          seg.curve_algo = dsp::CurveOptions::Algorithm::Exponent;
          seg.curve_curviness = 0.0f;
          entry.segments.push_back (std::move (seg));
        }
    }

  automation_cache_->add_automation_sequence (
    std::make_pair (start_sample, end_sample), std::move (entry));
}

} // namespace zrythm::structure::arrangement
