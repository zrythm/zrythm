// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/automation_region.h"
#include "structure/arrangement/chord_region.h"
#include "structure/arrangement/midi_region.h"
#include "structure/arrangement/region_renderer.h"
#include "utils/views.h"

namespace zrythm::structure::arrangement
{

// Define to enable debug logging for region serializer
static constexpr bool REGION_SERIALIZER_DEBUG = false;

RegionRenderer::LoopParameters::LoopParameters (const ArrangerObject &region)
{
  const auto * loop_range = region.loopRange ();
  loop_start = units::ticks (loop_range->loopStartPosition ()->ticks ());
  loop_end = units::ticks (loop_range->loopEndPosition ()->ticks ());
  clip_start = units::ticks (loop_range->clipStartPosition ()->ticks ());
  loop_length = std::max (units::ticks (0.0), loop_end - loop_start);
  region_length = units::ticks (region.bounds ()->length ()->ticks ());
  num_loops =
    loop_length > units::ticks (0.0)
      ? static_cast<int> (
          std::ceil ((region_length - loop_start + clip_start) / loop_length))
      : 0;
}

void
RegionRenderer::handle_midi_region_range (
  const MidiRegion                                       &region,
  juce::MidiMessageSequence                              &events,
  std::pair<units::precise_tick_t, units::precise_tick_t> virtual_range,
  units::precise_tick_t                                   absolute_start)
{
  const auto virtual_range_length = virtual_range.second - virtual_range.first;
  const auto absolute_end = absolute_start + virtual_range_length;

  // Add notes for this loop segment
  for (
    const auto * note :
    region.get_children_view () | std::views::filter ([] (const auto * n) {
      // Only check unmuted notes
      return !n->mute ()->muted ();
    }))
    {
      const auto note_virtual_start = units::ticks (note->position ()->ticks ());
      const auto note_virtual_end =
        note_virtual_start + units::ticks (note->bounds ()->length ()->ticks ());

      // Only include notes that fall within the loop range
      if (
        note_virtual_start >= virtual_range.second
        || note_virtual_end <= virtual_range.first)
        continue;

      // Calculate position in current loop segment
      const auto note_absolute_start = std::max (
        absolute_start,
        absolute_start + (note_virtual_start - virtual_range.first));
      const auto note_absolute_end = std::min (
        absolute_end, absolute_start + (note_virtual_end - virtual_range.first));

      // Add note on and note off events
      events.addEvent (
        juce::MidiMessage::noteOn (
          1, note->pitch (), static_cast<std::uint8_t> (note->velocity ())),
        note_absolute_start.in (units::ticks));
      events.addEvent (
        juce::MidiMessage::noteOff (
          1, note->pitch (), static_cast<std::uint8_t> (note->velocity ())),
        note_absolute_end.in (units::ticks));
    }
}

void
RegionRenderer::handle_audio_region_range (
  const AudioRegion                                      &region,
  juce::AudioSampleBuffer                                &buffer,
  std::pair<units::precise_tick_t, units::precise_tick_t> virtual_range,
  units::precise_tick_t                                   absolute_start)
{
  const auto virtual_range_length = virtual_range.second - virtual_range.first;
  const auto absolute_end = absolute_start + virtual_range_length;

  // Get the audio source from the region
  auto &audio_source = region.get_audio_source ();

  // Convert tick positions to sample positions
  const auto &tempo_map = region.get_tempo_map ();
  const auto  absolute_start_samples =
    tempo_map.tick_to_samples_rounded (absolute_start);
  const auto absolute_end_samples =
    tempo_map.tick_to_samples_rounded (absolute_end);
  const auto segment_length_samples =
    absolute_end_samples - absolute_start_samples;

  assert (segment_length_samples > units::samples (0));

  // Create a temporary buffer for this segment
  const auto segment_length_samples_int =
    static_cast<int> (segment_length_samples.in (units::samples));
  juce::AudioSampleBuffer temp_buffer (
    buffer.getNumChannels (), segment_length_samples_int);
  temp_buffer.clear ();

  // Calculate the read position in the audio source
  const auto virtual_start_samples =
    tempo_map.tick_to_samples_rounded (virtual_range.first);

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
  const auto absolute_start_samples_int =
    static_cast<int> (absolute_start_samples.in (units::samples));

  // Ensure the output buffer is large enough
  if (
    absolute_start_samples_int + segment_length_samples_int
    > buffer.getNumSamples ())
    {
      buffer.setSize (
        buffer.getNumChannels (),
        absolute_start_samples_int + segment_length_samples_int, true, true,
        false);
    }

  // Mix the temporary buffer into the output buffer
  for (int channel = 0; channel < buffer.getNumChannels (); ++channel)
    {
      buffer.addFrom (
        channel, absolute_start_samples_int, temp_buffer, channel, 0,
        segment_length_samples_int);
    }
}

void
RegionRenderer::handle_chord_region_range (
  const ChordRegion                                      &region,
  juce::MidiMessageSequence                              &events,
  std::pair<units::precise_tick_t, units::precise_tick_t> virtual_range,
  units::precise_tick_t                                   absolute_start)
{
  const auto virtual_range_length = virtual_range.second - virtual_range.first;
  const auto absolute_end = absolute_start + virtual_range_length;

  // Add chord objects for this loop segment
  for (
    const auto * chord_object :
    region.get_children_view () | std::views::filter ([] (const auto * co) {
      // Only check unmuted chord objects
      return !co->mute ()->muted ();
    }))
    {
      const auto chord_object_virtual_start =
        units::ticks (chord_object->position ()->ticks ());

      // Only include objects that fall within the loop range
      if (
        chord_object_virtual_start >= virtual_range.second
        || chord_object_virtual_start < virtual_range.first)
        continue;

      // Calculate position in current loop segment
      const auto chord_object_absolute_start = std::max (
        absolute_start,
        absolute_start + (chord_object_virtual_start - virtual_range.first));

      // Get the chord descriptor from the chord object
      // TODO: Implement a proper way to get the chord descriptor
      // For now, create a simple C major chord as an example
      dsp::ChordDescriptor chord_descr (
        dsp::MusicalNote::C, false, dsp::MusicalNote::C, dsp::ChordType::Major,
        dsp::ChordAccent::None, 0);
      chord_descr.update_notes ();

      // Add note on events for all notes in the chord
      for (size_t i = 0; i < dsp::ChordDescriptor::MAX_NOTES; ++i)
        {
          if (chord_descr.notes_.at (i))
            {
              const auto note = static_cast<midi_byte_t> (36 + i); // C2 + offset
              events.addEvent (
                juce::MidiMessage::noteOn (1, note, MidiNote::DEFAULT_VELOCITY),
                chord_object_absolute_start.in (units::ticks));
              events.addEvent (
                juce::MidiMessage::noteOff (1, note, MidiNote::DEFAULT_VELOCITY),
                absolute_end.in (units::ticks) -1.0);
            }
        }
    }
}

void
RegionRenderer::handle_automation_region_range (
  const AutomationRegion                                 &region,
  std::vector<float>                                     &values,
  std::pair<units::precise_tick_t, units::precise_tick_t> virtual_range,
  units::precise_tick_t                                   absolute_start)
{
  const auto virtual_range_length = virtual_range.second - virtual_range.first;
  const auto absolute_end = absolute_start + virtual_range_length;

  const auto get_ap_virtual_position = [] (const auto * given_ap) {
    assert (given_ap != nullptr);
    return units::ticks (given_ap->position ()->ticks ());
  };

  const auto &tempo_map = region.get_tempo_map ();

  // Helper lambda to interpolate values between two automation points
  const auto interpolate_values =
    [&] (
      const AutomationPoint &prev_point, const AutomationPoint &next_point,
      units::precise_tick_t prev_point_absolute_start,
      std::pair<units::precise_tick_t, units::precise_tick_t>
        range_in_output_buffer) {
      const auto start_idx = static_cast<size_t> (
        tempo_map.tick_to_samples_rounded (range_in_output_buffer.first)
          .in (units::samples));
      if (start_idx >= values.size ())
        return;

      const auto distance_between_points = units::ticks (
        next_point.position ()->ticks () - prev_point.position ()->ticks ());
      const auto next_point_absolute_start =
        prev_point_absolute_start + distance_between_points;
      const auto distance_between_points_in_segment =
        std::min (next_point_absolute_start, range_in_output_buffer.second)
        - std::max (prev_point_absolute_start, range_in_output_buffer.first);

      const auto start_val = prev_point.value ();
      const auto end_val = next_point.value ();
      const auto diff = end_val - start_val;
      const auto curve_up = start_val < end_val;

      // Calculate the offset from the previous point to the start position
      const auto start_offset_ratio =
        (range_in_output_buffer.first - prev_point_absolute_start)
          .as<float> (units::ticks)
        / distance_between_points.as<float> (units::ticks);
      assert (start_offset_ratio >= 0.f && start_offset_ratio <= 1.f);

      // Calculate the segment length in samples
      const auto segment_length_samples = static_cast<size_t> (
        (tempo_map.tick_to_samples_rounded (range_in_output_buffer.second)
         - tempo_map.tick_to_samples_rounded (range_in_output_buffer.first))
          .in (units::samples));

      // And the distance between the points in samples so we can interpolate
      // correctly
      const auto sample_distance_between_points =
        tempo_map.tick_to_samples (distance_between_points);
      [[maybe_unused]] const auto sample_distance_between_points_in_segment =
        tempo_map.tick_to_samples (distance_between_points_in_segment);

      if (
        segment_length_samples > 0 && distance_between_points > units::ticks (0)
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
                  const auto curve_val = region.get_normalized_value_in_curve (
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
  auto automation_points = region.get_sorted_children_view ();
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

      // Calculate absolute position for this automation point
      const auto ap_absolute_start =
        absolute_start + (ap_virtual_start - virtual_range.first);

      // Handle interpolation from this point to next point
      if (next_ap != nullptr)
        {
          const auto next_ap_virtual_start = get_ap_virtual_position (next_ap);

          // If next point is within this virtual range, interpolate to it
          if (
            next_ap_virtual_start > virtual_range.first
            && next_ap_virtual_start <= virtual_range.second)
            {
              const auto next_ap_absolute_start =
                absolute_start + (next_ap_virtual_start - virtual_range.first);

              interpolate_values (
                *ap, *next_ap, ap_absolute_start,
                std::make_pair (
                  std::max (ap_absolute_start, absolute_start),
                  next_ap_absolute_start));
            }
          // If next point is beyond this virtual range, interpolate to range end
          else if (next_ap_virtual_start > virtual_range.second)
            {
              interpolate_values (
                *ap, *next_ap, ap_absolute_start,
                std::make_pair (
                  std::max (ap_absolute_start, absolute_start), absolute_end));
            }
        }
      else
        {
          // This is the last point, fill remaining samples with its value
          const auto start_idx = static_cast<size_t> (
            tempo_map.tick_to_samples_rounded (ap_absolute_start)
              .in (units::samples));
          const auto end_idx = static_cast<size_t> (
            tempo_map.tick_to_samples_rounded (absolute_end).in (units::samples));

          if (start_idx < values.size () && end_idx > start_idx)
            {
              utils::float_ranges::fill (
                &values[start_idx], ap->value (), end_idx - start_idx);
            }
        }
    }
}

void
RegionRenderer::serialize_to_sequence (
  const MidiRegion                        &region,
  juce::MidiMessageSequence               &events,
  std::optional<std::pair<double, double>> timeline_range_ticks)
{
  serialize_region (region, events, timeline_range_ticks);
}

void
RegionRenderer::serialize_to_sequence (
  const ChordRegion                       &region,
  juce::MidiMessageSequence               &events,
  std::optional<std::pair<double, double>> timeline_range_ticks)
{
  serialize_region (region, events, timeline_range_ticks);
}

/**
 * Serializes an Automation region to sample-accurate automation values.
 */
void
RegionRenderer::serialize_to_automation_values (
  const AutomationRegion                  &region,
  std::vector<float>                      &values,
  std::optional<std::pair<double, double>> timeline_range_ticks)
{
  const auto start_opt =
    timeline_range_ticks
      ? std::make_optional (units::ticks (timeline_range_ticks->first))
      : std::nullopt;
  const auto end_opt =
    timeline_range_ticks
      ? std::make_optional (units::ticks (timeline_range_ticks->second))
      : std::nullopt;
  const LoopParameters loop_params (region);
  const auto          &tempo_map = region.get_tempo_map ();

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "serialize_automation_region: start={}, end={}, values_size={}",
        start_opt ? *start_opt : units::ticks (-1.0),
        end_opt ? *end_opt : units::ticks (-1.0), values.size ());
    }

  // Always use the full region length for the output buffer
  const auto region_length_samples =
    tempo_map.tick_to_samples_rounded (loop_params.region_length);

  // Resize the output buffer to the full region length
  values.resize (
    static_cast<size_t> (region_length_samples.in (units::samples)));

  // Initialize with default value (-1.0)
  utils::float_ranges::fill (values.data (), -1, values.size ());

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "Processing automation region with loop_start={}, loop_end={}, region_length={}",
        loop_params.loop_start, loop_params.loop_end, loop_params.region_length);
    }

  // Use the serialize_region template to handle the looping and automation data
  serialize_region (region, values, timeline_range_ticks);

  // Apply constraints in a second pass
  if (start_opt || end_opt)
    {
      const auto region_start = units::ticks (region.position ()->ticks ());
      const auto constraint_start = start_opt ? *start_opt : units::ticks (0.0);
      const auto constraint_end =
        end_opt ? *end_opt : units::ticks (std::numeric_limits<double>::max ());

      if constexpr (REGION_SERIALIZER_DEBUG)
        {
          z_debug (
            "Applying constraints: region_start={}, constraint_start={}, constraint_end={}",
            region_start, constraint_start, constraint_end);
        }

      // Calculate the sample range that corresponds to the constraint range
      const auto constraint_start_offset =
        std::max (units::ticks (0.0), constraint_start - region_start);
      const auto constraint_end_offset =
        std::max (units::ticks (0.0), constraint_end - region_start);

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

      if (REGION_SERIALIZER_DEBUG)
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

      if constexpr (REGION_SERIALIZER_DEBUG)
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
 * Serializes an Audio region to an audio sample buffer.
 *
 * Audio regions are always serialized as they would be played in the timeline
 * (with loops and clip start).
 */
void
RegionRenderer::serialize_to_buffer (
  const AudioRegion                       &region,
  juce::AudioSampleBuffer                 &buffer,
  std::optional<std::pair<double, double>> timeline_range_ticks)
{
  const auto start_opt =
    timeline_range_ticks
      ? std::make_optional (units::ticks (timeline_range_ticks->first))
      : std::nullopt;
  const auto end_opt =
    timeline_range_ticks
      ? std::make_optional (units::ticks (timeline_range_ticks->second))
      : std::nullopt;
  const LoopParameters loop_params (region);

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "serialize_audio_region: start={}, end={}, builtin_fade_frames={}",
        start_opt ? *start_opt : units::ticks (-1.0),
        end_opt ? *end_opt : units::ticks (-1.0),
        AudioRegion::BUILTIN_FADE_FRAMES);
      z_debug (
        "Loop params: loop_start={}, loop_end={}, clip_start={}, region_length={}, num_loops={}",
        loop_params.loop_start, loop_params.loop_end, loop_params.clip_start,
        loop_params.region_length, loop_params.num_loops);
      z_debug (
        "Region position: {} ticks, {} samples", region.position ()->ticks (),
        region.get_tempo_map ().tick_to_samples (
          units::ticks (region.position ()->ticks ())));
    }

  // Calculate the total number of samples needed
  units::precise_tick_t total_length_ticks;
  if (start_opt || end_opt)
    {
      // When constraints are used, we need to check if there's any overlap
      // with the region before creating the buffer
      const auto region_start = units::ticks (region.position ()->ticks ());
      const auto region_end = region_start + loop_params.region_length;
      const auto constraint_start = start_opt ? *start_opt : units::ticks (0.0);
      const auto constraint_end =
        end_opt ? *end_opt : units::ticks (std::numeric_limits<double>::max ());

      // Check if there's any overlap
      if (region_end <= constraint_start || region_start >= constraint_end)
        {
          // No overlap - create empty buffer
          buffer.setSize (2, 0);
          buffer.clear ();
          return;
        }

      // When constraints are used, the buffer should represent the full
      // constraint range, with silence where there's no overlap
      total_length_ticks = constraint_end - constraint_start;
    }
  else
    {
      total_length_ticks = loop_params.region_length;
    }

  const auto total_length_samples =
    region.get_tempo_map ().tick_to_samples (total_length_ticks);

  // Prepare the buffer - no leading silence for audio regions
  buffer.setSize (
    2, static_cast<int> (total_length_samples.in (units::samples)));
  buffer.clear ();

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "Buffer setup: total_length_ticks={}, total_length_samples={}",
        total_length_ticks, total_length_samples);
    }

  // Use the serialize_region template to handle the looping and audio data
  serialize_region (region, buffer, timeline_range_ticks);

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "After serialize_region: first sample L={}, R={}, last sample L={}, R={}",
        buffer.getSample (0, 0), buffer.getSample (1, 0),
        buffer.getNumSamples () > 0
          ? buffer.getSample (0, buffer.getNumSamples () - 1)
          : 0.0f,
        buffer.getNumSamples () > 0
          ? buffer.getSample (1, buffer.getNumSamples () - 1)
          : 0.0f);
    }

  // Second pass: apply gain to the entire buffer
  apply_gain_pass (region, buffer);

  if constexpr (REGION_SERIALIZER_DEBUG)
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

  // Third pass: apply region fades (object fades)
  apply_region_fades_pass (region, buffer);

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "After region fades pass: first sample L={}, R={}, last sample L={}, R={}",
        buffer.getSample (0, 0), buffer.getSample (1, 0),
        buffer.getNumSamples () > 0
          ? buffer.getSample (0, buffer.getNumSamples () - 1)
          : 0.0f,
        buffer.getNumSamples () > 0
          ? buffer.getSample (1, buffer.getNumSamples () - 1)
          : 0.0f);
    }

  // Fourth pass: apply built-in fades
  apply_builtin_fades_pass (region, buffer, AudioRegion::BUILTIN_FADE_FRAMES);

  if constexpr (REGION_SERIALIZER_DEBUG)
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
RegionRenderer::apply_gain_pass (
  const AudioRegion       &region,
  juce::AudioSampleBuffer &buffer)
{
  const auto current_gain = region.gain ();

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug ("apply_gain_pass: gain={}", current_gain);
    }

  if (!utils::math::floats_equal (current_gain, 1.f))
    {
      buffer.applyGain (current_gain);

      if constexpr (REGION_SERIALIZER_DEBUG)
        {
          z_debug (
            "Applied gain {}: first sample L={}, R={}", current_gain,
            buffer.getSample (0, 0), buffer.getSample (1, 0));
        }
    }
  else
    {
      if constexpr (REGION_SERIALIZER_DEBUG)
        {
          z_debug ("Gain is 1.0, skipping gain pass");
        }
    }
}

/**
 * Applies region (object) fades to the audio buffer as a separate pass.
 */
void
RegionRenderer::apply_region_fades_pass (
  const AudioRegion       &region,
  juce::AudioSampleBuffer &buffer)
{
  const auto region_length_in_frames = region.bounds ()->length ()->samples ();
  const auto fade_in_pos_in_frames =
    region.fadeRange ()->startOffset ()->samples ();
  const auto fade_out_pos_in_frames =
    region_length_in_frames - region.fadeRange ()->endOffset ()->samples ();
  const auto num_frames_in_fade_in_area = fade_in_pos_in_frames;
  const auto num_frames_in_fade_out_area =
    region.fadeRange ()->endOffset ()->samples ();

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "apply_region_fades_pass: buffer_size={}, region_length={}, fade_in_pos={}, fade_out_pos={}",
        buffer.getNumSamples (), region_length_in_frames, fade_in_pos_in_frames,
        fade_out_pos_in_frames);
    }

  auto * left_channel = buffer.getWritePointer (0);
  auto * right_channel = buffer.getWritePointer (1);

  for (int j = 0; j < buffer.getNumSamples (); ++j)
    {
      // For audio regions, buffer frame directly corresponds to region frame
      // (no leading silence)
      const auto region_local_frame = j;

      // Skip frames that are beyond the region length
      if (region_local_frame >= region_length_in_frames)
        {
          if (REGION_SERIALIZER_DEBUG && j < 5)
            {
              z_debug (
                "Skipping region fade at frame {}: beyond region length {}", j,
                region_length_in_frames);
            }
          continue;
        }

      bool applied_fade = false;

      // If inside object fade in
      if (
        region_local_frame >= 0
        && region_local_frame < num_frames_in_fade_in_area)
        {
          // Ensure we don't divide by zero
          if (num_frames_in_fade_in_area > 0)
            {
              auto fade_in = static_cast<
                float> (region.fadeRange ()->get_normalized_y_for_fade (
                static_cast<double> (region_local_frame)
                  / static_cast<double> (num_frames_in_fade_in_area),
                true));

              left_channel[j] *= fade_in;
              right_channel[j] *= fade_in;
              applied_fade = true;

              if (REGION_SERIALIZER_DEBUG && j < 5)
                {
                  z_debug (
                    "Applied object fade in at frame {}: fade={}, L={}, R={}",
                    region_local_frame, fade_in, left_channel[j],
                    right_channel[j]);
                }
            }
        }

      // If inside object fade out
      if (region_local_frame >= fade_out_pos_in_frames)
        {
          const auto num_frames_from_fade_out_start =
            region_local_frame - fade_out_pos_in_frames;
          if (
            num_frames_from_fade_out_start <= num_frames_in_fade_out_area
            && num_frames_in_fade_out_area > 0)
            {
              auto fade_out = static_cast<
                float> (region.fadeRange ()->get_normalized_y_for_fade (
                static_cast<double> (num_frames_from_fade_out_start)
                  / static_cast<double> (num_frames_in_fade_out_area),
                false));

              left_channel[j] *= fade_out;
              right_channel[j] *= fade_out;
              applied_fade = true;

              if (REGION_SERIALIZER_DEBUG && j < 5)
                {
                  z_debug (
                    "Applied object fade out at frame {}: fade={}, L={}, R={}",
                    region_local_frame, fade_out, left_channel[j],
                    right_channel[j]);
                }
            }
        }

      if (
        REGION_SERIALIZER_DEBUG && j < 5 && !applied_fade
        && region_local_frame >= 0
        && region_local_frame < region_length_in_frames)
        {
          z_debug (
            "No region fade applied at frame {}: L={}, R={}",
            region_local_frame, left_channel[j], right_channel[j]);
        }
    }
}

/**
 * Applies built-in fades to the audio buffer as a separate pass.
 */
void
RegionRenderer::apply_builtin_fades_pass (
  const AudioRegion       &region,
  juce::AudioSampleBuffer &buffer,
  int                      builtin_fade_frames)
{
  const auto buffer_size = buffer.getNumSamples ();

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "apply_builtin_fades_pass: buffer_size={}, builtin_fade_frames={}",
        buffer_size, builtin_fade_frames);
    }

  if (builtin_fade_frames <= 0 || buffer_size <= 0)
    {
      if constexpr (REGION_SERIALIZER_DEBUG)
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
      if constexpr (REGION_SERIALIZER_DEBUG)
        {
          z_debug (
            "Applying builtin fade in: {} samples (0.0 -> 1.0)", fade_in_length);
        }

      buffer.applyGainRamp (0, fade_in_length, 0.0f, 1.0f);

      if constexpr (REGION_SERIALIZER_DEBUG)
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

      if constexpr (REGION_SERIALIZER_DEBUG)
        {
          z_debug (
            "Applying builtin fade out: start={}, length={} samples (1.0 -> 0.0)",
            fade_out_start, fade_out_length);
        }

      buffer.applyGainRamp (fade_out_start, fade_out_length, 1.0f, 0.0f);

      if constexpr (REGION_SERIALIZER_DEBUG)
        {
          z_debug (
            "After builtin fade out: first fade sample L={}, R={}, last sample L={}, R={}",
            buffer.getSample (0, fade_out_start),
            buffer.getSample (1, fade_out_start),
            buffer.getSample (0, buffer_size - 1),
            buffer.getSample (1, buffer_size - 1));
        }
    }

  if constexpr (REGION_SERIALIZER_DEBUG)
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
