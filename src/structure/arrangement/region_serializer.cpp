// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/automation_region.h"
#include "structure/arrangement/chord_region.h"
#include "structure/arrangement/midi_region.h"
#include "structure/arrangement/region_serializer.h"

namespace zrythm::structure::arrangement
{

// Define to enable debug logging for region serializer
static constexpr bool REGION_SERIALIZER_DEBUG = false;

RegionSerializer::LoopParameters::LoopParameters (const ArrangerObject &region)
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
RegionSerializer::serialize_midi_region (
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
RegionSerializer::serialize_chord_region (
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
RegionSerializer::serialize_automation_region (
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
        start_opt ? start_opt->in (units::ticks) : -1.0,
        end_opt ? end_opt->in (units::ticks) : -1.0, values.size ());
    }

  // Get all automation points from the region
  const auto &automation_points = region.get_sorted_children_view ();

  // Always use the full region length for the output buffer
  const auto total_length_samples =
    tempo_map.tick_to_samples (loop_params.region_length);

  // Resize the output buffer to the full region length
  values.resize (static_cast<size_t> (total_length_samples.in (units::samples)));

  // Initialize with default value (-1.0)
  std::ranges::fill (values, -1.0f);

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "Processing {} automation points, loop_start={}, loop_end={}, region_length={}",
        automation_points.size (), loop_params.loop_start.in (units::ticks),
        loop_params.loop_end.in (units::ticks),
        loop_params.region_length.in (units::ticks));
    }

  // Process each automation point and render directly to the output buffer
  for (const auto * ap : automation_points)
    {
      const auto ap_pos = units::ticks (ap->position ()->ticks ());

      if constexpr (REGION_SERIALIZER_DEBUG)
        {
          z_debug (
            "Automation point at {} ticks (region starts at {})",
            ap_pos.in (units::ticks), region.position ()->ticks ());
        }

      // Determine if point should be written once or looped
      bool write_once = true;
      if (ap_pos >= loop_params.loop_start && ap_pos < loop_params.loop_end)
        write_once = false;

      // Generate values for each loop iteration
      const int max_loops = loop_params.num_loops;
      for (int loop_idx = 0; loop_idx <= max_loops; ++loop_idx)
        {
          // For non-looped points, only process first iteration
          if (write_once && loop_idx > 0)
            break;

          // Calculate point position for this loop iteration
          auto looped_ap_pos = ap_pos;

          // Adjust position for looped playback
          looped_ap_pos = get_looped_position (ap_pos, loop_params, loop_idx);

          // Skip if point is outside region bounds after adjustment
          if (
            looped_ap_pos < units::ticks (0.0)
            || looped_ap_pos >= loop_params.region_length)
            continue;

          // Convert to sample position (relative to region start)
          const auto sample_pos =
            tempo_map.tick_to_samples_rounded (looped_ap_pos);

          // Skip if outside the output buffer
          if (
            sample_pos < units::samples (0)
            || sample_pos
                 >= units::samples (static_cast<int64_t> (values.size ())))
            continue;

          // Get the next automation point for interpolation
          const auto * next_ap = region.get_next_ap (*ap, true);

          // Set the value at this point
          const auto start_idx = static_cast<size_t> (
            std::max (sample_pos.in (units::samples), int64_t{ 0 }));

          if constexpr (REGION_SERIALIZER_DEBUG)
            {
              z_debug (
                "Processing automation point at {} ticks, sample_idx={}, value={}, next_ap={}",
                ap_pos.in (units::ticks), start_idx, ap->value (),
                next_ap ? "exists" : "null");
            }

          if (start_idx < values.size ())
            {
              values[start_idx] = ap->value ();
              if constexpr (REGION_SERIALIZER_DEBUG)
                {
                  z_debug (
                    "  Set value at index {} to {}", start_idx, ap->value ());
                }
            }

          if (next_ap == nullptr)
            {
              // Last point, set all remaining samples to this value
              if (start_idx < values.size ())
                {
                  if constexpr (REGION_SERIALIZER_DEBUG)
                    {
                      z_debug (
                        "  Last point: filling from index {} to {} with value {}",
                        start_idx, values.size () - 1, ap->value ());
                    }
                  std::fill_n (
                    std::next (values.data (), start_idx),
                    values.size () - start_idx, ap->value ());
                }
              continue;
            }

          // Calculate the end position of this segment
          const auto next_ap_pos = units::ticks (next_ap->position ()->ticks ());
          auto looped_next_ap_pos =
            get_looped_position (next_ap_pos, loop_params, loop_idx);

          const auto next_sample_pos =
            tempo_map.tick_to_samples_rounded (looped_next_ap_pos);

          // Fill the segment from this point to the next point
          const auto end_idx = static_cast<size_t> (std::min (
            next_sample_pos.in (units::samples),
            static_cast<int64_t> (values.size ())));

          if (start_idx + 1 >= values.size () || start_idx + 1 >= end_idx)
            continue;

          const auto segment_length = end_idx - (start_idx + 1);
          if (segment_length == 0)
            continue;

          // Interpolate values for this segment
          if constexpr (REGION_SERIALIZER_DEBUG)
            {
              z_debug (
                "  Interpolating from index {} to {} (segment_length={})",
                start_idx + 1, end_idx - 1, segment_length);
            }

          for (size_t i = 0; i < segment_length; ++i)
            {
              const auto sample_offset =
                static_cast<double> (i + 1)
                / static_cast<double> (segment_length + 1);
              const auto normalized_x = sample_offset;

              // Get the curve value
              const auto curve_val =
                region.get_normalized_value_in_curve (*ap, normalized_x);

              // Linearly interpolate between the two points
              const auto interpolated_val =
                ap->value () + (next_ap->value () - ap->value ()) * curve_val;

              const auto output_idx = start_idx + 1 + i;
              values[output_idx] = static_cast<float> (interpolated_val);

              if (REGION_SERIALIZER_DEBUG && output_idx < 5)
                {
                  z_debug (
                    "    Set interpolated value at index {} to {} (offset={})",
                    output_idx, interpolated_val, sample_offset);
                }
            }
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
            region_start.in (units::ticks), constraint_start.in (units::ticks),
            constraint_end.in (units::ticks));
        }

      // Calculate the sample range that corresponds to the constraint range
      const auto constraint_start_offset =
        std::max (units::ticks (0.0), constraint_start - region_start);
      const auto constraint_end_offset =
        std::max (units::ticks (0.0), constraint_end - region_start);

      const auto constraint_start_samples = static_cast<size_t> (
        tempo_map.tick_to_samples (constraint_start_offset).in (units::samples));
      const auto constraint_end_samples = static_cast<size_t> (
        tempo_map.tick_to_samples (constraint_end_offset).in (units::samples));

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
RegionSerializer::serialize_audio_region (
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
        start_opt ? start_opt->in (units::ticks) : -1.0,
        end_opt ? end_opt->in (units::ticks) : -1.0, builtin_fade_frames);
      z_debug (
        "Loop params: loop_start={}, loop_end={}, clip_start={}, region_length={}, num_loops={}",
        loop_params.loop_start.in (units::ticks),
        loop_params.loop_end.in (units::ticks),
        loop_params.clip_start.in (units::ticks),
        loop_params.region_length.in (units::ticks), loop_params.num_loops);
      z_debug (
        "Region position: {} ticks, {} samples", region.position ()->ticks (),
        region.get_tempo_map ()
          .tick_to_samples (units::ticks (region.position ()->ticks ()))
          .in (units::samples));
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
        total_length_ticks.in (units::ticks),
        total_length_samples.in (units::samples));
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
RegionSerializer::process_arranger_object (
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
          looped_obj_pos = get_looped_position (obj_pos, loop_params, loop_idx);
          looped_obj_end = get_looped_position (obj_end, loop_params, loop_idx);

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
RegionSerializer::process_midi_note (
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
RegionSerializer::process_chord_object (
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
RegionSerializer::process_audio_loop (
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
    get_looped_position (units::ticks (0.0), loop_params, loop_idx);
  auto loop_end_pos =
    get_looped_position (loop_params.region_length, loop_params, loop_idx);

  if constexpr (REGION_SERIALIZER_DEBUG)
    {
      z_debug (
        "process_audio_loop[{}]: loop_start_pos={}, loop_end_pos={}", loop_idx,
        loop_start_pos.in (units::ticks), loop_end_pos.in (units::ticks));
    }

  // Convert to sample positions
  const auto &tempo_map = region.get_tempo_map ();
  const auto  start_samples = tempo_map.tick_to_samples (loop_start_pos);
  const auto  end_samples = tempo_map.tick_to_samples (loop_end_pos);

  // Always adjust for clip start (as_played=true)
  const auto clip_start_samples =
    tempo_map.tick_to_samples (loop_params.clip_start);
  const auto adjusted_start_samples = start_samples + clip_start_samples;

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
            z_debug (
              "Loop iteration {} is outside time range, skipping (loop: {}-{}, constraint: {}-{})",
              loop_idx, global_start_pos.in (units::ticks),
              global_end_pos.in (units::ticks),
              constraint_start.in (units::ticks),
              constraint_end.in (units::ticks));
          return;
        }
    }

  // Calculate the actual range to process
  auto actual_start = adjusted_start_samples;
  auto actual_end = end_samples + clip_start_samples;

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
        region.get_tempo_map ().tick_to_samples (
          std::max (constraint_start - region_start_ticks, units::ticks (0.0)));
      const auto constraint_end_samples =
        region.get_tempo_map ().tick_to_samples (
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
  // Add the buffer offset to account for region start positioning
  // When constraints are used, adjust the offset to account for the range start
  units::precise_sample_t output_offset;
  if (start || end)
    {
      // When constraints are used, the output buffer starts at the constraint
      // start. We need to calculate where this loop iteration falls within
      // the constraint range.
      const auto constraint_start = start ? *start : units::ticks (0.0);
      const auto global_start =
        units::ticks (region.position ()->ticks ()) + loop_start_pos;

      // Calculate the overlap between the loop iteration and the constraint
      // range
      const auto loop_global_end =
        units::ticks (region.position ()->ticks ()) + loop_end_pos;
      const auto constraint_end =
        end ? *end : units::ticks (std::numeric_limits<double>::max ());

      // Find the actual start and end of the overlap
      const auto overlap_start = std::max (global_start, constraint_start);
      const auto overlap_end = std::min (loop_global_end, constraint_end);

      if (overlap_start < overlap_end)
        {
          // Calculate offset from constraint start to overlap start
          const auto offset_from_constraint_start =
            overlap_start - constraint_start;
          output_offset = region.get_tempo_map ().tick_to_samples (
            offset_from_constraint_start);
        }
      else
        {
          // No overlap, shouldn't happen since we checked above
          output_offset = units::samples (0);
        }
    }
  else
    {
      // When no constraints are used, we need to calculate where this loop
      // iteration should be placed in the output buffer based on the
      // sequential playback order

      // When clip start is used, we want to write the audio sequentially
      // from the beginning of the buffer, not at the clip start position
      if (loop_params.clip_start > units::ticks (0.0))
        {
          // With clip start, we need to write loop iterations sequentially
          // Calculate the position in the output buffer for this loop iteration
          units::precise_tick_t playback_pos = units::ticks (0.0);

          if (loop_idx == 0)
            {
              // First iteration: starts at 0 (we always write to buffer start)
              playback_pos = units::ticks (0.0);
            }
          else
            {
              // Subsequent iterations: calculate based on previous loops
              // First part: from clip start to loop end
              const auto first_part_length = std::min (
                loop_params.loop_end - loop_params.clip_start,
                loop_params.region_length);

              // Full loops between loop start and loop end
              const auto loop_length =
                loop_params.loop_end - loop_params.loop_start;
              const auto num_full_loops = std::max (0, loop_idx - 1);

              // Current loop position in output buffer
              playback_pos = first_part_length + (num_full_loops * loop_length);
            }

          output_offset = tempo_map.tick_to_samples (playback_pos);
        }
      else
        {
          // Without clip start, calculate normal loop position
          units::precise_tick_t playback_pos;
          if (loop_idx == 0)
            {
              playback_pos = units::ticks (0.0);
            }
          else
            {
              // Subsequent iterations: calculate position based on previous
              // loops
              const auto loop_length =
                loop_params.loop_end - loop_params.loop_start;
              const auto num_full_loops = std::max (0, loop_idx - 1);

              // Current loop position
              playback_pos =
                loop_params.loop_start + (num_full_loops * loop_length);
            }

          output_offset = tempo_map.tick_to_samples (playback_pos);
        }

      // Clamp to buffer size
      if (output_offset >= units::samples (buffer.getNumSamples ()))
        {
          if constexpr (REGION_SERIALIZER_DEBUG)
            {
              z_debug (
                "Loop iteration {}: output_offset {} exceeds buffer size {}, skipping",
                loop_idx, output_offset.in (units::samples),
                buffer.getNumSamples ());
            }
          return;
        }
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
  const auto read_pos = static_cast<int64_t> (actual_start.in (units::samples));
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
        actual_length.in (units::samples), temp_buffer.getSample (0, 0),
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
RegionSerializer::apply_gain_pass (
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
RegionSerializer::apply_region_fades_pass (
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
RegionSerializer::apply_builtin_fades_pass (
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

/**
 * Calculates the position of an event in looped playback.
 */
units::precise_tick_t
RegionSerializer::get_looped_position (
  units::precise_tick_t original_pos,
  const LoopParameters &loop_params,
  int                   loop_index)
{
  // Clamp to loop end
  auto adjusted_pos = std::min (original_pos, loop_params.loop_end);

  // Adjust for clip start offset
  if (original_pos < loop_params.clip_start)
    loop_index++;

  // Calculate loop offset
  const auto loop_offset =
    loop_params.loop_length * loop_index - loop_params.clip_start;

  // Apply loop offset and clamp to region bounds
  adjusted_pos += loop_offset;
  adjusted_pos = std::max (
    units::ticks (0.0), std::min (adjusted_pos, loop_params.region_length));

  return adjusted_pos;
}

/**
 * Checks if a position range falls within the specified constraints.
 */
bool
RegionSerializer::is_position_in_range (
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
RegionSerializer::is_midi_note_playable (
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
