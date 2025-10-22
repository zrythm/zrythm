// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/automation_region.h"
#include "structure/arrangement/chord_region.h"
#include "structure/arrangement/midi_region.h"
#include "structure/arrangement/region_renderer.h"

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
RegionRenderer::serialize_midi_region (
  const MidiRegion          &region,
  juce::MidiMessageSequence &events,
  std::optional<double>      start,
  std::optional<double>      end,
  bool                       add_region_start,
  bool                       as_played)
{
  const auto region_start_offset =
    add_region_start
      ? units::ticks (region.position ()->ticks ())
      : units::ticks (0.0);
  const auto start_opt =
    start ? std::make_optional (units::ticks (*start)) : std::nullopt;
  const auto end_opt =
    end ? std::make_optional (units::ticks (*end)) : std::nullopt;
  const LoopParameters loop_params (region);

  // Process each MIDI note in the region
  for (const auto * note : region.get_children_view ())
    {
      // Skip muted notes when processing full playback
      if (as_played && note->mute ()->muted ())
        continue;

      // For as_played, check if note is in playable part of region
      if (as_played && !is_midi_note_playable (*note, loop_params))
        continue;

      process_midi_note (
        *note, loop_params, region_start_offset, start_opt, end_opt, as_played,
        events);
    }
}

void
RegionRenderer::serialize_chord_region (
  const ChordRegion         &region,
  juce::MidiMessageSequence &events,
  std::optional<double>      start,
  std::optional<double>      end,
  bool                       add_region_start,
  bool                       as_played)
{
  const auto region_start_offset =
    add_region_start
      ? units::ticks (region.position ()->ticks ())
      : units::ticks (0.0);
  const auto start_opt =
    start ? std::make_optional (units::ticks (*start)) : std::nullopt;
  const auto end_opt =
    end ? std::make_optional (units::ticks (*end)) : std::nullopt;
  const LoopParameters loop_params (region);

  // Process each chord object in the region
  for (const auto * chord_obj : region.get_children_view ())
    {
      // Skip muted chord objects when processing full playback
      if (as_played && chord_obj->mute ()->muted ())
        continue;

      process_chord_object (
        *chord_obj, loop_params, region_start_offset, start_opt, end_opt,
        as_played, events);
    }
}

/**
 * Serializes an Automation region to sample-accurate automation values.
 */
void
RegionRenderer::serialize_automation_region (
  const AutomationRegion &region,
  std::vector<float>     &values,
  std::optional<double>   start,
  std::optional<double>   end)
{
  const auto start_opt =
    start ? std::make_optional (units::ticks (*start)) : std::nullopt;
  const auto end_opt =
    end ? std::make_optional (units::ticks (*end)) : std::nullopt;
  const LoopParameters loop_params (region);
  const auto          &tempo_map = region.get_tempo_map ();

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "serialize_automation_region: start={}, end={}, values_size={}",
        start_opt ? *start_opt : units::ticks (-1.0),
        end_opt ? *end_opt : units::ticks (-1.0), values.size ());
    }

  // Get all automation points from the region
  const auto &automation_points = region.get_sorted_children_view ();

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
        "Processing {} automation points, loop_start={}, loop_end={}, region_length={}",
        automation_points.size (), loop_params.loop_start, loop_params.loop_end,
        loop_params.region_length);
    }

  // Helper lambda to interpolate values between two automation points
  // using the curve's interpolation method and to fill the output buffer in the
  // given range.
  // prev_point_start_offset: Position of @p prev_point relative to region start
  // (after loops are applied, so could be different from
  // `prev_point->position()->ticks()`)
  const auto interpolate_values =
    [&] (
      const AutomationPoint &prev_point, const AutomationPoint &next_point,
      units::precise_tick_t prev_point_start_offset,
      std::pair<units::precise_tick_t, units::precise_tick_t>
        range_in_output_buffer) {
      const auto start_idx = static_cast<size_t> (
        tempo_map.tick_to_samples_rounded (range_in_output_buffer.first)
          .in (units::samples));
      if (start_idx >= values.size ())
        return;

      const auto prev_pos = prev_point_start_offset;
      const auto distance_between_points = units::ticks (
        next_point.position ()->ticks () - prev_point.position ()->ticks ());
      const auto next_pos = prev_pos + distance_between_points;

      if constexpr (REGION_SERIALIZER_DEBUG)
        {
          z_debug (
            "Interpolating: prev_pos={}, next_pos={}, range_in_output_buffer={}",
            prev_pos, next_pos, range_in_output_buffer);
        }

      const auto start_val = prev_point.value ();
      const auto end_val = next_point.value ();
      const auto diff = end_val - start_val;
      const auto curve_up = start_val < end_val;

      // Calculate the offset from the previous point to the start position
      const auto start_offset_ratio =
        (range_in_output_buffer.first - prev_pos).as<float> (units::ticks)
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

      if constexpr (REGION_SERIALIZER_DEBUG)
        {
          z_debug (
            "  start_offset_ratio={}, segment_length_samples={}",
            start_offset_ratio, segment_length_samples);
        }

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

  // Process each automation point and render directly to the output buffer
  for (const auto * ap : automation_points)
    {
      const auto ap_pos = units::ticks (ap->position ()->ticks ());

      if constexpr (REGION_SERIALIZER_DEBUG)
        {
          z_debug (
            "Automation point at {} (region starts at {})", ap_pos,
            region.position ()->ticks ());
        }

      // Get the next & prev automation point
      const auto * next_ap = region.get_next_ap (*ap, true);
      const auto * prev_ap = region.get_prev_ap (*ap);

      // Generate values for each loop iteration
      for (const auto loop_idx : std::views::iota (0, loop_params.num_loops))
        {
          // Calculate point positions (relative to region start) for this loop
          // iteration
          const auto looped_ap_pos_ticks =
            get_event_looped_position (ap_pos, loop_params, loop_idx);
          const auto looped_ap_pos_samples =
            tempo_map.tick_to_samples_rounded (looped_ap_pos_ticks);

          // Calculate this loop's bounds relative to region start (FIXME: these
          // don't take into account clip start?)
          const auto current_loop_start_ticks = get_event_looped_position (
            loop_idx == 0 ? units::ticks (0) : loop_params.loop_start,
            loop_params, loop_idx);
          [[maybe_unused]] const auto current_loop_start_samples =
            tempo_map.tick_to_samples_rounded (current_loop_start_ticks);
          const auto current_loop_end_ticks = get_event_looped_position (
            loop_params.loop_end, loop_params, loop_idx);
          const auto current_loop_end_samples =
            tempo_map.tick_to_samples_rounded (current_loop_end_ticks);

          // Skip if point is outside loop bounds after adjustment
          if (
            looped_ap_pos_ticks < current_loop_start_ticks
            || looped_ap_pos_ticks >= current_loop_end_ticks)
            continue;

          // First, handle special case for when there is a previous point
          // before this loop segment that should be used to interpolate up to
          // this point
          if (prev_ap != nullptr)
            {
              // Calculate the would-be looped position of the previous point
              const auto looped_prev_ap_pos_ticks = get_event_looped_position (
                units::ticks (prev_ap->position ()->ticks ()), loop_params,
                loop_idx);

              if (looped_prev_ap_pos_ticks < current_loop_start_ticks)
                {
                  // Interpolate values for this segment
                  interpolate_values (
                    *prev_ap, *ap, looped_prev_ap_pos_ticks,
                    std::make_pair (
                      current_loop_start_ticks, looped_ap_pos_ticks));
                }
            }

          if constexpr (REGION_SERIALIZER_DEBUG)
            {
              z_debug (
                "Processing automation point at {} ticks, looped_ap_pos_ticks={}, value={}, next_ap={}",
                ap_pos, looped_ap_pos_ticks, ap->value (),
                next_ap ? "exists" : "null");
            }

          if (next_ap == nullptr)
            {
              // Last point, set all remaining samples to this value
              if constexpr (REGION_SERIALIZER_DEBUG)
                {
                  z_debug (
                    "  Last point: filling from index {} to {} with value {}",
                    looped_ap_pos_ticks, current_loop_end_ticks, ap->value ());
                }
              utils::float_ranges::fill (
                &values[looped_ap_pos_samples.in (units::samples)], ap->value (),
                (current_loop_end_samples - looped_ap_pos_samples)
                  .in (units::samples));
              continue;
            }

          // Calculate the end position of this segment
          const auto looped_next_ap_pos_ticks = get_event_looped_position (
            units::ticks (next_ap->position ()->ticks ()), loop_params,
            loop_idx);
          [[maybe_unused]] const auto looped_next_ap_pos_samples =
            tempo_map.tick_to_samples_rounded (looped_next_ap_pos_ticks);

          // Fill the segment from this point to the next point
          const auto start_ticks = looped_ap_pos_ticks;
          auto       end_ticks = looped_next_ap_pos_ticks;

          // If the next point is beyond the current loop end and we're in a
          // looped region, we should interpolate to the loop end instead of
          // stopping at the next point
          if (end_ticks > current_loop_end_ticks)
            {
              end_ticks = current_loop_end_ticks;
            }

          if (start_ticks >= end_ticks)
            continue;

          // Interpolate values
          interpolate_values (
            *ap, *next_ap, looped_ap_pos_ticks,
            std::make_pair (looped_ap_pos_ticks, end_ticks));
        }
    }

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
RegionRenderer::serialize_audio_region (
  const AudioRegion       &region,
  juce::AudioSampleBuffer &buffer,
  std::optional<double>    start,
  std::optional<double>    end,
  int                      builtin_fade_frames)
{
  const auto start_opt =
    start ? std::make_optional (units::ticks (*start)) : std::nullopt;
  const auto end_opt =
    end ? std::make_optional (units::ticks (*end)) : std::nullopt;
  const LoopParameters loop_params (region);

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "serialize_audio_region: start={}, end={}, builtin_fade_frames={}",
        start_opt ? *start_opt : units::ticks (-1.0),
        end_opt ? *end_opt : units::ticks (-1.0), builtin_fade_frames);
      z_debug (
        "Loop params: loop_start={}, loop_end={}, clip_start={}, region_length={}, num_loops={}",
        loop_params.loop_start, loop_params.loop_end, loop_params.clip_start,
        loop_params.region_length, loop_params.num_loops);
      z_debug (
        "Region position: {} ticks, {} samples", region.position ()->ticks (),
        region.get_tempo_map ().tick_to_samples (
          units::ticks (region.position ()->ticks ())));
    }

  // Get the audio source from the region
  auto &audio_source = region.get_audio_source ();

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

  // Process each loop iteration - first pass: copy raw audio data
  for (int loop_idx = 0; loop_idx <= loop_params.num_loops; ++loop_idx)
    {
      if constexpr (REGION_SERIALIZER_DEBUG)
        z_debug ("Processing loop iteration {}", loop_idx);
      process_audio_loop (
        region, audio_source, loop_params, loop_idx, units::samples (0),
        start_opt, end_opt, buffer);
    }

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "After all loops: first sample L={}, R={}, last sample L={}, R={}",
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
  apply_region_fades_pass (region, buffer, loop_params);

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
  apply_builtin_fades_pass (region, buffer, builtin_fade_frames);

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
 * Processes a single arranger object across all loop iterations.
 *
 * @tparam ObjectType The type of arranger object (MidiNote or ChordObject)
 * @tparam EventGenerator Function that generates MIDI events for the object
 * @param obj The arranger object to process
 * @param loop_params Loop parameters for the region
 * @param region_start_offset Offset to add to positions for global timeline
 * @param start Optional start position constraint
 * @param end Optional end position constraint
 * @param as_played Whether to process as played (with loops)
 * @param events Output MIDI message sequence
 * @param event_generator Function that generates events for the object
 */
template <typename ObjectType, typename EventGenerator>
void
RegionRenderer::process_arranger_object (
  const ObjectType                    &obj,
  const LoopParameters                &loop_params,
  units::precise_tick_t                region_start_offset,
  std::optional<units::precise_tick_t> start,
  std::optional<units::precise_tick_t> end,
  bool                                 as_played,
  juce::MidiMessageSequence           &events,
  EventGenerator                       event_generator)
{
  const auto obj_pos = units::ticks (obj.position ()->ticks ());
  const auto obj_length = units::ticks (obj.bounds ()->length ()->ticks ());
  const auto obj_end = obj_pos + obj_length;

  // Determine if object should be written once or looped
  bool write_once = true;
  if (
    as_played && obj_pos >= loop_params.loop_start
    && obj_pos < loop_params.loop_end)
    write_once = false;

  // Generate events for each loop iteration
  const int max_loops = as_played ? loop_params.num_loops : 0;
  for (int loop_idx = 0; loop_idx <= max_loops; ++loop_idx)
    {
      // For non-looped objects, only process first iteration
      if (write_once && loop_idx > 0)
        break;

      // Calculate object positions for this loop iteration
      auto looped_obj_pos = obj_pos;
      auto looped_obj_end = obj_end;

      if (as_played)
        {
          // Adjust positions for looped playback
          looped_obj_pos =
            get_event_looped_position (obj_pos, loop_params, loop_idx);
          looped_obj_end =
            get_event_looped_position (obj_end, loop_params, loop_idx);

          // Skip if object is outside region bounds after adjustment
          if (
            looped_obj_pos < units::ticks (0.0)
            || looped_obj_pos >= loop_params.region_length)
            continue;
        }

      // Convert to global timeline positions
      const auto global_start_pos = looped_obj_pos + region_start_offset;
      const auto global_end_pos = looped_obj_end + region_start_offset;

      // Check if object is within the requested time range
      if (is_position_in_range (global_start_pos, global_end_pos, start, end))
        {
          // Adjust for start constraint
          const auto adjusted_start =
            start ? (global_start_pos - *start) : global_start_pos;
          const auto adjusted_end =
            start ? (global_end_pos - *start) : global_end_pos;

          // Generate events using the provided generator function
          event_generator (obj, adjusted_start, adjusted_end, events);
        }
    }
}

/**
 * Processes a single MIDI note across all loop iterations.
 */
void
RegionRenderer::process_midi_note (
  const MidiNote                      &note,
  const LoopParameters                &loop_params,
  units::precise_tick_t                region_start_offset,
  std::optional<units::precise_tick_t> start,
  std::optional<units::precise_tick_t> end,
  bool                                 as_played,
  juce::MidiMessageSequence           &events)
{
  process_arranger_object (
    note, loop_params, region_start_offset, start, end, as_played, events,
    [] (
      const MidiNote &midi_note, units::precise_tick_t start_pos,
      units::precise_tick_t end_pos, juce::MidiMessageSequence &evs) {
      // Add note on and note off events
      evs.addEvent (
        juce::MidiMessage::noteOn (
          1, midi_note.pitch (),
          static_cast<std::uint8_t> (midi_note.velocity ())),
        start_pos.in (units::ticks));
      evs.addEvent (
        juce::MidiMessage::noteOff (
          1, midi_note.pitch (),
          static_cast<std::uint8_t> (midi_note.velocity ())),
        end_pos.in (units::ticks));
    });
}

void
RegionRenderer::process_chord_object (
  const arrangement::ChordObject      &chord_obj,
  const LoopParameters                &loop_params,
  units::precise_tick_t                region_start_offset,
  std::optional<units::precise_tick_t> start,
  std::optional<units::precise_tick_t> end,
  bool                                 as_played,
  juce::MidiMessageSequence           &events)
{
  // For chord objects, we need to override the length since they don't have
  // a meaningful bounds length. Use a default duration of 1 beat.
  process_arranger_object (
    chord_obj, loop_params, region_start_offset, start, end, as_played, events,
    [] (
      const arrangement::ChordObject &chord_object,
      units::precise_tick_t start_pos, units::precise_tick_t end_pos,
      juce::MidiMessageSequence &evs) {
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
              evs.addEvent (
                juce::MidiMessage::noteOn (1, note, MidiNote::DEFAULT_VELOCITY),
                start_pos.in (units::ticks));
              evs.addEvent (
                juce::MidiMessage::noteOff (1, note, MidiNote::DEFAULT_VELOCITY),
                end_pos.in (units::ticks));
            }
        }
    });
}

/**
 * Processes a single audio loop iteration.
 *
 * Audio regions are always processed as they would be played in the timeline
 * (with loops and clip start).
 */
void
RegionRenderer::process_audio_loop (
  const AudioRegion                   &region,
  juce::PositionableAudioSource       &audio_source,
  const LoopParameters                &loop_params,
  int                                  loop_idx,
  units::precise_sample_t              buffer_offset_samples,
  std::optional<units::precise_tick_t> start,
  std::optional<units::precise_tick_t> end,
  juce::AudioSampleBuffer             &buffer)
{
  // Calculate the time range for this loop iteration
  // Always adjust for looped playback (as_played=true)
  auto loop_start_pos =
    get_event_looped_position (units::ticks (0.0), loop_params, loop_idx);
  auto loop_end_pos = get_event_looped_position (
    loop_params.region_length, loop_params, loop_idx);

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "process_audio_loop[{}]: loop_start_pos={}, loop_end_pos={}", loop_idx,
        loop_start_pos, loop_end_pos);
    }

  // Convert to sample positions
  const auto &tempo_map = region.get_tempo_map ();
  const auto start_samples = tempo_map.tick_to_samples_rounded (loop_start_pos);
  const auto end_samples = tempo_map.tick_to_samples_rounded (loop_end_pos);

  // Note: clip start is already accounted for in the loop position calculation
  // so we don't need to add it again here
  const auto adjusted_start_samples = start_samples;

  // Check if this loop iteration is within the requested time range
  const auto global_start_pos =
    loop_start_pos + units::ticks (region.position ()->ticks ());
  const auto global_end_pos =
    loop_end_pos + units::ticks (region.position ()->ticks ());

  // When constraints are used, we need to check if any part of this loop
  // overlaps with the requested range
  if (start || end)
    {
      // For constraints, we need to check if the loop iteration overlaps
      // with the constraint range, not if it's fully contained
      const auto constraint_start = start ? *start : units::ticks (0.0);
      const auto constraint_end =
        end ? *end : units::ticks (std::numeric_limits<double>::max ());

      // Check if there's any overlap between the loop iteration and the
      // constraint range
      const bool has_overlap = !(
        global_end_pos <= constraint_start
        || global_start_pos >= constraint_end);

      if (!has_overlap)
        {
          if constexpr (REGION_SERIALIZER_DEBUG)
            {
              z_debug (
                "Loop iteration {} is outside time range, skipping (loop: {}-{}, constraint: {}-{})",
                loop_idx, global_start_pos, global_end_pos, constraint_start,
                constraint_end);
            }
          return;
        }
    }

  // Calculate the actual range to process
  auto actual_start = adjusted_start_samples;
  auto actual_end = end_samples;

  // When constraints are used, we need to adjust the range to only include
  // the portion that overlaps with the constraint range
  if (start || end)
    {
      const auto constraint_start = start ? *start : units::ticks (0.0);
      const auto constraint_end =
        end ? *end : units::ticks (std::numeric_limits<double>::max ());

      // Convert constraint positions to samples relative to the region start
      const auto region_start_ticks =
        units::ticks (region.position ()->ticks ());
      const auto constraint_start_samples =
        region.get_tempo_map ().tick_to_samples_rounded (
          std::max (constraint_start - region_start_ticks, units::ticks (0.0)));
      const auto constraint_end_samples =
        region.get_tempo_map ().tick_to_samples_rounded (
          std::max (constraint_end - region_start_ticks, units::ticks (0.0)));

      // Adjust the actual range to only include the overlap
      actual_start = std::max (actual_start, constraint_start_samples);
      actual_end = std::min (actual_end, constraint_end_samples);
    }

  const auto actual_length = actual_end - actual_start;

  if (actual_length <= units::samples (0))
    {
      if constexpr (REGION_SERIALIZER_DEBUG)
        z_debug (
          "Loop iteration {} has non-positive length, skipping", loop_idx);
      return;
    }
  // Calculate the offset into the output buffer
  units::sample_t output_offset;

  if (start || end)
    {
      // With constraints: calculate offset from constraint start to loop start
      const auto constraint_start = start ? *start : units::ticks (0.0);
      const auto global_start =
        units::ticks (region.position ()->ticks ()) + loop_start_pos;
      const auto offset_from_constraint_start = global_start - constraint_start;
      output_offset =
        tempo_map.tick_to_samples_rounded (offset_from_constraint_start);
    }
  else
    {
      // Without constraints: calculate sequential position for this loop
      // iteration
      units::precise_tick_t playback_pos = units::ticks (0.0);

      if (loop_idx > 0)
        {
          const auto loop_length = loop_params.loop_end - loop_params.loop_start;
          const auto num_full_loops = std::max (0, loop_idx - 1);

          if (loop_params.clip_start > units::ticks (0.0))
            {
              // With clip start: position after initial part
              const auto first_part_length = std::min (
                loop_params.loop_end - loop_params.clip_start,
                loop_params.region_length);
              playback_pos = first_part_length + (num_full_loops * loop_length);
            }
          else
            {
              // Without clip start: normal loop positioning
              playback_pos =
                loop_params.loop_start + (num_full_loops * loop_length);
            }
        }

      output_offset = tempo_map.tick_to_samples_rounded (playback_pos);
    }

  // Skip if offset exceeds buffer size
  if (output_offset >= units::samples (buffer.getNumSamples ()))
    {
      if constexpr (REGION_SERIALIZER_DEBUG)
        {
          z_debug (
            "Loop iteration {}: output_offset {} exceeds buffer size {}, skipping",
            loop_idx, output_offset, buffer.getNumSamples ());
        }
      return;
    }

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "Loop iteration {}: actual_start={}, actual_end={}, actual_length={}, output_offset={}",
        loop_idx, actual_start.in (units::samples),
        actual_end.in (units::samples), actual_length.in (units::samples),
        output_offset.in (units::samples));
    }

  // Read audio data from the source
  // Check if we're reading beyond the audio source length
  const auto source_length = audio_source.getTotalLength ();
  const auto read_pos = actual_start.in (units::samples);
  const auto actual_length_int =
    static_cast<int> (actual_length.in (units::samples));

  // Create a temporary buffer for this loop iteration
  juce::AudioSampleBuffer temp_buffer (2, actual_length_int);
  temp_buffer.clear (); // Start with silence

  if (read_pos < source_length)
    {
      // We can read some data from the source
      const auto readable_length = std::min (
        actual_length_int, static_cast<int> (source_length - read_pos));

      audio_source.setNextReadPosition (read_pos);

      juce::AudioSourceChannelInfo source_ch_nfo{
        &temp_buffer, 0, readable_length
      };
      audio_source.getNextAudioBlock (source_ch_nfo);

      // The rest of the buffer (if any) remains silent (zeros)
    }
  // If read_pos >= source_length, the entire buffer remains silent

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "Read {} samples from audio source, first sample L={}, R={}",
        actual_length, temp_buffer.getSample (0, 0),
        temp_buffer.getSample (1, 0));
    }

  // Note: Processing (gain and fades) will be applied in separate passes
  // later For now, just copy the raw audio data

  // Ensure we don't exceed the output buffer bounds
  const auto buffer_size = units::samples (buffer.getNumSamples ());
  const auto output_offset_int =
    static_cast<int> (output_offset.in (units::samples));

  // Calculate how many samples we can actually write
  const auto samples_to_write = std::min (
    actual_length_int,
    static_cast<int> ((buffer_size - output_offset).in (units::samples)));

  if (samples_to_write > 0 && output_offset_int >= 0)
    {
      // Ensure buffer is large enough
      if (output_offset_int + samples_to_write > buffer.getNumSamples ())
        {
          buffer.setSize (
            buffer.getNumChannels (), output_offset_int + samples_to_write,
            false, true, false);
        }

      if constexpr (REGION_SERIALIZER_DEBUG)
        {
          z_debug (
            "Writing {} samples to output buffer at offset {}",
            samples_to_write, output_offset_int);
        }

      // Mix into the output buffer
      for (int channel = 0; channel < 2; ++channel)
        {
          buffer.addFrom (
            channel, output_offset_int, temp_buffer, channel, 0,
            samples_to_write);
        }

      if constexpr (REGION_SERIALIZER_DEBUG)
        {
          z_debug (
            "After writing: output buffer first sample L={}, R={}, last sample L={}, R={}",
            buffer.getSample (0, output_offset_int),
            buffer.getSample (1, output_offset_int),
            buffer.getSample (0, output_offset_int + samples_to_write - 1),
            buffer.getSample (1, output_offset_int + samples_to_write - 1));
          z_debug (
            "Output buffer status: first sample L={}, R={}, last sample L={}, R={}",
            buffer.getSample (0, 0), buffer.getSample (1, 0),
            buffer.getSample (0, buffer.getNumSamples () - 1),
            buffer.getSample (1, buffer.getNumSamples () - 1));
        }
    }
  else
    {
      if constexpr (REGION_SERIALIZER_DEBUG)
        {
          z_debug (
            "Skipping write: samples_to_write={}, output_offset_int={}",
            samples_to_write, output_offset_int);
        }
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
  juce::AudioSampleBuffer &buffer,
  const LoopParameters    &loop_params)
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

units::precise_tick_t
RegionRenderer::get_event_looped_position (
  units::precise_tick_t original_pos,
  const LoopParameters &loop_params,
  int                   loop_index)
{
  // Clamp to loop end
  auto adjusted_pos = std::min (original_pos, loop_params.loop_end);

  // Calculate loop offset
  const auto loop_offset =
    loop_params.loop_length * loop_index + loop_params.clip_start;

  // Apply loop offset and clamp to region bounds
  adjusted_pos += loop_offset;
  adjusted_pos = std::min (adjusted_pos, loop_params.region_length);

  return adjusted_pos;
}

/**
 * Checks if a position range falls within the specified constraints.
 */
bool
RegionRenderer::is_position_in_range (
  units::precise_tick_t                start_pos,
  units::precise_tick_t                end_pos,
  std::optional<units::precise_tick_t> range_start,
  std::optional<units::precise_tick_t> range_end)
{
  if (range_start && end_pos <= *range_start)
    return false;
  if (range_end && start_pos >= *range_end)
    return false;
  return true;
}

/**
 * Checks if a MIDI note is in a playable part of the region.
 */
bool
RegionRenderer::is_midi_note_playable (
  const MidiNote       &note,
  const LoopParameters &loop_params)
{
  const auto note_pos = units::ticks (note.position ()->ticks ());

  // Note is playable if it's in the loop range
  if (note_pos >= loop_params.loop_start && note_pos < loop_params.loop_end)
    return true;

  // Or if it's between clip start and loop start
  if (note_pos >= loop_params.clip_start && note_pos < loop_params.loop_start)
    return true;

  return false;
}

} // namespace zrythm::structure::arrangement
