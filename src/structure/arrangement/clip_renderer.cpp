// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include <fmt/std.h>

#include "dsp/chord_descriptor.h"
#include "dsp/curve.h"
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

// Maps a source-content position to a timeline delta (relative to the clip
// start) within a loop segment. Shared by the MIDI, chord, and automation
// clip range handlers so the loop-projection formula lives in one place.
static dsp::TimelineTick
segment_content_to_timeline_delta (
  const dsp::ContentTimeWarp &warp,
  dsp::TimelineTick           clip_start_tl,
  const LoopSegment          &segment,
  dsp::ContentTick            ct)
{
  return warp.contentToTimeline (segment.abs_start + (ct - segment.virt_start))
         - clip_start_tl;
}

void
ClipRenderer::handle_midi_clip_range (
  const MidiClip            &clip,
  juce::MidiMessageSequence &events,
  const LoopSegment         &segment)
{
  const auto * warp = clip.contentWarp ();
  const auto   clip_start_tl = warp->contentToTimeline (dsp::ContentTick{});

  auto to_timeline = [&] (dsp::ContentTick ct) -> dsp::TimelineTick {
    return segment_content_to_timeline_delta (*warp, clip_start_tl, segment, ct);
  };

  const auto segment_start = to_timeline (segment.virt_start);
  const auto segment_end = to_timeline (segment.virt_end);

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
      if (note_v_start >= segment.virt_end || note_v_end <= segment.virt_start)
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
          return start >= segment.virt_start && start < segment.virt_end;
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
ClipRenderer::handle_chord_clip_range (
  const ChordClip           &clip,
  juce::MidiMessageSequence &events,
  const LoopSegment         &segment)
{
  const auto * warp = clip.contentWarp ();
  const auto   clip_start_tl = warp->contentToTimeline (dsp::ContentTick{});

  auto to_timeline = [&] (dsp::ContentTick ct) -> dsp::TimelineTick {
    return segment_content_to_timeline_delta (*warp, clip_start_tl, segment, ct);
  };

  const auto segment_end = to_timeline (segment.virt_end);

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
        chord_v_start >= segment.virt_end || chord_v_start < segment.virt_start)
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

namespace
{

/// Result of evaluating the automation curve at a virtual tick position.
struct EvalResult
{
  float                   value;
  const AutomationPoint * driving_ap; // nullptr if before first AP
  const AutomationPoint * next_ap;    // nullptr if at/after last AP
};

/// Evaluates the automation value at a virtual (content) tick position and
/// returns the value, the AutomationPoint whose curve drives the segment, and
/// the next AP that defines the end of the curve domain.
EvalResult
eval_at_virt_tick (const auto &sorted_points, dsp::ContentTick virt_tick)
{
  if (sorted_points.empty ())
    return { 0.0f, nullptr, nullptr };

  const auto tick_of = [] (const AutomationPoint * ap) {
    return ap->position ()->asTick ();
  };

  const auto first_tick = tick_of (sorted_points.front ());
  const auto last_tick = tick_of (sorted_points.back ());

  // Before the first automation point: no automation applies.
  if (virt_tick < first_tick)
    return { sorted_points.front ()->value (), nullptr, nullptr };

  // At or after the last AP: hold last value, no next AP.
  if (virt_tick >= last_tick)
    return { sorted_points.back ()->value (), sorted_points.back (), nullptr };

  // lower_bound: first AP whose tick >= virt_tick.
  // Guaranteed valid (not begin, not end) because we excluded < first and >=
  // last.
  auto it = std::ranges::lower_bound (sorted_points, virt_tick, {}, tick_of);

  const auto * next_ap = *it;
  const auto   next_tick = tick_of (next_ap);

  // Exact match on an AP — return it directly so the boundary point carries
  // the same curve params as the AP (avoids sort-order ambiguity).
  if (next_tick == virt_tick)
    {
      auto         after_it = it;
      const auto * after =
        (++after_it != sorted_points.end ()) ? *after_it : nullptr;
      return { next_ap->value (), next_ap, after };
    }

  // Interpolation between prev_ap and next_ap.
  const auto * prev_ap = *std::ranges::prev (it);
  const auto   prev_tick = tick_of (prev_ap);
  if (next_tick <= prev_tick)
    return { prev_ap->value (), prev_ap, next_ap };

  const double tick_span = (next_tick - prev_tick).asDouble ();
  const double t =
    std::clamp ((virt_tick - prev_tick).asDouble () / tick_span, 0.0, 1.0);

  const auto val = dsp::evaluate_curve (
    prev_ap->value (), next_ap->value (), prev_ap->curveOpts ()->algorithm (),
    static_cast<float> (prev_ap->curveOpts ()->curviness ()), t);
  return { val, prev_ap, next_ap };
}

} // anonymous namespace

void
ClipRenderer::handle_automation_clip_range (
  const AutomationClip                 &clip,
  std::vector<RenderedAutomationPoint> &points,
  const LoopSegment                    &segment)
{
  const auto * warp = clip.contentWarp ();
  const auto   clip_start_tl = warp->contentToTimeline (dsp::ContentTick{});

  auto to_timeline = [&] (dsp::ContentTick ct) -> dsp::TimelineTick {
    return segment_content_to_timeline_delta (*warp, clip_start_tl, segment, ct);
  };

  // Helper: build a RenderedAutomationPoint from AP + curve-domain info.
  auto make_point =
    [&] (
      const AutomationPoint * ap, const AutomationPoint * next_ap,
      dsp::TimelineTick position, float value) {
      const auto origin_start = to_timeline (ap->position ()->asTick ());
      return RenderedAutomationPoint{
        position,
        value,
        ap->curveOpts ()->algorithm (),
        static_cast<float> (ap->curveOpts ()->curviness ()),
        origin_start,
        next_ap != nullptr
          ? to_timeline (next_ap->position ()->asTick ())
          : origin_start,
        ap->value (),
        next_ap != nullptr ? next_ap->value () : ap->value (),
      };
    };

  const auto sorted_aps = clip.get_sorted_children_view ();
  const auto vs = segment.virt_start;
  const auto ve = segment.virt_end;

  // Boundary point at segment start — skip when an AP already sits at vs
  // (the AP loop below will emit it with identical params).
  {
    auto [val, driving_ap, next_ap] = eval_at_virt_tick (sorted_aps, vs);
    if (driving_ap != nullptr && driving_ap->position ()->asTick () != vs)
      points.push_back (make_point (driving_ap, next_ap, to_timeline (vs), val));
  }

  // Automation points within this loop segment.
  for (auto ap_it = sorted_aps.begin (); ap_it != sorted_aps.end (); ++ap_it)
    {
      const auto * ap = *ap_it;
      const auto   ap_virt = ap->position ()->asTick ();
      if (ap_virt < vs || ap_virt > ve)
        continue;

      auto         next_it = ap_it;
      const auto * next_ap =
        (++next_it != sorted_aps.end ()) ? *next_it : nullptr;

      points.push_back (
        make_point (ap, next_ap, to_timeline (ap_virt), ap->value ()));
    }

  // Boundary point at segment end — gives the RT reader a curve endpoint
  // when the next AP lies beyond the segment boundary.
  {
    auto [val, driving_ap, next_ap] = eval_at_virt_tick (sorted_aps, ve);
    if (
      driving_ap != nullptr && next_ap != nullptr
      && driving_ap->position ()->asTick () != ve)
      points.push_back (make_point (driving_ap, next_ap, to_timeline (ve), val));
  }
}

void
ClipRenderer::serialize_to_points (
  const AutomationClip                 &clip,
  std::vector<RenderedAutomationPoint> &points,
  std::optional<TimelineRange>          timeline_range_ticks)
{
  serialize_clip (clip, points, timeline_range_ticks);
  std::ranges::sort (points, {}, &RenderedAutomationPoint::position);
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

  // Transform entries are already folded into the content window (in
  // source sample space, before loop expansion), so ranged entries
  // repeat in every loop iteration.
  const juce::AudioSampleBuffer * read_buffer = &samples;
  utils::audio::AudioBuffer       entry_window;
  if (!clip.transform_entries ().empty ())
    {
      entry_window = AudioFunction::audio_clip_content_window (clip);
      read_buffer = &entry_window;
    }
  const auto readable_frames = units::samples (read_buffer->getNumSamples ());

  const auto source_bpm = fs.source_bpm ();
  const auto sr = clip.get_tempo_map ().get_sample_rate ();
  const auto effective_bpm =
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
    native_offset (clip.clipStartPosition ()), units::samples (0),
    readable_frames);
  const auto loop_start_s = clamp (
    native_offset (clip.loopStartPosition ()), units::samples (0),
    readable_frames);
  const auto loop_end_raw = clamp (
    native_offset (clip.loopEndPosition ()), units::samples (0),
    readable_frames);
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
      const auto this_end = min (
        min (loop_end_s, readable_frames), read_pos + (out_len - write_pos));
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
          c, write_pos.in<int> (units::samples), *read_buffer, c,
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
