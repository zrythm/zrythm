// SPDX-FileCopyrightText: © 2020, 2023-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>
#include <numeric>

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/midi_function.h"
#include "structure/arrangement/midi_note.h"
#include "utils/enum_utils.h"
#include "utils/logger.h"

namespace zrythm::structure::arrangement
{

namespace
{

/// Minimum length a note can have, in content ticks (applied by
/// sanitized()).
constexpr dsp::ContentTick kMinLengthTicks{ units::ticks (1.0) };

/// Gap before the next note for Portato.
constexpr dsp::ContentTick kPortatoGapTicks{ units::ticks (48.0) };

/// Note length for Staccato.
constexpr dsp::ContentTick kStaccatoLengthTicks{ units::ticks (48.0) };

/// Content-tick offset the given duration spans at the tempo under @p
/// note. The duration is measured on the timeline and mapped back into
/// the note's content space through the owning clip's warp, so offsets
/// stay correct in timestretched clips.
dsp::ContentTick
offset_ticks_at (const MidiNote &note, units::precise_second_t duration)
{
  const auto &tempo_map = note.get_tempo_map ();
  const auto  base = timeline_ticks (note);
  const auto  shifted =
    tempo_map.seconds_to_tick (tempo_map.tick_to_seconds (base) + duration);
  const auto * clip = qobject_cast<const Clip *> (note.parentObject ());
  if (clip == nullptr)
    {
      // Without an owning clip there is no content/timeline distinction:
      // the timeline offset doubles as the content offset
      return dsp::ContentTick{ units::ticks ((shifted - base).asDouble ()) };
    }
  const auto * warp = clip->contentWarp ();
  return warp->timelineToContent (shifted) - warp->timelineToContent (base);
}

/// Clamps a state to the ranges the note setters enforce — positions are
/// bounded by the clip start — so that writing the state reproduces it
/// exactly.
MidiFunction::NoteState
sanitized (MidiFunction::NoteState state)
{
  state.position_ticks_ = std::max (state.position_ticks_, dsp::ContentTick{});
  state.length_ticks_ = std::max (state.length_ticks_, kMinLengthTicks);
  state.pitch_ = std::clamp (state.pitch_, 0, 127);
  state.velocity_ = std::clamp (state.velocity_, 0, 127);
  return state;
}

/// Captures the current state of each note.
std::vector<MidiFunction::NoteState>
capture_states (std::span<MidiNote * const> notes)
{
  std::vector<MidiFunction::NoteState> states;
  states.reserve (notes.size ());
  for (const auto * mn : notes)
    {
      states.push_back (
        { mn->position ()->asTick (), mn->length ()->asTick (), mn->pitch (),
          mn->velocity () });
    }
  return states;
}

/// Indices of the notes sorted by position (ascending).
std::vector<std::size_t>
by_position (std::span<MidiNote * const> notes)
{
  std::vector<std::size_t> indices (notes.size ());
  std::iota (indices.begin (), indices.end (), std::size_t{ 0 });
  std::ranges::sort (indices, {}, [&] (std::size_t i) {
    return notes[i]->position ()->ticks ();
  });
  return indices;
}

/// Clamps every state of the result to what the note setters enforce.
void
sanitize_result (MidiFunction::Result &result)
{
  for (auto &state : result.new_states_)
    {
      state = sanitized (state);
    }
  for (auto &added : result.added_notes_)
    {
      added.state_ = sanitized (added.state_);
    }
}

/// Extends each note to (or truncates it at) the next distinct onset,
/// minus @p gap_ticks. Notes without a following distinct onset (the
/// final group) keep their length.
void
extend_to_next_onset (
  MidiFunction::Result           &result,
  const std::vector<std::size_t> &indices,
  dsp::ContentTick                gap_ticks)
{
  for (std::size_t rank = 0; rank < indices.size (); ++rank)
    {
      auto       &state = result.new_states_[indices[rank]];
      std::size_t next_rank = rank + 1;
      while (
        next_rank < indices.size ()
        && result.new_states_[indices[next_rank]].position_ticks_
             == state.position_ticks_)
        {
          ++next_rank;
        }
      if (next_rank == indices.size ())
        break;
      const auto &next = result.new_states_[indices[next_rank]];
      state.length_ticks_ = std::max (
        kMinLengthTicks,
        next.position_ticks_ - state.position_ticks_ - gap_ticks);
    }
}

/// The {display, value} entries of the curve algorithm choice.
QVariantList
curve_algorithm_choices ()
{
  return CurveOptions_Algorithm_to_qml_list ();
}

} // namespace

// --- Crescendo ---

// Labels keep the string literal directly inside the QObject::tr call:
// lupdate only extracts literals adjacent to a tr call (a macro wrapping
// the call hides the literal) and looks them up in the tr call's context.
utils::Utf8String
MidiFunction::Crescendo::label (bool translate)
{
  return translate ? utils::Utf8String::from_qstring (QObject::tr ("Crescendo"))
                   : utils::Utf8String (u8"Crescendo");
}

std::vector<EditorFunctionParam<MidiFunction::Crescendo>>
MidiFunction::Crescendo::params ()
{
  return {
    { utils::Utf8String (u8"startVelocity"),
     utils::Utf8String::from_qstring (QObject::tr ("Start Velocity")),
     { .kind = ParamSpec::Kind::Integer,
        .minimum = 1,
        .maximum = 127,
        .default_value = 1 },
     [] (Crescendo &c, ParamValue v) { c.start_vel = std::get<int> (v); }    },
    { utils::Utf8String (u8"endVelocity"),
     utils::Utf8String::from_qstring (QObject::tr ("End Velocity")),
     { .kind = ParamSpec::Kind::Integer,
        .minimum = 1,
        .maximum = 127,
        .default_value = 127 },
     [] (Crescendo &c, ParamValue v) { c.end_vel = std::get<int> (v); }      },
    { utils::Utf8String (u8"curveAlgorithm"),
     utils::Utf8String::from_qstring (QObject::tr ("Curve")),
     { .kind = ParamSpec::Kind::Choice,
        .choices = curve_algorithm_choices (),
        .default_value =
          ENUM_VALUE_TO_INT (dsp::CurveOptions::Algorithm::SuperEllipse) },
     [] (Crescendo &c, ParamValue v) {
        c.curve_algo =
          static_cast<dsp::CurveOptions::Algorithm> (std::get<int> (v));
      }                                                },
    { utils::Utf8String (u8"curviness"),
     utils::Utf8String::from_qstring (QObject::tr ("Curviness")),
     { .kind = ParamSpec::Kind::Double,
        .minimum = -1.0,
        .maximum = 1.0,
        .default_value = 0.5 },
     [] (Crescendo &c, ParamValue v) { c.curviness = std::get<double> (v); } },
  };
}

MidiFunction::Result
MidiFunction::Crescendo::compute (
  std::span<MidiNote * const> notes,
  const Crescendo            &params)
{
  Result result;
  result.new_states_ = capture_states (notes);
  if (notes.empty ())
    return result;

  const auto indices = by_position (notes);

  dsp::CurveOptions curve_opts;
  curve_opts.algo_ = params.curve_algo;
  curve_opts.curviness_ = params.curviness;

  const int vel_interval = std::abs (params.end_vel - params.start_vel);
  const dsp::ContentTick first_pos =
    result.new_states_[indices.front ()].position_ticks_;
  const dsp::ContentTick last_pos =
    result.new_states_[indices.back ()].position_ticks_;
  const dsp::ContentTick total_ticks = last_pos - first_pos;

  if (notes.size () == 1)
    {
      result.new_states_[0].velocity_ = params.start_vel;
      sanitize_result (result);
      return result;
    }
  if (total_ticks != dsp::ContentTick{})
    {
      for (auto &state : result.new_states_)
        {
          const double vel_multiplier = curve_opts.get_normalized_y (
            (state.position_ticks_ - first_pos) / total_ticks,
            params.start_vel > params.end_vel);
          state.velocity_ = std::lround (
            (double) std::min (params.start_vel, params.end_vel)
            + (vel_interval * vel_multiplier));
        }
    }

  sanitize_result (result);
  return result;
}

// --- Flam ---

utils::Utf8String
MidiFunction::Flam::label (bool translate)
{
  return translate ? utils::Utf8String::from_qstring (QObject::tr ("Flam"))
                   : utils::Utf8String (u8"Flam");
}

std::vector<EditorFunctionParam<MidiFunction::Flam>>
MidiFunction::Flam::params ()
{
  return {
    { utils::Utf8String (u8"time"),
     utils::Utf8String::from_qstring (QObject::tr ("Time (ms)")),
     { .kind = ParamSpec::Kind::Double,
        .minimum = -100.0,
        .maximum = 100.0,
        .default_value = 10.0 },
     [] (Flam &f, ParamValue v) {
        f.time = units::seconds (std::get<double> (v) / 1000.0);
      }                                       },
    { utils::Utf8String (u8"velocity"),
     utils::Utf8String::from_qstring (QObject::tr ("Velocity")),
     { .kind = ParamSpec::Kind::Integer,
        .minimum = 1,
        .maximum = 127,
        .default_value = 127 },
     [] (Flam &f, ParamValue v) { f.vel = std::get<int> (v); } },
  };
}

MidiFunction::Result
MidiFunction::Flam::compute (
  std::span<MidiNote * const> notes,
  const Flam                 &params)
{
  Result result;
  result.new_states_ = capture_states (notes);
  if (notes.empty ())
    return result;

  // Each note gets a grace note at the offset given by the time
  // parameter. For non-negative offsets the grace note starts after the
  // note, which is truncated to end where the grace note begins; for
  // negative offsets the grace note starts before the note and runs up
  // to the note's start.
  const bool non_negative = params.time >= units::precise_second_t{};
  for (std::size_t i = 0; i < notes.size (); ++i)
    {
      const NoteState        state = result.new_states_[i];
      const dsp::ContentTick offset_ticks =
        offset_ticks_at (*notes[i], params.time);
      if (non_negative)
        {
          result.new_states_[i].length_ticks_ = std::max (
            std::min (state.length_ticks_, offset_ticks), kMinLengthTicks);
        }
      result.added_notes_.push_back (
        {
          i,
          NoteState{
                    state.position_ticks_ + offset_ticks,
                    non_negative
              ? state.length_ticks_
              : std::max (-offset_ticks, kMinLengthTicks),
                    state.pitch_, params.vel }
      });
    }

  sanitize_result (result);
  return result;
}

// --- FlipHorizontal ---

utils::Utf8String
MidiFunction::FlipHorizontal::label (bool translate)
{
  return translate
           ? utils::Utf8String::from_qstring (QObject::tr ("Flip Horizontal"))
           : utils::Utf8String (u8"Flip Horizontal");
}

std::vector<EditorFunctionParam<MidiFunction::FlipHorizontal>>
MidiFunction::FlipHorizontal::params ()
{
  return {};
}

MidiFunction::Result
MidiFunction::FlipHorizontal::compute (
  std::span<MidiNote * const> notes,
  const FlipHorizontal &)
{
  Result result;
  result.new_states_ = capture_states (notes);
  if (notes.empty ())
    return result;

  // The note at each position rank takes the original position of the
  // note at the opposite rank, keeping its own length.
  const auto                    indices = by_position (notes);
  std::vector<dsp::ContentTick> original_positions;
  original_positions.reserve (notes.size ());
  for (const auto &state : result.new_states_)
    {
      original_positions.push_back (state.position_ticks_);
    }
  const auto n = indices.size ();
  for (std::size_t rank = 0; rank < n; ++rank)
    {
      const auto target_rank = n - rank - 1;
      if (target_rank == rank)
        continue;
      result.new_states_[indices[rank]].position_ticks_ =
        original_positions[target_rank];
    }

  sanitize_result (result);
  return result;
}

// --- FlipVertical ---

utils::Utf8String
MidiFunction::FlipVertical::label (bool translate)
{
  return translate
           ? utils::Utf8String::from_qstring (QObject::tr ("Flip Vertical"))
           : utils::Utf8String (u8"Flip Vertical");
}

std::vector<EditorFunctionParam<MidiFunction::FlipVertical>>
MidiFunction::FlipVertical::params ()
{
  return {};
}

MidiFunction::Result
MidiFunction::FlipVertical::compute (
  std::span<MidiNote * const> notes,
  const FlipVertical &)
{
  Result result;
  result.new_states_ = capture_states (notes);
  if (notes.empty ())
    return result;

  const auto pitches = notes | std::views::transform (&MidiNote::pitch);
  auto [min_it, max_it] = std::ranges::minmax_element (pitches);
  const int highest_pitch = *max_it;
  const int lowest_pitch = *min_it;
  const int diff = highest_pitch - lowest_pitch;
  for (auto &state : result.new_states_)
    {
      state.pitch_ = diff - (state.pitch_ - lowest_pitch) + lowest_pitch;
    }

  sanitize_result (result);
  return result;
}

// --- Legato ---

utils::Utf8String
MidiFunction::Legato::label (bool translate)
{
  return translate ? utils::Utf8String::from_qstring (QObject::tr ("Legato"))
                   : utils::Utf8String (u8"Legato");
}

std::vector<EditorFunctionParam<MidiFunction::Legato>>
MidiFunction::Legato::params ()
{
  return {};
}

MidiFunction::Result
MidiFunction::Legato::compute (std::span<MidiNote * const> notes, const Legato &)
{
  Result result;
  result.new_states_ = capture_states (notes);
  if (notes.empty ())
    return result;

  extend_to_next_onset (result, by_position (notes), dsp::ContentTick{});

  sanitize_result (result);
  return result;
}

// --- Portato ---

utils::Utf8String
MidiFunction::Portato::label (bool translate)
{
  return translate ? utils::Utf8String::from_qstring (QObject::tr ("Portato"))
                   : utils::Utf8String (u8"Portato");
}

std::vector<EditorFunctionParam<MidiFunction::Portato>>
MidiFunction::Portato::params ()
{
  return {};
}

MidiFunction::Result
MidiFunction::Portato::compute (
  std::span<MidiNote * const> notes,
  const Portato &)
{
  Result result;
  result.new_states_ = capture_states (notes);
  if (notes.empty ())
    return result;

  extend_to_next_onset (result, by_position (notes), kPortatoGapTicks);

  sanitize_result (result);
  return result;
}

// --- Staccato ---

utils::Utf8String
MidiFunction::Staccato::label (bool translate)
{
  return translate ? utils::Utf8String::from_qstring (QObject::tr ("Staccato"))
                   : utils::Utf8String (u8"Staccato");
}

std::vector<EditorFunctionParam<MidiFunction::Staccato>>
MidiFunction::Staccato::params ()
{
  return {};
}

MidiFunction::Result
MidiFunction::Staccato::compute (
  std::span<MidiNote * const> notes,
  const Staccato &)
{
  Result result;
  result.new_states_ = capture_states (notes);
  if (notes.empty ())
    return result;

  for (auto &state : result.new_states_)
    {
      state.length_ticks_ = kStaccatoLengthTicks;
    }

  sanitize_result (result);
  return result;
}

// --- Strum ---

utils::Utf8String
MidiFunction::Strum::label (bool translate)
{
  return translate ? utils::Utf8String::from_qstring (QObject::tr ("Strum"))
                   : utils::Utf8String (u8"Strum");
}

std::vector<EditorFunctionParam<MidiFunction::Strum>>
MidiFunction::Strum::params ()
{
  return {
    { utils::Utf8String (u8"ascending"),
     utils::Utf8String::from_qstring (QObject::tr ("Ascending")),
     { .kind = ParamSpec::Kind::Boolean, .default_value = true },
     [] (Strum &s, ParamValue v) { s.ascending = std::get<bool> (v); }   },
    { utils::Utf8String (u8"curveAlgorithm"),
     utils::Utf8String::from_qstring (QObject::tr ("Curve")),
     { .kind = ParamSpec::Kind::Choice,
        .choices = curve_algorithm_choices (),
        .default_value =
          ENUM_VALUE_TO_INT (dsp::CurveOptions::Algorithm::SuperEllipse) },
     [] (Strum &s, ParamValue v) {
        s.curve_algo =
          static_cast<dsp::CurveOptions::Algorithm> (std::get<int> (v));
      }                                                },
    { utils::Utf8String (u8"curviness"),
     utils::Utf8String::from_qstring (QObject::tr ("Curviness")),
     { .kind = ParamSpec::Kind::Double,
        .minimum = -1.0,
        .maximum = 1.0,
        .default_value = 0.5 },
     [] (Strum &s, ParamValue v) { s.curviness = std::get<double> (v); } },
    { utils::Utf8String (u8"time"),
     utils::Utf8String::from_qstring (QObject::tr ("Total Time (ms)")),
     { .kind = ParamSpec::Kind::Double,
        .minimum = 1.0,
        .maximum = 200.0,
        .default_value = 50.0 },
     [] (Strum &s, ParamValue v) {
        s.time = units::seconds (std::get<double> (v) / 1000.0);
      }                                                },
  };
}

MidiFunction::Result
MidiFunction::Strum::compute (
  std::span<MidiNote * const> notes,
  const Strum                &params)
{
  Result result;
  result.new_states_ = capture_states (notes);
  if (notes.empty ())
    return result;

  // The strum starts at the earliest note: the notes sorted by pitch are
  // offset from it following the curve, over the total time.
  const auto             indices = by_position (notes);
  const auto            &earliest = *notes[indices.front ()];
  const dsp::ContentTick base_pos =
    result.new_states_[indices.front ()].position_ticks_;

  dsp::CurveOptions curve_opts;
  curve_opts.algo_ = params.curve_algo;
  curve_opts.curviness_ = params.curviness;

  std::vector<std::size_t> by_pitch (notes.size ());
  std::iota (by_pitch.begin (), by_pitch.end (), std::size_t{ 0 });
  if (params.ascending)
    {
      std::ranges::sort (by_pitch, {}, [&] (std::size_t i) {
        return notes[i]->pitch ();
      });
    }
  else
    {
      std::ranges::sort (by_pitch, std::ranges::greater{}, [&] (std::size_t i) {
        return notes[i]->pitch ();
      });
    }
  const auto n = by_pitch.size ();
  for (std::size_t rank = 0; rank < n; ++rank)
    {
      // The offset multiplier grows with the rank; the strum direction
      // comes from the pitch sort alone
      const double time_multiplier =
        curve_opts.get_normalized_y ((double) rank / (double) n, false);
      result.new_states_[by_pitch[rank]].position_ticks_ =
        base_pos + offset_ticks_at (earliest, time_multiplier * params.time);
    }

  sanitize_result (result);
  return result;
}

// --- Generic dispatch ---

std::optional<MidiFunction::Kind>
MidiFunction::kind_from_id (std::string_view id)
{
  return detail::kind_from_id<Kind> (id);
}

utils::Utf8String
MidiFunction::label (const Kind &kind, bool translate)
{
  return detail::label (kind, translate);
}

MidiFunction::Result
MidiFunction::compute (std::span<MidiNote * const> notes, const Kind &kind)
{
  return detail::compute (notes, kind);
}

void
MidiFunction::apply (std::span<MidiNote *> sel, const Kind &kind)
{
  const auto result = compute (sel, kind);
  const auto write_state = [] (MidiNote &mn, const NoteState &state) {
    mn.setPitch (state.pitch_);
    mn.setVelocity (state.velocity_);
    mn.position ()->set_ticks (state.position_ticks_);
    mn.length ()->set_ticks (state.length_ticks_);
  };
  for (auto [mn, state] : std::views::zip (sel, result.new_states_))
    {
      write_state (*mn, state);
    }
  if (!result.added_notes_.empty ())
    {
      z_warning (
        "the function adds {} note(s) that apply() cannot attach to an owner",
        result.added_notes_.size ());
    }
}
}
