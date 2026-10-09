// SPDX-FileCopyrightText: © 2020, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

#include "dsp/curve.h"
#include "dsp/tick_types.h"
#include "structure/arrangement/editor_function_kind.h"
#include "structure/arrangement/editor_function_param.h"
#include "utils/midi.h"
#include "utils/units.h"
#include "utils/utf8_string.h"

namespace zrythm::structure::arrangement
{
class MidiNote;

/**
 * @brief Transformative functions applicable to a selection of MIDI
 * notes.
 *
 * Each function is a kind: a struct whose instance carries the
 * function's parameters, which also declares its parameters for the
 * generic function dialog and its computation. Kinds are pure
 * computations: compute() derives the target state of each note (and,
 * for Flam, the grace notes to add) without mutating anything, so the
 * result can be captured in an undoable command.
 */
class MidiFunction
{
public:
  /// The state of a MIDI note a computed function result targets.
  struct NoteState
  {
    dsp::ContentTick position_ticks_{};
    dsp::ContentTick length_ticks_{};
    int              pitch_ = 0;
    int              velocity_ = 0;
  };

  /// A note to add alongside the transformed selection, cloned from the input
  /// note at @ref source_index_.
  struct AddedNote
  {
    std::size_t source_index_ = 0;
    NoteState   state_{};
  };

  /// The outcome of computing a function over a selection of notes.
  struct Result
  {
    /// Target state of each input note, parallel to the input span.
    std::vector<NoteState> new_states_;
    /// Notes to add (Flam grace notes).
    std::vector<AddedNote> added_notes_;
  };

  /// Interpolates the velocities of the notes from start to end
  /// velocity, following a curve.
  struct Crescendo
  {
    /// Velocity of the first note.
    midi_byte_t start_vel{ 1 };
    /// Velocity of the last note.
    midi_byte_t                  end_vel{ 127 };
    dsp::CurveOptions::Algorithm curve_algo{
      dsp::CurveOptions::Algorithm::SuperEllipse
    };
    double curviness{ 0.5 };

    static constexpr std::string_view kId = "crescendo";
    static constexpr int              kMinimumSelectionCount = 1;

    static utils::Utf8String label (bool translate = false);
    static std::vector<EditorFunctionParam<Crescendo>> params ();
    static Result
    compute (std::span<MidiNote * const> notes, const Crescendo &params);
  };

  /// Adds a grace note before or after each note.
  struct Flam
  {
    /// Offset of the grace note from its note.
    units::precise_second_t time{ units::seconds (0.010) };
    /// Velocity of the grace notes.
    midi_byte_t vel{ 127 };

    static constexpr std::string_view kId = "flam";
    static constexpr int              kMinimumSelectionCount = 1;

    static utils::Utf8String label (bool translate = false);
    static std::vector<EditorFunctionParam<Flam>> params ();
    static Result
    compute (std::span<MidiNote * const> notes, const Flam &params);
  };

  /// Mirrors the positions of the notes within the selection.
  struct FlipHorizontal
  {
    static constexpr std::string_view kId = "flip-horizontal";
    static constexpr int              kMinimumSelectionCount = 1;

    static utils::Utf8String label (bool translate = false);
    static std::vector<EditorFunctionParam<FlipHorizontal>> params ();
    static Result
    compute (std::span<MidiNote * const> notes, const FlipHorizontal &params);
  };

  /// Mirrors the pitches of the notes within the selection.
  struct FlipVertical
  {
    static constexpr std::string_view kId = "flip-vertical";
    static constexpr int              kMinimumSelectionCount = 1;

    static utils::Utf8String label (bool translate = false);
    static std::vector<EditorFunctionParam<FlipVertical>> params ();
    static Result
    compute (std::span<MidiNote * const> notes, const FlipVertical &params);
  };

  /// Extends each note to the next distinct onset, so notes sharing an
  /// onset sustain together.
  struct Legato
  {
    static constexpr std::string_view kId = "legato";
    static constexpr int              kMinimumSelectionCount = 2;

    static utils::Utf8String label (bool translate = false);
    static std::vector<EditorFunctionParam<Legato>> params ();
    static Result
    compute (std::span<MidiNote * const> notes, const Legato &params);
  };

  /// Like Legato but leaving a gap before the next distinct onset.
  struct Portato
  {
    static constexpr std::string_view kId = "portato";
    static constexpr int              kMinimumSelectionCount = 2;

    static utils::Utf8String label (bool translate = false);
    static std::vector<EditorFunctionParam<Portato>> params ();
    static Result
    compute (std::span<MidiNote * const> notes, const Portato &params);
  };

  /// Shortens each note to a fixed length.
  struct Staccato
  {
    static constexpr std::string_view kId = "staccato";
    static constexpr int              kMinimumSelectionCount = 1;

    static utils::Utf8String label (bool translate = false);
    static std::vector<EditorFunctionParam<Staccato>> params ();
    static Result
    compute (std::span<MidiNote * const> notes, const Staccato &params);
  };

  /// Spreads the notes over time by pitch, following a curve.
  struct Strum
  {
    /// Whether the strum spreads from the lowest to the highest pitch.
    bool                         ascending{ true };
    dsp::CurveOptions::Algorithm curve_algo{
      dsp::CurveOptions::Algorithm::SuperEllipse
    };
    double curviness{ 0.5 };
    /// Total spread time.
    units::precise_second_t time{ units::seconds (0.050) };

    static constexpr std::string_view kId = "strum";
    static constexpr int              kMinimumSelectionCount = 1;

    static utils::Utf8String label (bool translate = false);
    static std::vector<EditorFunctionParam<Strum>> params ();
    static Result
    compute (std::span<MidiNote * const> notes, const Strum &params);
  };

  /// The available functions, each carrying its parameters.
  using Kind = std::variant<
    Crescendo,
    Flam,
    FlipHorizontal,
    FlipVertical,
    Legato,
    Portato,
    Staccato,
    Strum>;

  /**
   * Returns the function kind with the given id, if any.
   */
  static std::optional<Kind> kind_from_id (std::string_view id);

  /**
   * Returns the user-visible name of the function, translated when @p
   * translate is set.
   */
  static utils::Utf8String label (const Kind &kind, bool translate = false);

  /**
   * @brief Computes the target state of each note for the given function
   * kind, without mutating any note.
   *
   * Millisecond offsets (Flam, Strum) are converted to ticks at the tempo
   * under the notes. Position and length targets stay in the clip's
   * content-tick domain. Pitch and velocity targets are clamped to their
   * valid ranges, matching what the note setters enforce when the states
   * are written.
   *
   * @param notes Notes the function operates on.
   * @param kind Function kind with its parameters.
   */
  static Result compute (std::span<MidiNote * const> notes, const Kind &kind);

  /**
   * @brief Writes the computed target state of the given function kind to
   * each note of @p sel.
   *
   * Notes the function adds (Flam grace notes) are not attached to an owner
   * here; callers that support adding notes use compute() directly.
   *
   * @param sel Selections to edit.
   * @param kind Function kind with its parameters.
   */
  static void apply (std::span<MidiNote *> sel, const Kind &kind);
};
}
