// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include <fmt/std.h>

#include "dsp/chord_descriptor.h"
#include "dsp/tempo_warp_map.h"
#include "dsp/time_warp_map.h"
#include "dsp/timestretch_engine.h"
#include "structure/arrangement/audio_clip.h"
#include "structure/arrangement/automation_clip.h"
#include "structure/arrangement/chord_clip.h"
#include "structure/arrangement/clip_renderer.h"
#include "structure/arrangement/midi_clip.h"
#include "utils/float_ranges.h"
#include "utils/math_utils.h"
#include "utils/views.h"

namespace zrythm::structure::arrangement
{

// Define to enable debug logging for clip serializer
static constexpr bool CLIP_SERIALIZER_DEBUG = false;

ClipRenderer::LoopParameters::LoopParameters (const Clip &clip)
{
  loop_start = clip.loopStartPosition ()->asTick ();
  loop_end = clip.loopEndPosition ()->asTick ();
  clip_start = clip.clipStartPosition ()->asTick ();
  loop_length = std::max (dsp::ContentTick{}, loop_end - loop_start);
  clip_length = clip.length ()->asTick ();
  num_loops =
    loop_length > dsp::ContentTick{}
      ? static_cast<int> (std::ceil (
          (clip_length - loop_start + clip_start).asDouble ()
          / loop_length.asDouble ()))
      : 0;
}

void
ClipRenderer::handle_midi_clip_range (
  const MidiClip                               &clip,
  juce::MidiMessageSequence                    &events,
  std::pair<dsp::ContentTick, dsp::ContentTick> virtual_range,
  dsp::TimelineTick                             segment_start)
{
  const auto * warp = clip.contentWarp ();

  // Timeline positions of the segment boundaries
  const auto tl_segment_start = warp->contentToTimeline (virtual_range.first);
  const auto tl_segment_end = warp->contentToTimeline (virtual_range.second);
  const auto segment_end = segment_start + (tl_segment_end - tl_segment_start);

  // Helper: content position → timeline position within this loop segment
  auto to_timeline = [&] (dsp::ContentTick ct) -> dsp::TimelineTick {
    return segment_start + (warp->contentToTimeline (ct) - tl_segment_start);
  };

  // Add notes for this loop segment
  for (
    const auto * note :
    clip.ArrangerObjectOwner<MidiNote>::get_children_view ()
      | std::views::filter ([] (const auto * n) {
          // Only check unmuted notes
          return !n->mute ()->muted ();
        }))
    {
      const auto note_v_start = note->position ()->asTick ();
      const auto note_v_end = note_v_start + note->length ()->asTick ();

      // Only include notes that fall within the loop range
      if (
        note_v_start >= virtual_range.second
        || note_v_end <= virtual_range.first)
        continue;

      const auto note_start =
        std::max (segment_start, to_timeline (note_v_start));
      const auto note_end = std::min (segment_end, to_timeline (note_v_end));

      const auto ch = note->midiChannel () + 1;
      events.addEvent (
        juce::MidiMessage::noteOn (
          ch, note->pitch (), static_cast<std::uint8_t> (note->velocity ())),
        note_start.asDouble ());
      events.addEvent (
        juce::MidiMessage::noteOff (
          ch, note->pitch (), static_cast<std::uint8_t> (note->velocity ())),
        note_end.asDouble ());
    }

  for (
    const auto * ev :
    clip.ArrangerObjectOwner<MidiControlEvent>::get_children_view ()
      | std::views::filter ([&] (const auto * e) {
          const auto start = e->position ()->asTick ();
          return start >= virtual_range.first && start < virtual_range.second;
        }))
    {
      const auto ev_start = to_timeline (ev->position ()->asTick ());

      const auto ch = ev->midiChannel () + 1;
      switch (ev->controlEventType ())
        {
        case MidiControlEvent::EventType::ControlChange:
          events.addEvent (
            juce::MidiMessage::controllerEvent (
              ch, ev->midiController (),
              static_cast<std::uint8_t> (ev->midiValue ())),
            ev_start.asDouble ());
          break;
        case MidiControlEvent::EventType::PitchBend:
          events.addEvent (
            juce::MidiMessage::pitchWheel (
              ch, static_cast<int> (ev->midiValue ())),
            ev_start.asDouble ());
          break;
        case MidiControlEvent::EventType::ChannelPressure:
          events.addEvent (
            juce::MidiMessage::channelPressureChange (
              ch, static_cast<std::uint8_t> (ev->midiValue ())),
            ev_start.asDouble ());
          break;
        case MidiControlEvent::EventType::PolyKeyPressure:
          events.addEvent (
            juce::MidiMessage::aftertouchChange (
              ch, ev->midiController (),
              static_cast<std::uint8_t> (ev->midiValue ())),
            ev_start.asDouble ());
          break;
        case MidiControlEvent::EventType::ProgramChange:
          events.addEvent (
            juce::MidiMessage::programChange (
              ch, static_cast<std::uint8_t> (ev->midiValue ())),
            ev_start.asDouble ());
          break;
        }
    }
}

void
ClipRenderer::handle_audio_clip_range (
  const AudioClip                              &clip,
  juce::AudioSampleBuffer                      &buffer,
  std::pair<dsp::ContentTick, dsp::ContentTick> virtual_range,
  dsp::TimelineTick                             segment_start)
{
  const auto * warp = clip.contentWarp ();

  // Timeline positions of the segment boundaries
  const auto tl_segment_start = warp->contentToTimeline (virtual_range.first);
  const auto tl_segment_end = warp->contentToTimeline (virtual_range.second);
  const auto segment_end = segment_start + (tl_segment_end - tl_segment_start);

  // Get the audio source from the clip
  auto &audio_source = clip.get_audio_source ();

  // Convert tick positions to sample positions
  const auto &tempo_map = clip.get_tempo_map ();
  const auto  segment_start_samples =
    tempo_map.tick_to_samples_rounded (segment_start);
  const auto segment_end_samples =
    tempo_map.tick_to_samples_rounded (segment_end);
  const auto segment_length_samples =
    segment_end_samples - segment_start_samples;

  assert (segment_length_samples > units::samples (0));

  // Create a temporary buffer for this segment
  const auto segment_length_samples_int =
    static_cast<int> (segment_length_samples.in (units::samples));
  juce::AudioSampleBuffer temp_buffer (
    buffer.getNumChannels (), segment_length_samples_int);
  temp_buffer.clear ();

  // Calculate the read position in the audio source.
  // Content position → timeline samples via warp.
  const auto virtual_start_samples =
    warp->contentToTimelineSamples (virtual_range.first);

  // Read audio data from the source
  const auto source_length = audio_source.getTotalLength ();
  const auto virtual_start_samples_int =
    static_cast<int> (virtual_start_samples.in (units::samples));

  if (virtual_start_samples_int < source_length)
    {
      const auto readable_length = std::min (
        segment_length_samples_int,
        static_cast<int> (source_length - virtual_start_samples_int));

      audio_source.setNextReadPosition (virtual_start_samples_int);

      juce::AudioSourceChannelInfo source_ch_nfo{
        &temp_buffer, 0, readable_length
      };
      audio_source.getNextAudioBlock (source_ch_nfo);
    }

  // Mix into the output buffer at the correct position
  const auto segment_start_samples_int =
    static_cast<int> (segment_start_samples.in (units::samples));

  // Ensure the output buffer is large enough
  if (
    segment_start_samples_int + segment_length_samples_int
    > buffer.getNumSamples ())
    {
      buffer.setSize (
        buffer.getNumChannels (),
        segment_start_samples_int + segment_length_samples_int, true, true,
        false);
    }

  // Mix the temporary buffer into the output buffer
  for (int channel = 0; channel < buffer.getNumChannels (); ++channel)
    {
      buffer.addFrom (
        channel, segment_start_samples_int, temp_buffer, channel, 0,
        segment_length_samples_int);
    }
}

void
ClipRenderer::handle_chord_clip_range (
  const ChordClip                              &clip,
  juce::MidiMessageSequence                    &events,
  std::pair<dsp::ContentTick, dsp::ContentTick> virtual_range,
  dsp::TimelineTick                             segment_start)
{
  const auto * warp = clip.contentWarp ();

  // Timeline positions of the segment boundaries
  const auto tl_segment_start = warp->contentToTimeline (virtual_range.first);
  const auto tl_segment_end = warp->contentToTimeline (virtual_range.second);
  const auto segment_end = segment_start + (tl_segment_end - tl_segment_start);

  // Helper: content position → timeline position within this loop segment
  auto to_timeline = [&] (dsp::ContentTick ct) -> dsp::TimelineTick {
    return segment_start + (warp->contentToTimeline (ct) - tl_segment_start);
  };

  // Gather the unmuted chord objects that fall within this loop segment,
  // together with their absolute start position. A ChordObject has no
  // duration of its own, so each chord is held until the next chord begins
  // (or until the segment end for the last chord, so that it re-triggers
  // cleanly on loop wrap-around).
  struct ChordInSegment
  {
    const ChordObject * chord;
    dsp::TimelineTick   segment_start;
  };
  std::vector<ChordInSegment> active_chords;
  // Add chord objects for this loop segment
  for (
    const auto * chord_object :
    clip.get_children_view () | std::views::filter ([] (const auto * co) {
      // Only check unmuted chord objects
      return !co->mute ()->muted ();
    }))
    {
      const auto chord_v_start = chord_object->position ()->asTick ();

      // Only include objects that fall within the loop range
      if (
        chord_v_start >= virtual_range.second
        || chord_v_start < virtual_range.first)
        continue;

      active_chords.push_back ({ chord_object, to_timeline (chord_v_start) });
    }

  std::ranges::sort (active_chords, {}, &ChordInSegment::segment_start);

  for (size_t i = 0; i < active_chords.size (); ++i)
    {
      // The chord stops when the next chord begins, or at the segment end for
      // the last chord in this segment.
      const auto note_off_absolute =
        (i + 1 < active_chords.size ())
          ? active_chords[i + 1].segment_start
          : segment_end;

      // Get the chord descriptor from the chord object
      const auto pitches =
        active_chords[i].chord->chordDescriptor ()->getMidiPitches ();

      // Add note on events for all notes in the chord
      for (auto note : pitches)
        {
          events.addEvent (
            juce::MidiMessage::noteOn (1, note, MidiNote::DEFAULT_VELOCITY),
            active_chords[i].segment_start.asDouble ());
          events.addEvent (
            juce::MidiMessage::noteOff (1, note, MidiNote::DEFAULT_VELOCITY),
            note_off_absolute.asDouble ());
        }
    }
}

void
ClipRenderer::handle_automation_clip_range (
  const AutomationClip                         &clip,
  std::vector<float>                           &values,
  std::pair<dsp::ContentTick, dsp::ContentTick> virtual_range,
  dsp::TimelineTick                             segment_start)
{
  const auto * warp = clip.contentWarp ();

  // Timeline positions of the segment boundaries
  const auto tl_segment_start = warp->contentToTimeline (virtual_range.first);
  const auto tl_segment_end = warp->contentToTimeline (virtual_range.second);
  const auto segment_end = segment_start + (tl_segment_end - tl_segment_start);

  const auto get_ap_virtual_position = [] (const auto * given_ap) {
    assert (given_ap != nullptr);
    return given_ap->position ()->asTick ();
  };

  const auto &tempo_map = clip.get_tempo_map ();

  // Helper: content position → timeline position within this loop segment
  auto to_timeline = [&] (dsp::ContentTick ct) -> dsp::TimelineTick {
    return segment_start + (warp->contentToTimeline (ct) - tl_segment_start);
  };

  // Helper lambda to interpolate values between two automation points
  const auto interpolate_values =
    [&] (
      const AutomationPoint &prev_point, const AutomationPoint &next_point,
      dsp::TimelineTick                               prev_point_start,
      std::pair<dsp::TimelineTick, dsp::TimelineTick> range_in_output_buffer) {
      const auto start_idx = static_cast<size_t> (
        tempo_map.tick_to_samples_rounded (range_in_output_buffer.first)
          .in (units::samples));
      if (start_idx >= values.size ())
        return;

      const auto tl_prev = to_timeline (prev_point.position ()->asTick ());
      const auto tl_next = to_timeline (next_point.position ()->asTick ());
      const auto tl_distance = tl_next - tl_prev;
      assert (tl_distance > dsp::TimelineTick{});

      const auto distance_between_points_in_segment =
        std::min (tl_next, range_in_output_buffer.second)
        - std::max (tl_prev, range_in_output_buffer.first);

      const auto start_val = prev_point.value ();
      const auto end_val = next_point.value ();
      const auto diff = end_val - start_val;
      const auto curve_up = start_val < end_val;

      // Calculate the offset from the previous point to the start position
      const auto start_offset_ratio = static_cast<float> (
        (range_in_output_buffer.first.asDouble () - prev_point_start.asDouble ())
        / tl_distance.asDouble ());
      assert (start_offset_ratio >= 0.f && start_offset_ratio <= 1.f);

      // Calculate the segment length in samples
      const auto segment_length_samples = static_cast<size_t> (
        (tempo_map.tick_to_samples_rounded (range_in_output_buffer.second)
         - tempo_map.tick_to_samples_rounded (range_in_output_buffer.first))
          .in (units::samples));

      // And the distance between the points in samples so we can interpolate
      // correctly
      const auto sample_distance_between_points =
        tempo_map.tick_to_samples (tl_distance);
      [[maybe_unused]] const auto sample_distance_between_points_in_segment =
        tempo_map.tick_to_samples (distance_between_points_in_segment);

      if (
        segment_length_samples > 0 && tl_distance > dsp::TimelineTick{}
        && start_idx + segment_length_samples <= values.size ())
        {
          // Check if this is a linear curve (most common case)
          const bool is_linear_curve =
            prev_point.curveOpts ()->algorithm ()
              != dsp::CurveOptions::Algorithm::Pulse
            && utils::math::floats_equal (
              prev_point.curveOpts ()->curviness (), 0.0);

          if (is_linear_curve)
            {
              // Fast path for linear curves - use simple linear interpolation
              // Calculate the interpolated value at the start position
              const auto start_val_interp =
                start_val + (diff * start_offset_ratio);

              // Calculate the step for interpolation from start to end
              const auto step =
                (end_val - start_val_interp)
                / static_cast<float> (
                  sample_distance_between_points.in (units::samples));

              // Interpolate from start position to the end position
              for (size_t i = 0; i < segment_length_samples; ++i)
                {
                  const auto interpolated_val =
                    start_val_interp + (step * static_cast<float> (i));
                  values[start_idx + i] = interpolated_val;
                }
            }
          else
            {
              // Slower path for curved automation - use the curve's
              // interpolation method
              for (size_t i = 0; i < segment_length_samples; ++i)
                {
                  // Calculate the position within the total segment (0 to 1)
                  const auto offset_ratio =
                    static_cast<float> (i)
                    / static_cast<float> (
                      sample_distance_between_points.in (units::samples));
                  assert (offset_ratio >= 0.f && offset_ratio <= 1.f);

                  // Get the curve value (expensive operation)
                  const auto curve_val = clip.get_normalized_value_in_curve (
                    prev_point, offset_ratio);

                  // Interpolate between the two points using the curve
                  const auto interpolated_val =
                    curve_up
                      ? start_val + (std::abs (diff) * curve_val)
                      : end_val + (std::abs (diff) * curve_val);

                  values[start_idx + i] = static_cast<float> (interpolated_val);
                }
            }
        }
    };

  // Add automation for this loop segment
  auto automation_points = clip.get_sorted_children_view ();
  for (const auto &[index, ap] : utils::views::enumerate (automation_points))
    {
      const auto * prev_ap =
        index > 0
          ? *std::next (automation_points.begin (), static_cast<int> (index) - 1)
          : nullptr;
      const auto * next_ap =
        index < (automation_points.size () - 1)
          ? *std::next (automation_points.begin (), static_cast<int> (index) + 1)
          : nullptr;

      const auto ap_virtual_start = get_ap_virtual_position (ap);

      // Whether the current automation point influences the current loop
      // segment (i.e., participates in interpolation)
      const auto ap_influences_segment = (ap_virtual_start >= virtual_range.first && ap_virtual_start <= virtual_range.second)
      || (next_ap != nullptr && get_ap_virtual_position(next_ap) > virtual_range.first && get_ap_virtual_position(next_ap) <= virtual_range.second) || (prev_ap != nullptr && get_ap_virtual_position(prev_ap) >= virtual_range.first && get_ap_virtual_position(prev_ap) < virtual_range.second);

      if (!ap_influences_segment)
        continue;

      const auto ap_start = to_timeline (ap_virtual_start);

      // Handle interpolation from this point to next point
      if (next_ap != nullptr)
        {
          const auto next_ap_virtual_start = get_ap_virtual_position (next_ap);

          // If next point is within this virtual range, interpolate to it
          if (
            next_ap_virtual_start > virtual_range.first
            && next_ap_virtual_start <= virtual_range.second)
            {
              const auto next_ap_start = to_timeline (next_ap_virtual_start);

              interpolate_values (
                *ap, *next_ap, ap_start,
                std::make_pair (
                  std::max (ap_start, segment_start), next_ap_start));
            }
          // If next point is beyond this virtual range, interpolate to range end
          else if (next_ap_virtual_start > virtual_range.second)
            {
              interpolate_values (
                *ap, *next_ap, ap_start,
                std::make_pair (std::max (ap_start, segment_start), segment_end));
            }
        }
      else
        {
          // This is the last point, fill remaining samples with its value
          const auto start_idx = static_cast<size_t> (
            tempo_map.tick_to_samples_rounded (ap_start).in (units::samples));
          const auto end_idx = static_cast<size_t> (
            tempo_map.tick_to_samples_rounded (segment_end).in (units::samples));

          if (start_idx < values.size () && end_idx > start_idx)
            {
              utils::float_ranges::fill (
                std::span (values).subspan (start_idx, end_idx - start_idx),
                ap->value ());
            }
        }
    }
}

void
ClipRenderer::serialize_to_sequence (
  const MidiClip              &clip,
  juce::MidiMessageSequence   &events,
  std::optional<TimelineRange> timeline_range_ticks)
{
  serialize_clip (clip, events, timeline_range_ticks);
}

void
ClipRenderer::serialize_to_sequence (
  const ChordClip             &clip,
  juce::MidiMessageSequence   &events,
  std::optional<TimelineRange> timeline_range_ticks)
{
  serialize_clip (clip, events, timeline_range_ticks);
}

/**
 * Serializes an Automation clip to sample-accurate automation values.
 */
void
ClipRenderer::serialize_to_automation_values (
  const AutomationClip        &clip,
  std::vector<float>          &values,
  std::optional<TimelineRange> timeline_range_ticks)
{
  const LoopParameters loop_params (clip);
  const auto          &tempo_map = clip.get_tempo_map ();

  if constexpr (CLIP_SERIALIZER_DEBUG)
    {
      z_debug (
        "serialize_automation_clip: timeline_range_ticks={}, values_size={}",
        timeline_range_ticks, values.size ());
    }

  // Always use the full clip length for the output buffer.
  // contentToTimeline returns absolute positions, so subtract the
  // clip start to get the relative length.
  const auto * warp = clip.contentWarp ();
  const auto   clip_start_tl = warp->contentToTimeline (dsp::ContentTick{});
  const auto   clip_end_tl = warp->contentToTimeline (loop_params.clip_length);
  const auto   clip_length_samples =
    tempo_map.tick_to_samples_rounded (clip_end_tl)
    - tempo_map.tick_to_samples_rounded (clip_start_tl);

  // Resize the output buffer to the full clip length
  values.resize (static_cast<size_t> (clip_length_samples.in (units::samples)));

  // Initialize with default value (-1.0)
  utils::float_ranges::fill (values, -1);

  if constexpr (CLIP_SERIALIZER_DEBUG)
    {
      z_debug (
        "Processing automation clip with loop_start={}, loop_end={}, clip_length={}",
        loop_params.loop_start, loop_params.loop_end, loop_params.clip_length);
    }

  // Use the serialize_clip template to handle the looping and automation data
  serialize_clip (clip, values, timeline_range_ticks);

  // Apply constraints in a second pass
  if (timeline_range_ticks)
    {
      const auto clip_start = clip.position ()->asTick ();
      const auto constraint_start = timeline_range_ticks->first;
      const auto constraint_end = timeline_range_ticks->second;

      if constexpr (CLIP_SERIALIZER_DEBUG)
        {
          z_debug (
            "Applying constraints: clip_start={}, constraint_start={}, constraint_end={}",
            clip_start, constraint_start, constraint_end);
        }

      // Calculate the sample range that corresponds to the constraint range
      const auto constraint_start_offset =
        std::max (dsp::TimelineTick{}, constraint_start - clip_start);
      const auto constraint_end_offset =
        std::max (dsp::TimelineTick{}, constraint_end - clip_start);

      const auto constraint_start_samples = static_cast<size_t> (
        tempo_map.tick_to_samples_rounded (constraint_start_offset)
          .in (units::samples));
      const auto constraint_end_samples = static_cast<size_t> (
        tempo_map.tick_to_samples_rounded (constraint_end_offset)
          .in (units::samples));

      // Trim values outside the constraint range
      for (size_t i = 0; i < values.size (); ++i)
        {
          if (i < constraint_start_samples || i >= constraint_end_samples)
            {
              values[i] = -1.0f;
            }
        }

      if (CLIP_SERIALIZER_DEBUG)
        {
          z_debug (
            "Trimmed values outside constraint range: start_samples={}, end_samples={}",
            constraint_start_samples, constraint_end_samples);
          // Log some values from the middle of the output buffer where we
          // expect to see the automation values
          z_debug ("Output buffer middle values (around 1148):");
          for (
            size_t i = 1140; i < std::min (size_t{ 1150 }, values.size ()); ++i)
            {
              z_debug ("  [{}] = {}", i, values[i]);
            }
        }

      if constexpr (CLIP_SERIALIZER_DEBUG)
        {
          // Log some values from the output buffer to help debug
          z_debug ("Output buffer first 10 values:");
          for (size_t i = 0; i < std::min (size_t{ 10 }, values.size ()); ++i)
            {
              z_debug ("  [{}] = {}", i, values[i]);
            }
        }
    }
}

/**
 * Serializes an Audio clip to an audio sample buffer.
 *
 * Audio clips are always serialized as they would be played in the timeline
 * (with loops and clip start).
 */
utils::audio::AudioBuffer
ClipRenderer::build_native_looped_buffer (
  const AudioClip &clip,
  units::sample_t  out_start,
  units::sample_t  out_end)
{
  auto       &fs = clip.get_children_view ().front ()->file_audio_source ();
  const auto &samples = fs.get_samples ();
  const int   channels = samples.getNumChannels ();
  const auto  clip_frames = units::samples (samples.getNumSamples ());
  const auto  source_bpm = fs.source_bpm ();
  const auto  sr = clip.get_tempo_map ().get_sample_rate ();
  const auto  effective_bpm =
    source_bpm > units::bpm (0.0)
      ? source_bpm
      : clip.get_tempo_map ().tempo_at_tick (
          units::ticks (static_cast<int64_t> (clip.position ()->ticks ())));

  // Convert a clip-internal position (always Musical ticks) to a native sample
  // offset, using the clip's source BPM (or project tempo fallback).
  const auto native_offset = [&] (const dsp::Position * pos) -> units::sample_t {
    return au::round_as<int64_t> (
      units::samples, (units::ticks (pos->ticks ()) / effective_bpm * sr));
  };

  const auto clip_start_s = clamp (
    native_offset (clip.clipStartPosition ()), units::samples (0), clip_frames);
  const auto loop_start_s = clamp (
    native_offset (clip.loopStartPosition ()), units::samples (0), clip_frames);
  const auto loop_end_raw = clamp (
    native_offset (clip.loopEndPosition ()), units::samples (0), clip_frames);
  const auto loop_end_s =
    std::max (loop_end_raw, loop_start_s + units::samples (1));
  const bool do_loop = clip.looped ();

  const auto out0 = max (units::samples (0), out_start);
  const auto out1 = max (out0, out_end);
  const auto out_len = out1 - out0;

  utils::audio::AudioBuffer b1 (channels, out_len.in<int> (units::samples));
  b1.clear ();

  // Clip read position for an output position (native samples from clip
  // start), computed in O(1): the first leg plays clip_start -> loop_end;
  // subsequent legs loop loop_start -> loop_end. This lets a sub-range request
  // start reading at @p out_start directly instead of iterating from 0.
  const auto clip_pos_at = [&] (units::sample_t out_pos) -> units::sample_t {
    if (!do_loop)
      return clip_start_s + out_pos;
    const auto first_leg = loop_end_s - clip_start_s;
    if (out_pos < first_leg)
      return clip_start_s + out_pos;
    const auto loop_len = max (units::samples (1), loop_end_s - loop_start_s);
    return loop_start_s + ((out_pos - first_leg) % loop_len);
  };

  auto read_pos = clip_pos_at (out0);
  auto write_pos = units::samples (0);
  while (write_pos < out_len)
    {
      // au does not support min() on a tuple/initializer list so we do double
      // min()
      const auto this_end =
        min (min (loop_end_s, clip_frames), read_pos + (out_len - write_pos));
      const auto this_len = this_end - read_pos;
      if (this_len <= units::samples (0))
        {
          if (do_loop && read_pos >= loop_end_s)
            {
              read_pos = loop_start_s;
              continue;
            }
          break; // past clip / un-looped tail: leave silence
        }
      for (int c = 0; c < channels; ++c)
        b1.copyFrom (
          c, write_pos.in<int> (units::samples), samples, c,
          read_pos.in<int> (units::samples), this_len.in<int> (units::samples));
      write_pos += this_len;
      read_pos += this_len;
      if (read_pos >= loop_end_s)
        {
          if (do_loop)
            read_pos = loop_start_s; // wrap to loop start
          else
            break; // un-looped clip: stop, remainder stays silent
        }
    }
  return b1;
}

void
ClipRenderer::serialize_to_buffer (
  const AudioClip             &clip,
  juce::AudioSampleBuffer     &buffer,
  std::optional<TimelineRange> timeline_range_ticks)
{
  const LoopParameters loop_params (clip);

  if constexpr (CLIP_SERIALIZER_DEBUG)
    {
      z_debug (
        "serialize_audio_clip: timeline_range_ticks={}, builtin_fade_frames={}",
        timeline_range_ticks, AudioClip::BUILTIN_FADE_FRAMES);
      z_debug (
        "Loop params: loop_start={}, loop_end={}, clip_start={}, clip_length={}, num_loops={}",
        loop_params.loop_start, loop_params.loop_end, loop_params.clip_start,
        loop_params.clip_length, loop_params.num_loops);
      z_debug (
        "Clip position: {} ticks, {} samples", clip.position ()->ticks (),
        clip.get_tempo_map ().tick_to_samples (clip.position ()->asTick ()));
    }

  // Check constraint overlap up front (before rendering) using tick bounds.
  const auto &tempo_map = clip.get_tempo_map ();
  const auto  clip_start_tick = clip.position ()->asTick ();
  const auto  clip_end_tick = timeline_end_ticks (clip);
  if (timeline_range_ticks)
    {
      const auto &constraint_start = timeline_range_ticks->first;
      const auto &constraint_end = timeline_range_ticks->second;
      if (clip_end_tick <= constraint_start || clip_start_tick >= constraint_end)
        {
          buffer.setSize (2, 0);
          buffer.clear ();
          return;
        }
    }

  // Source info + the clip's native (B1) length, used both for the stretch
  // decision and to map clip positions.
  auto      &fs = clip.get_children_view ().front ()->file_audio_source ();
  const auto source_bpm = fs.source_bpm ();
  const auto effective_bpm =
    source_bpm > units::bpm (0.0)
      ? source_bpm
      : tempo_map.tempo_at_tick (
          units::ticks (static_cast<int64_t> (clip.position ()->ticks ())));
  const auto native_offset = [&] (const dsp::Position * pos) -> units::sample_t {
    return au::round_as<int64_t> (
      units::samples,
      units::ticks (pos->ticks ()) / effective_bpm
        * tempo_map.get_sample_rate ());
  };
  const auto native_clip_len =
    max (units::samples (0), native_offset (clip.length ()));

  // Compute the sample-space warp map from ContentTimeWarp's canonical warp
  // points. This unified path handles both musical-mode cases: identity warp
  // (musical ON → stretch to project tempo) and tempo-derived warp (musical
  // OFF → native speed). The stretch decision is based on sample-space anchors.
  bool             needs_stretch = false;
  units::sample_t  timeline_clip_len = native_clip_len;
  dsp::TimeWarpMap warp;
  if (source_bpm > units::bpm (0.0) && native_clip_len > units::samples (0))
    {
      auto warp_points = clip.contentWarp ()->warpPoints ();
      warp = dsp::to_time_warp_map (
        warp_points, tempo_map, clip_start_tick, source_bpm, native_clip_len);
      needs_stretch = !dsp::is_sample_space_identity (warp.anchors);
      timeline_clip_len = warp.output_length;
    }

  // Size the output buffer and compute where the clip's audio lands in it.
  const auto clip_start_sample =
    tempo_map.tick_to_samples_rounded (clip_start_tick);
  int64_t buf_size = timeline_clip_len.in<int64_t> (units::samples);
  int64_t clip_buf_offset = 0; // buffer index where clip_audio[0] goes
  if (timeline_range_ticks)
    {
      const auto constraint_start_sample =
        tempo_map.tick_to_samples_rounded (timeline_range_ticks->first);
      const auto constraint_end_sample =
        tempo_map.tick_to_samples_rounded (timeline_range_ticks->second);
      buf_size =
        (constraint_end_sample - constraint_start_sample).in (units::samples);
      clip_buf_offset =
        clip_start_sample.in (units::samples) -constraint_start_sample.in (
          units::samples);
    }
  buffer.setSize (2, static_cast<int> (std::max (int64_t{ 0 }, buf_size)));
  buffer.clear ();

  // The clip-audio indices that overlap the buffer.
  const auto out0 = std::max (int64_t{ 0 }, -clip_buf_offset);
  const auto out1 = std::min (
    timeline_clip_len.in<int64_t> (units::samples), buf_size - clip_buf_offset);
  const auto dst_start = std::max (int64_t{ 0 }, clip_buf_offset);
  const auto copy_len = std::max (int64_t{ 0 }, out1 - out0);
  if (copy_len > 0)
    {
      utils::audio::AudioBuffer content;
      if (needs_stretch)
        {
          // Genuine musical stretch: render the FULL clip (offline RubberBand
          // needs the whole input for a seamless result) then slice the range.
          // Note: unlike the no-stretch branch below, this is O(full clip)
          // even for a sub-range request — unavoidable for offline quality.
          auto full_b1 = build_native_looped_buffer (
            clip, units::samples (0), native_clip_len);
          dsp::StretchOptions stretch_opts;
          stretch_opts.algorithm = clip.effectiveStretchAlgorithm ();
          auto engine = dsp::create_default_timestretch_engine (
            stretch_opts,
            au::round_as<int> (units::sample_rate, tempo_map.get_sample_rate ()));
          content = engine->stretch (full_b1, warp, stretch_opts);
          const int chans =
            std::min (buffer.getNumChannels (), content.getNumChannels ());
          for (int c = 0; c < chans; ++c)
            buffer.copyFrom (
              c, static_cast<int> (dst_start), content, c,
              static_cast<int> (out0), static_cast<int> (copy_len));
        }
      else
        {
          // No stretch needed: read ONLY the requested range directly from the
          // clip (O(range)) — this is the recording / incremental-update path.
          content = build_native_looped_buffer (
            clip, units::samples (out0), units::samples (out1));
          const int chans =
            std::min (buffer.getNumChannels (), content.getNumChannels ());
          for (int c = 0; c < chans; ++c)
            buffer.copyFrom (
              c, static_cast<int> (dst_start), content, c, 0,
              static_cast<int> (copy_len));
        }
    }

  if constexpr (CLIP_SERIALIZER_DEBUG)
    {
      z_debug (
        "Buffer setup: timeline_clip_len={}, buffer samples={}, stretch={}",
        timeline_clip_len, buffer.getNumSamples (), needs_stretch);
    }

  // Second pass: apply gain to the entire buffer
  apply_gain_pass (clip, buffer);

  if constexpr (CLIP_SERIALIZER_DEBUG)
    {
      z_debug (
        "After gain pass: first sample L={}, R={}, last sample L={}, R={}",
        buffer.getSample (0, 0), buffer.getSample (1, 0),
        buffer.getNumSamples () > 0
          ? buffer.getSample (0, buffer.getNumSamples () - 1)
          : 0.0f,
        buffer.getNumSamples () > 0
          ? buffer.getSample (1, buffer.getNumSamples () - 1)
          : 0.0f);
    }

  // Third pass: apply clip fades (object fades)
  apply_clip_fades_pass (clip, buffer);

  if constexpr (CLIP_SERIALIZER_DEBUG)
    {
      z_debug (
        "After clip fades pass: first sample L={}, R={}, last sample L={}, R={}",
        buffer.getSample (0, 0), buffer.getSample (1, 0),
        buffer.getNumSamples () > 0
          ? buffer.getSample (0, buffer.getNumSamples () - 1)
          : 0.0f,
        buffer.getNumSamples () > 0
          ? buffer.getSample (1, buffer.getNumSamples () - 1)
          : 0.0f);
    }

  // Fourth pass: apply built-in fades
  apply_builtin_fades_pass (clip, buffer, AudioClip::BUILTIN_FADE_FRAMES);

  if constexpr (CLIP_SERIALIZER_DEBUG)
    {
      z_debug (
        "After builtin fades pass: first sample L={}, R={}, last sample L={}, R={}",
        buffer.getSample (0, 0), buffer.getSample (1, 0),
        buffer.getNumSamples () > 0
          ? buffer.getSample (0, buffer.getNumSamples () - 1)
          : 0.0f,
        buffer.getNumSamples () > 0
          ? buffer.getSample (1, buffer.getNumSamples () - 1)
          : 0.0f);
    }
}

/**
 * Applies gain to the entire audio buffer as a separate pass.
 */
void
ClipRenderer::apply_gain_pass (
  const AudioClip         &clip,
  juce::AudioSampleBuffer &buffer)
{
  const auto current_gain = clip.gain ();

  if constexpr (CLIP_SERIALIZER_DEBUG)
    {
      z_debug ("apply_gain_pass: gain={}", current_gain);
    }

  if (!utils::math::floats_equal (current_gain, 1.f))
    {
      buffer.applyGain (current_gain);

      if constexpr (CLIP_SERIALIZER_DEBUG)
        {
          z_debug (
            "Applied gain {}: first sample L={}, R={}", current_gain,
            buffer.getSample (0, 0), buffer.getSample (1, 0));
        }
    }
  else
    {
      if constexpr (CLIP_SERIALIZER_DEBUG)
        {
          z_debug ("Gain is 1.0, skipping gain pass");
        }
    }
}

/**
 * Applies clip (object) fades to the audio buffer as a separate pass.
 */
void
ClipRenderer::apply_clip_fades_pass (
  const AudioClip         &clip,
  juce::AudioSampleBuffer &buffer)
{
  // TODO: Fades are always in project ticks (follow tempo). For non-musical
  // clips, a fade stored in beats will change in wall-clock time when tempo
  // changes (e.g., 50ms at 120 BPM → ~43ms at 140 BPM). Consider adding a
  // fadeTimeUnit="seconds" option for fixed-duration micro-fades.
  const auto &tempo_map = clip.get_tempo_map ();
  const auto  clip_length_in_frames = static_cast<int> (
    clip.get_end_position_samples (true)
      .in (units::samples) -tempo_map
      .tick_to_samples_rounded (clip.position ()->asTick ())
      .in (units::samples));
  const auto fade_in_pos_in_frames = static_cast<int> (
    tempo_map
      .tick_to_samples_rounded (
        dsp::TimelineTick{
          units::ticks (clip.fadeRange ()->startOffset ()->ticks ()) })
      .in (units::samples));
  const auto fade_out_pos_in_frames =
    clip_length_in_frames
    - static_cast<int> (
      tempo_map
        .tick_to_samples_rounded (
          dsp::TimelineTick{
            units::ticks (clip.fadeRange ()->endOffset ()->ticks ()) })
        .in (units::samples));
  const auto num_frames_in_fade_in_area = fade_in_pos_in_frames;
  const auto num_frames_in_fade_out_area = static_cast<int> (
    tempo_map
      .tick_to_samples_rounded (
        dsp::TimelineTick{
          units::ticks (clip.fadeRange ()->endOffset ()->ticks ()) })
      .in (units::samples));

  if constexpr (CLIP_SERIALIZER_DEBUG)
    {
      z_debug (
        "apply_clip_fades_pass: buffer_size={}, clip_length={}, fade_in_pos={}, fade_out_pos={}",
        buffer.getNumSamples (), clip_length_in_frames, fade_in_pos_in_frames,
        fade_out_pos_in_frames);
    }

  auto * left_channel = buffer.getWritePointer (0);
  auto * right_channel = buffer.getWritePointer (1);

  for (int j = 0; j < buffer.getNumSamples (); ++j)
    {
      // For audio clips, buffer frame directly corresponds to clip frame
      // (no leading silence)
      const auto clip_local_frame = j;

      // Skip frames that are beyond the clip length
      if (clip_local_frame >= clip_length_in_frames)
        {
          if (CLIP_SERIALIZER_DEBUG && j < 5)
            {
              z_debug (
                "Skipping clip fade at frame {}: beyond clip length {}", j,
                clip_length_in_frames);
            }
          continue;
        }

      bool applied_fade = false;

      // If inside object fade in
      if (clip_local_frame >= 0 && clip_local_frame < num_frames_in_fade_in_area)
        {
          // Ensure we don't divide by zero
          if (num_frames_in_fade_in_area > 0)
            {
              auto fade_in = static_cast<
                float> (clip.fadeRange ()->get_normalized_y_for_fade (
                static_cast<double> (clip_local_frame)
                  / static_cast<double> (num_frames_in_fade_in_area),
                true));

              left_channel[j] *= fade_in;
              right_channel[j] *= fade_in;
              applied_fade = true;

              if (CLIP_SERIALIZER_DEBUG && j < 5)
                {
                  z_debug (
                    "Applied object fade in at frame {}: fade={}, L={}, R={}",
                    clip_local_frame, fade_in, left_channel[j],
                    right_channel[j]);
                }
            }
        }

      // If inside object fade out
      if (clip_local_frame >= fade_out_pos_in_frames)
        {
          const auto num_frames_from_fade_out_start =
            clip_local_frame - fade_out_pos_in_frames;
          if (
            num_frames_from_fade_out_start <= num_frames_in_fade_out_area
            && num_frames_in_fade_out_area > 0)
            {
              auto fade_out = static_cast<
                float> (clip.fadeRange ()->get_normalized_y_for_fade (
                static_cast<double> (num_frames_from_fade_out_start)
                  / static_cast<double> (num_frames_in_fade_out_area),
                false));

              left_channel[j] *= fade_out;
              right_channel[j] *= fade_out;
              applied_fade = true;

              if (CLIP_SERIALIZER_DEBUG && j < 5)
                {
                  z_debug (
                    "Applied object fade out at frame {}: fade={}, L={}, R={}",
                    clip_local_frame, fade_out, left_channel[j],
                    right_channel[j]);
                }
            }
        }

      if (
        CLIP_SERIALIZER_DEBUG && j < 5 && !applied_fade && clip_local_frame >= 0
        && clip_local_frame < clip_length_in_frames)
        {
          z_debug (
            "No clip fade applied at frame {}: L={}, R={}", clip_local_frame,
            left_channel[j], right_channel[j]);
        }
    }
}

/**
 * Applies built-in fades to the audio buffer as a separate pass.
 */
void
ClipRenderer::apply_builtin_fades_pass (
  const AudioClip         &clip,
  juce::AudioSampleBuffer &buffer,
  int                      builtin_fade_frames)
{
  const auto buffer_size = buffer.getNumSamples ();

  if constexpr (CLIP_SERIALIZER_DEBUG)
    {
      z_debug (
        "apply_builtin_fades_pass: buffer_size={}, builtin_fade_frames={}",
        buffer_size, builtin_fade_frames);
    }

  if (builtin_fade_frames <= 0 || buffer_size <= 0)
    {
      if constexpr (CLIP_SERIALIZER_DEBUG)
        {
          z_debug (
            "Built-in fade frames is {} or buffer is empty, skipping builtin fades pass",
            builtin_fade_frames);
        }
      return;
    }

  // Apply built-in fade in using JUCE's gain ramp (linear from 0.0 to 1.0)
  const auto fade_in_length = std::min (builtin_fade_frames, buffer_size);
  if (fade_in_length > 0)
    {
      if constexpr (CLIP_SERIALIZER_DEBUG)
        {
          z_debug (
            "Applying builtin fade in: {} samples (0.0 -> 1.0)", fade_in_length);
        }

      buffer.applyGainRamp (0, fade_in_length, 0.0f, 1.0f);

      if constexpr (CLIP_SERIALIZER_DEBUG)
        {
          z_debug (
            "After builtin fade in: first sample L={}, R={}, last fade sample L={}, R={}",
            buffer.getSample (0, 0), buffer.getSample (1, 0),
            buffer.getSample (0, fade_in_length - 1),
            buffer.getSample (1, fade_in_length - 1));
        }
    }

  // Apply built-in fade out using JUCE's gain ramp (linear from 1.0 to 0.0)
  // Simply apply to the last N samples of the buffer
  const auto fade_out_length = std::min (builtin_fade_frames, buffer_size);
  if (fade_out_length > 0)
    {
      const auto fade_out_start = buffer_size - fade_out_length;

      if constexpr (CLIP_SERIALIZER_DEBUG)
        {
          z_debug (
            "Applying builtin fade out: start={}, length={} samples (1.0 -> 0.0)",
            fade_out_start, fade_out_length);
        }

      buffer.applyGainRamp (fade_out_start, fade_out_length, 1.0f, 0.0f);

      if constexpr (CLIP_SERIALIZER_DEBUG)
        {
          z_debug (
            "After builtin fade out: first fade sample L={}, R={}, last sample L={}, R={}",
            buffer.getSample (0, fade_out_start),
            buffer.getSample (1, fade_out_start),
            buffer.getSample (0, buffer_size - 1),
            buffer.getSample (1, buffer_size - 1));
        }
    }

  if constexpr (CLIP_SERIALIZER_DEBUG)
    {
      // Log some samples that didn't get built-in fades applied
      const auto non_fade_start = fade_in_length;
      const auto non_fade_end = buffer_size - fade_out_length;
      if (non_fade_start < non_fade_end && non_fade_start < buffer_size)
        {
          z_debug (
            "No builtin fade applied at frame {}: L={}, R={}", non_fade_start,
            buffer.getSample (0, non_fade_start),
            buffer.getSample (1, non_fade_start));
        }
    }
}

} // namespace zrythm::structure::arrangement
