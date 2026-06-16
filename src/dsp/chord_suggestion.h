// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <vector>

#include "dsp/chord_descriptor.h"
#include "dsp/musical_scale.h"

namespace zrythm::dsp::chords
{

// ============================================================================
// Enums
// ============================================================================

/// Bit flags for which categories of chord candidates to generate.
enum class ChordCandidateCategory : uint32_t
{
  DiatonicTriads = 1 << 0,
  DiatonicSevenths = 1 << 1,
  SecondaryDominants = 1 << 2,
  BorrowedChords = 1 << 3,
  LeadingTone = 1 << 4,
};

inline ChordCandidateCategory
operator| (ChordCandidateCategory a, ChordCandidateCategory b) noexcept
{
  return static_cast<ChordCandidateCategory> (
    static_cast<uint32_t> (a) | static_cast<uint32_t> (b));
}

inline ChordCandidateCategory
operator& (ChordCandidateCategory a, ChordCandidateCategory b) noexcept
{
  return static_cast<ChordCandidateCategory> (
    static_cast<uint32_t> (a) & static_cast<uint32_t> (b));
}

/// Harmonic function of a chord within its scale.
enum class HarmonicFunction
{
  Tonic,
  Predominant,
  Dominant,
  Other,
};

/// How a candidate was generated — populated at generation time.
enum class CandidateType
{
  DiatonicTriad,
  DiatonicSeventh,
  SecondaryDominant,
  BorrowedChord,
  LeadingTone,
};

// ============================================================================
// Structs
// ============================================================================

/// Lightweight POD storing just the chord identity (no QObject overhead).
/// ChordDescriptor inherits QObject (copy/move deleted in Qt6), so it cannot
/// be stored by value in containers. Use make_descriptor() when you need a
/// temporary ChordDescriptor for method calls.
struct ChordKey
{
  MusicalNote root_note = MusicalNote::C;
  ChordType   type = ChordType::Major;
  ChordAccent accent = ChordAccent::None;

  bool operator== (const ChordKey &other) const
  {
    return root_note == other.root_note && type == other.type
           && accent == other.accent;
  }

  /// Constructs a temporary ChordDescriptor for method calls.
  ChordDescriptor make_descriptor () const
  {
    return ChordDescriptor (root_note, type, accent);
  }
};

/// A candidate chord with metadata about how it was generated.
struct CandidateChord
{
  ChordKey           chord_key{};
  CandidateType      type = CandidateType::DiatonicTriad;
  int                scale_degree = -1;
  std::optional<int> secondary_dominant_target_degree{};
};

/// The result of scoring one candidate chord.
struct ChordSuggestion
{
  ChordKey         chord_key{};
  CandidateType    candidate_type = CandidateType::DiatonicTriad;
  float            overall_score = 0.0f;
  float            functional_score = 0.0f;
  float            root_motion_score = 0.0f;
  float            voice_leading_score = 0.0f;
  int              scale_degree = -1;
  HarmonicFunction function = HarmonicFunction::Other;
  int              rank = 0;
};

// ============================================================================
// Public API
// ============================================================================

/// Returns true if the scale has a minor 3rd.
bool
is_minor_type (const MusicalScale &scale);

/// Returns the scale degree (0-6) of a chord root, or -1 if not in scale.
int
get_scale_degree (MusicalNote root, const MusicalScale &scale);

/// Scores the root-note interval change. Returns 0.0-1.0.
float
score_root_motion (MusicalNote prev_root, MusicalNote next_root);

/// Scores voice-leading smoothness. Returns 0.0-1.0.
float
score_voice_leading (const ChordKey &prev, const ChordKey &next);

/// Scores what fraction of the chord's tones belong to the scale.
float
score_diatonic_membership (const ChordKey &chord, const MusicalScale &scale);

/// Scores functional compatibility. Returns 0.0-1.0.
float
score_functional_compatibility (
  const ChordKey     &prev,
  const ChordKey     &candidate,
  const MusicalScale &scale);

/// Conditional: 7th resolution. Returns 0.0-1.0 (0.5 = neutral).
/// Only dominant 7ths are processed; major 7ths return neutral.
float
score_seventh_resolution (
  const ChordKey     &prev,
  const ChordKey     &next,
  const MusicalScale &scale);

/// Conditional: secondary dominant resolution. Returns 0.0-1.0.
float
score_secondary_dominant (
  const CandidateChord &prev,
  const CandidateChord &candidate,
  const MusicalScale   &scale);

/// Conditional: leading tone resolution (minor keys). Returns 0.0-1.0.
float
score_leading_tone_resolution (
  const ChordKey     &prev,
  const ChordKey     &candidate,
  const MusicalScale &scale);

/// Conditional: borrowed chord (no-op). Returns 0.5.
///
/// Placeholder for future genre-specific borrowed-chord preferences. It is
/// currently NOT invoked by suggest_next_chords(): the chromaticism cost of
/// borrowed chords is already captured by the 0.85 substitute discount in
/// score_functional_compatibility() and the lower score_diatonic_membership(),
/// so applying an extra modifier here would triple-count the same quality.
float
score_borrowed_chord (const CandidateChord &candidate, const MusicalScale &scale);

/// Generates all candidates based on the selected categories.
std::vector<CandidateChord>
generate_candidates (
  const MusicalScale    &scale,
  ChordCandidateCategory categories);

/// Main entry point: returns ranked chord suggestions for what to play next.
std::vector<ChordSuggestion>
suggest_next_chords (
  const ChordKey        &current_chord,
  const MusicalScale    &scale,
  ChordCandidateCategory categories,
  int                    max_results = 3);

} // namespace zrythm::dsp::chords
