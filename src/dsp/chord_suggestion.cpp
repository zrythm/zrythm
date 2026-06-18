// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <optional>
#include <vector>

#include "dsp/chord_suggestion.h"

namespace zrythm::dsp::chords
{

// ============================================================================
// Anonymous namespace: constexpr data + internal helpers
// ============================================================================

namespace
{

// Major-type scale degree transition weights [prev_degree][next_degree].
constexpr std::array<std::array<float, 7>, 7> major_transition_weights = {
  {
   { 0.2f, 0.8f, 0.5f, 0.9f, 0.8f, 0.8f, 0.2f }, // I
    { 0.4f, 0.2f, 0.2f, 0.5f, 1.0f, 0.3f, 0.4f }, // ii
    { 0.2f, 0.4f, 0.2f, 0.4f, 0.2f, 0.7f, 0.1f }, // iii
    { 0.7f, 0.7f, 0.2f, 0.2f, 1.0f, 0.5f, 0.4f }, // IV
    { 1.0f, 0.2f, 0.4f, 0.2f, 0.2f, 0.5f, 0.2f }, // V
    { 0.4f, 0.7f, 0.6f, 0.9f, 0.4f, 0.2f, 0.1f }, // vi
    { 1.0f, 0.1f, 0.1f, 0.2f, 0.4f, 0.2f, 0.1f }, // vii°
  }
};

// Minor-type scale degree transition weights [prev_degree][next_degree].
constexpr std::array<std::array<float, 7>, 7> minor_transition_weights = {
  {
   { 0.2f, 0.6f, 0.5f, 0.8f, 0.8f, 0.7f, 0.7f }, // i
    { 0.4f, 0.2f, 0.2f, 0.5f, 0.9f, 0.3f, 0.3f }, // ii°
    { 0.3f, 0.4f, 0.2f, 0.4f, 0.3f, 0.6f, 0.7f }, // III
    { 0.6f, 0.6f, 0.3f, 0.2f, 0.9f, 0.5f, 0.5f }, // iv
    { 0.7f, 0.2f, 0.4f, 0.2f, 0.2f, 0.4f, 0.3f }, // v
    { 0.4f, 0.6f, 0.6f, 0.7f, 0.4f, 0.2f, 0.5f }, // VI
    { 0.5f, 0.3f, 0.7f, 0.6f, 0.4f, 0.5f, 0.2f }, // VII
  }
};

// Root motion scores indexed by directed semitone interval (0-11).
constexpr std::array<float, 12> root_motion_scores = {
  0.0f, // unison
  0.4f, // minor 2nd
  0.5f, // major 2nd
  0.7f, // minor 3rd
  0.7f, // major 3rd
  1.0f, // perfect 4th (= descending 5th) — strongest
  0.1f, // tritone
  0.5f, // perfect 5th (= descending 4th)
  0.7f, // minor 6th (= descending major 3rd)
  0.7f, // major 6th (= descending minor 3rd)
  0.5f, // minor 7th (= descending major 2nd)
  0.4f, // major 7th (= descending minor 2nd)
};

// Score weights for combining subscores.
struct ScoreWeights
{
  float functional = 0.45f;
  float root_motion = 0.25f;
  float voice_leading = 0.15f;
  float diatonic = 0.15f;
};
constexpr ScoreWeights kScoreWeights{};

// Maximum modifier magnitudes.
struct MaxMagnitudes
{
  float seventh_resolution = 0.10f;
  float secondary_dominant = 0.25f;
  float leading_tone = 0.25f;
};
constexpr MaxMagnitudes kMaxMagnitudes{};

// Harmonic function per scale degree, indexed [minor ? 1 : 0][degree].
constexpr std::array<std::array<HarmonicFunction, 7>, 2> kDegreeFunctions = {
  {
   { HarmonicFunction::Tonic, HarmonicFunction::Predominant,
      HarmonicFunction::Other, HarmonicFunction::Predominant,
      HarmonicFunction::Dominant, HarmonicFunction::Tonic,
      HarmonicFunction::Dominant }, // major
    { HarmonicFunction::Tonic, HarmonicFunction::Predominant,
      HarmonicFunction::Other, HarmonicFunction::Predominant,
      HarmonicFunction::Dominant, HarmonicFunction::Tonic,
      HarmonicFunction::Other }, // minor
  }
};

// Named tuning constants.
constexpr float kSubstituteDiscount = 0.85f;
constexpr float kQualityAdjDiminishedPenalty = -0.15f;
constexpr float kQualityAdjMajorOverMinor = 0.10f;
constexpr float kQualityAdjMinorOverMajor = -0.10f;
constexpr float kQualityAdjMinorOverDim = 0.10f;
constexpr float kQualityAdjMajorOverDim = 0.15f;
constexpr float kHarmonicMinorVCadenceBonus = 0.20f;
constexpr float kDimDominantResolutionBonus = 0.15f;
constexpr float kMaxCircularDistance = 6.0f;

/// Returns the ChordType expected at a given degree for the reference scale.
ChordType
expected_chord_type_at_degree (int degree, bool minor_scale)
{
  if (minor_scale)
    {
      static constexpr std::array<ChordType, 7> types = {
        ChordType::Minor,      // i
        ChordType::Diminished, // ii°
        ChordType::Major,      // III
        ChordType::Minor,      // iv
        ChordType::Minor,      // v
        ChordType::Major,      // VI
        ChordType::Major,      // VII
      };
      return types[degree];
    }
  static constexpr std::array<ChordType, 7> types = {
    ChordType::Major,      // I
    ChordType::Minor,      // ii
    ChordType::Minor,      // iii
    ChordType::Major,      // IV
    ChordType::Major,      // V
    ChordType::Minor,      // vi
    ChordType::Diminished, // vii°
  };
  return types[degree];
}

/// Maps a borrowed chord root to its functional substitute degree (0-6).
/// Returns -1 if no mapping exists.
int
borrowed_chord_substitute_degree (MusicalNote root, const MusicalScale &scale)
{
  auto scale_root = static_cast<int> (scale.rootKey ());
  auto interval = (static_cast<int> (root) - scale_root + 12) % 12;
  bool minor = is_minor_type (scale);

  if (!minor)
    {
      switch (interval)
        {
        case 0:
          return 0; // Cm -> I
        case 2:
          return 1; // D° -> ii
        case 3:
          return 5; // Eb -> vi (pragmatic)
        case 5:
          return 3; // Fm -> IV
        case 7:
          return 4; // Gm -> V
        case 8:
          return 3; // Ab -> IV
        case 10:
          return 4; // Bb -> V
        default:
          return -1;
        }
    }
  // A minor borrowing from A major
  switch (interval)
    {
    case 0:
      return 0; // A -> i
    case 2:
      return 1; // Bm -> ii°
    case 5:
      return 3; // D -> iv
    default:
      return -1;
    }
}

/// Checks if a chord is a borrowed chord (in parallel key but not current).
bool
is_borrowed_chord (const ChordKey &chord, const MusicalScale &scale)
{
  return borrowed_chord_substitute_degree (chord.root_note, scale) >= 0
         && !scale.containsNote (chord.root_note);
}

/// Applies quality-based adjustments for modes and harmonic minor.
float
apply_quality_adjustments (
  float               base_weight,
  int                 prev_degree,
  const ChordKey     &prev,
  int                 candidate_degree,
  const ChordKey     &candidate,
  const MusicalScale &scale)
{
  bool  minor = is_minor_type (scale);
  float result = base_weight;

  // Candidate quality adjustments
  ChordType expected_candidate =
    expected_chord_type_at_degree (candidate_degree, minor);
  ChordType actual_candidate = candidate.type;

  if (actual_candidate != expected_candidate)
    {
      if (actual_candidate == ChordType::Diminished)
        result += kQualityAdjDiminishedPenalty;
      else if (
        actual_candidate == ChordType::Major
        && expected_candidate == ChordType::Minor)
        result += kQualityAdjMajorOverMinor;
      else if (
        actual_candidate == ChordType::Minor
        && expected_candidate == ChordType::Major)
        result += kQualityAdjMinorOverMajor;
      else if (
        actual_candidate == ChordType::Minor
        && expected_candidate == ChordType::Diminished)
        result += kQualityAdjMinorOverDim;
      else if (
        actual_candidate == ChordType::Major
        && expected_candidate == ChordType::Diminished)
        result += kQualityAdjMajorOverDim;
    }

  // Prev quality adjustments
  if (prev_degree >= 0)
    {
      ChordType expected_prev =
        expected_chord_type_at_degree (prev_degree, minor);
      ChordType actual_prev = prev.type;

      if (actual_prev == ChordType::Major && expected_prev == ChordType::Minor)
        {
          result += kQualityAdjMajorOverMinor;
          if (minor && prev_degree == 4 && candidate_degree == 0)
            result += kHarmonicMinorVCadenceBonus;
        }

      // Prev is dominant-function diminished resolving to tonic.
      // Only vii° in major (degree 6, expected diminished).
      bool prev_is_dim_dominant =
        actual_prev == ChordType::Diminished && !minor && prev_degree == 6;
      if (prev_is_dim_dominant && candidate_degree == 0)
        result += kDimDominantResolutionBonus;
    }

  return std::clamp (result, 0.0f, 1.0f);
}

/// Converts a conditional score [0,1] to a modifier via centered formula.
float
to_modifier (float score, float max_magnitude)
{
  return (score - 0.5f) * 2.0f * max_magnitude;
}

/// Checks if a chord is the raised-leading-tone diminished in a minor key.
bool
is_leading_tone_chord (const ChordKey &chord, const MusicalScale &scale)
{
  if (!is_minor_type (scale))
    return false;
  if (chord.type != ChordType::Diminished)
    return false;
  auto root_val = static_cast<int> (scale.rootKey ());
  auto leading_tone = static_cast<MusicalNote> ((root_val + 11) % 12);
  return chord.root_note == leading_tone;
}

/// Checks if a chord is the tonic of the scale.
bool
is_tonic (const ChordKey &chord, const MusicalScale &scale)
{
  return chord.root_note == scale.rootKey ();
}

/// Maps a scale degree + scale to a harmonic function.
HarmonicFunction
degree_to_function (int degree, bool minor)
{
  if (degree < 0 || degree > 6)
    return HarmonicFunction::Other;
  return kDegreeFunctions[minor ? 1 : 0][static_cast<size_t> (degree)];
}

// ---------------------------------------------------------------------------
// Candidate generators (internal)
// ---------------------------------------------------------------------------

std::vector<CandidateChord>
generate_diatonic_triads (const MusicalScale &scale)
{
  auto triads =
    MusicalScale::get_diatonic_triads (scale.scaleType (), scale.rootKey ());
  std::vector<CandidateChord> result;
  for (size_t i = 0; i < triads.size (); ++i)
    {
      CandidateChord cand;
      cand.chord_key = ChordKey{ triads[i].root_note, triads[i].chord_type };
      cand.type = CandidateType::DiatonicTriad;
      cand.scale_degree = static_cast<int> (i);
      result.push_back (std::move (cand));
    }
  return result;
}

std::vector<CandidateChord>
generate_diatonic_sevenths (const MusicalScale &scale)
{
  auto triads =
    MusicalScale::get_diatonic_triads (scale.scaleType (), scale.rootKey ());
  std::vector<CandidateChord> result;
  for (size_t i = 0; i < triads.size (); ++i)
    {
      bool seventh_in_scale = scale.isAccentInScale (
        triads[i].root_note, triads[i].chord_type, ChordAccent::Seventh);
      bool maj_seventh_in_scale = scale.isAccentInScale (
        triads[i].root_note, triads[i].chord_type, ChordAccent::MajorSeventh);

      ChordAccent accent = ChordAccent::None;
      if (seventh_in_scale)
        accent = ChordAccent::Seventh;
      else if (maj_seventh_in_scale)
        accent = ChordAccent::MajorSeventh;
      else
        continue;

      CandidateChord cand;
      cand.chord_key =
        ChordKey{ triads[i].root_note, triads[i].chord_type, accent };
      cand.type = CandidateType::DiatonicSeventh;
      cand.scale_degree = static_cast<int> (i);
      result.push_back (std::move (cand));
    }
  return result;
}

std::vector<CandidateChord>
generate_secondary_dominants (const MusicalScale &scale)
{
  auto triads =
    MusicalScale::get_diatonic_triads (scale.scaleType (), scale.rootKey ());
  std::vector<CandidateChord> result;

  for (size_t i = 0; i < triads.size (); ++i)
    {
      if (i == 0)
        continue;
      if (triads[i].chord_type == ChordType::Diminished)
        continue;

      auto target_root = static_cast<int> (triads[i].root_note);
      auto dom_root = static_cast<MusicalNote> ((target_root + 7) % 12);

      CandidateChord cand;
      cand.chord_key =
        ChordKey{ dom_root, ChordType::Major, ChordAccent::Seventh };
      cand.type = CandidateType::SecondaryDominant;
      cand.scale_degree = -1;
      cand.secondary_dominant_target_degree = static_cast<int> (i);
      result.push_back (std::move (cand));
    }
  return result;
}

std::vector<CandidateChord>
generate_borrowed_chords (const MusicalScale &scale)
{
  std::vector<CandidateChord> result;
  bool                        minor = is_minor_type (scale);

  auto parallel_type =
    minor ? MusicalScale::ScaleType::Major : MusicalScale::ScaleType::Minor;
  auto parallel_triads =
    MusicalScale::get_diatonic_triads (parallel_type, scale.rootKey ());
  auto current_triads =
    MusicalScale::get_diatonic_triads (scale.scaleType (), scale.rootKey ());
  auto scale_root_val = static_cast<int> (scale.rootKey ());

  for (const auto &pt : parallel_triads)
    {
      const int sub_degree =
        borrowed_chord_substitute_degree (pt.root_note, scale);
      if (sub_degree < 0)
        continue;

      if (
        std::ranges::any_of (current_triads, [&] (const auto &ct) {
          return ct.root_note == pt.root_note && ct.chord_type == pt.chord_type;
        }))
        continue;

      if (minor)
        {
          const auto root_int = static_cast<int> (pt.root_note);
          if (
            root_int == (scale_root_val + 7) % 12
            || root_int == (scale_root_val + 11) % 12)
            continue;
        }

      CandidateChord cand;
      cand.chord_key = ChordKey{ pt.root_note, pt.chord_type };
      cand.type = CandidateType::BorrowedChord;
      cand.scale_degree = sub_degree;
      result.push_back (std::move (cand));
    }
  return result;
}

std::vector<CandidateChord>
generate_minor_key_leading_tone (const MusicalScale &scale)
{
  std::vector<CandidateChord> result;
  if (!is_minor_type (scale))
    return result;

  auto root_val = static_cast<int> (scale.rootKey ());
  auto leading_tone = static_cast<MusicalNote> ((root_val + 11) % 12);

  CandidateChord cand;
  cand.chord_key = ChordKey{ leading_tone, ChordType::Diminished };
  cand.type = CandidateType::LeadingTone;
  cand.scale_degree = -1;
  result.push_back (std::move (cand));
  return result;
}

/// If @p chord is a secondary dominant (major-minor 7th), returns the diatonic
/// degree (1-6) it resolves to, or -1 if it is not a secondary dominant or its
/// target is the tonic / a diminished triad.
int
detect_secondary_dominant_target (
  const ChordKey     &chord,
  const MusicalScale &scale)
{
  if (chord.type != ChordType::Major || chord.accent != ChordAccent::Seventh)
    return -1;

  auto dom_root = static_cast<int> (chord.root_note);
  auto target_root = static_cast<MusicalNote> ((dom_root + 5) % 12);

  int target_degree = get_scale_degree (target_root, scale);
  if (target_degree <= 0)
    return -1;

  auto triads =
    MusicalScale::get_diatonic_triads (scale.scaleType (), scale.rootKey ());
  if (
    target_degree < static_cast<int> (triads.size ())
    && triads[target_degree].chord_type == ChordType::Diminished)
    return -1;

  return target_degree;
}

} // anonymous namespace

// ============================================================================
// Public functions
// ============================================================================

bool
is_minor_type (const MusicalScale &scale)
{
  return !scale.containsNote (
    static_cast<MusicalNote> ((static_cast<int> (scale.rootKey ()) + 4) % 12));
}

int
get_scale_degree (MusicalNote root, const MusicalScale &scale)
{
  if (!scale.containsNote (root))
    return -1;

  auto root_val = static_cast<int> (root);
  auto scale_root_val = static_cast<int> (scale.rootKey ());
  int  semitones_from_root = (root_val - scale_root_val + 12) % 12;

  int degree = 0;
  for (int i = 0; i < semitones_from_root; ++i)
    {
      if (
        scale.containsNote (static_cast<MusicalNote> ((scale_root_val + i) % 12)))
        ++degree;
    }
  return degree;
}

float
score_root_motion (MusicalNote prev_root, MusicalNote next_root)
{
  int interval =
    (static_cast<int> (next_root) - static_cast<int> (prev_root) + 12) % 12;
  return root_motion_scores[interval];
}

float
score_voice_leading (const ChordKey &prev, const ChordKey &next)
{
  auto prev_desc = prev.make_descriptor ();
  auto next_desc = next.make_descriptor ();
  auto prev_pitches = prev_desc.getMidiPitches ();
  auto next_pitches = next_desc.getMidiPitches ();

  std::array<bool, 12> prev_pc{};
  std::array<bool, 12> next_pc{};
  for (auto p : prev_pitches)
    prev_pc[p % 12] = true;
  for (auto p : next_pitches)
    next_pc[p % 12] = true;

  int shared = 0;
  int prev_count = 0;
  int next_count = 0;
  for (int i = 0; i < 12; ++i)
    {
      if (prev_pc[i])
        ++prev_count;
      if (next_pc[i])
        ++next_count;
      if (prev_pc[i] && next_pc[i])
        ++shared;
    }

  const int max_tones = std::min (prev_count, next_count);
  if (max_tones == 0)
    return 0.0f;

  const float shared_ratio =
    static_cast<float> (shared) / static_cast<float> (max_tones);

  // Greedy nearest-voice displacement on pitch classes (mod 12) with
  // shortest circular distance. This avoids octave-offset artifacts from
  // raw MIDI pitches (different chord roots start at different octaves).
  std::vector<int> next_pc_list;
  for (int i = 0; i < 12; ++i)
    if (next_pc[i])
      next_pc_list.push_back (i);

  float total_displacement = 0.0f;
  int   matched = 0;
  for (int i = 0; i < 12; ++i)
    {
      if (!prev_pc[i])
        continue;
      if (next_pc_list.empty ())
        break;

      size_t best_idx = 0;
      int    best_dist = std::numeric_limits<int>::max ();
      for (size_t j = 0; j < next_pc_list.size (); ++j)
        {
          int diff = std::abs (i - next_pc_list[j]);
          int dist = std::min (diff, 12 - diff);
          if (dist < best_dist)
            {
              best_dist = dist;
              best_idx = j;
            }
        }
      total_displacement += static_cast<float> (best_dist);
      ++matched;
      next_pc_list.erase (next_pc_list.begin () + best_idx);
    }

  const float avg_displacement =
    matched > 0
      ? total_displacement / static_cast<float> (matched)
      : kMaxCircularDistance;
  const float displacement_score =
    std::clamp (1.0f - avg_displacement / kMaxCircularDistance, 0.0f, 1.0f);

  return 0.5f * shared_ratio + 0.5f * displacement_score;
}

float
score_diatonic_membership (const ChordKey &chord, const MusicalScale &scale)
{
  auto desc = chord.make_descriptor ();
  auto intervals = desc.getIntervals ();
  if (intervals.empty ())
    return 0.0f;

  int  in_scale = 0;
  int  total = 0;
  auto root_val = static_cast<int> (chord.root_note);
  for (int interval : intervals)
    {
      auto note = static_cast<MusicalNote> ((root_val + interval) % 12);
      if (scale.containsNote (note))
        ++in_scale;
      ++total;
    }

  return total > 0 ? static_cast<float> (in_scale) / static_cast<float> (total) : 0.0f;
}

float
score_functional_compatibility (
  const ChordKey     &prev,
  const ChordKey     &candidate,
  const MusicalScale &scale)
{
  bool minor = is_minor_type (scale);

  int prev_degree = get_scale_degree (prev.root_note, scale);
  int candidate_degree = get_scale_degree (candidate.root_note, scale);

  // Apply borrowed chord substitute mapping for BOTH prev and candidate.
  bool prev_is_borrowed = is_borrowed_chord (prev, scale);
  if (prev_is_borrowed && prev_degree < 0)
    {
      prev_degree = borrowed_chord_substitute_degree (prev.root_note, scale);
    }

  bool candidate_is_borrowed = is_borrowed_chord (candidate, scale);
  if (candidate_is_borrowed && candidate_degree < 0)
    {
      candidate_degree =
        borrowed_chord_substitute_degree (candidate.root_note, scale);
    }

  // If prev has no degree (secondary dominant, #vii°, or unmapped chromatic),
  // check if it's a harmonic minor V before returning neutral.
  if (prev_degree < 0)
    {
      // Harmonic minor V in minor keys: root at degree 4 position
      // (perfect 5th above tonic), major quality. This is a recognized
      // chromatic variant that SHOULD get matrix scoring.
      auto scale_root_val = static_cast<int> (scale.rootKey ());
      auto dominant_root = static_cast<MusicalNote> ((scale_root_val + 7) % 12);
      if (
        minor && prev.root_note == dominant_root
        && prev.type == ChordType::Major)
        {
          prev_degree = 4;
        }
      else
        {
          return 0.5f;
        }
    }

  if (candidate_degree < 0)
    return 0.5f;

  // Check if prev is a chromatic chord with diatonic root (e.g., D7 in C
  // major: root D is in scale but F# is not). These are secondary dominants
  // or altered chords — return neutral so the conditional modifier handles
  // them. Exempt: borrowed chords (already mapped to substitute degree) and
  // harmonic minor V (recognized chromatic variant).
  if (
    prev_degree >= 0 && !prev_is_borrowed
    && score_diatonic_membership (prev, scale) < 1.0f)
    {
      auto scale_root_val = static_cast<int> (scale.rootKey ());
      auto dominant_root = static_cast<MusicalNote> ((scale_root_val + 7) % 12);
      if (!(minor && prev.root_note == dominant_root
            && prev.type == ChordType::Major))
        {
          return 0.5f;
        }
    }

  const auto &matrix =
    minor ? minor_transition_weights : major_transition_weights;

  // Both degrees must be valid 7×7 matrix indices. The pipeline guarantees
  // this: generate_candidates() returns empty for non-heptatonic scales, and
  // chromatic-root chords (borrowed, secondary dominants) are mapped to a
  // substitute degree or return early above. Direct callers must only pass
  // heptatonic scales.
  assert (prev_degree >= 0 && prev_degree < 7);
  assert (candidate_degree >= 0 && candidate_degree < 7);

  float base_weight = matrix[prev_degree][candidate_degree];

  if (prev_is_borrowed)
    base_weight *= kSubstituteDiscount;
  if (candidate_is_borrowed)
    base_weight *= kSubstituteDiscount;

  base_weight = apply_quality_adjustments (
    base_weight, prev_degree, prev, candidate_degree, candidate, scale);

  return base_weight;
}

float
score_seventh_resolution (
  const ChordKey     &prev,
  const ChordKey     &next,
  const MusicalScale &scale)
{
  // Only dominant 7ths carry a dissonance that "needs" resolution.
  // Major 7ths are consonant color tones — return neutral.
  if (prev.accent != ChordAccent::Seventh)
    return 0.5f;

  auto prev_root = static_cast<int> (prev.root_note);
  int  seventh_pitch_class = (prev_root + 10) % 12;

  // Scan down from the 7th to find the nearest scale note below it.
  // That note is the expected diatonic-step resolution (can be a half
  // step or whole step depending on the key). E.g., E7 in C major:
  // 7th is D, nearest scale note below is C (whole step) — correct.
  for (int i = 1; i <= 12; ++i)
    {
      auto resolution_note =
        static_cast<MusicalNote> ((seventh_pitch_class - i + 12) % 12);
      if (scale.containsNote (resolution_note))
        {
          auto next_desc = next.make_descriptor ();
          return next_desc.isKeyInChord (resolution_note) ? 1.0f : 0.0f;
        }
    }
  return 0.5f; // unreachable — scales always have notes
}

float
score_secondary_dominant (
  const CandidateChord &prev,
  const CandidateChord &candidate,
  const MusicalScale & /*scale*/)
{
  if (
    prev.type == CandidateType::SecondaryDominant
    && prev.secondary_dominant_target_degree.has_value ()
    && candidate.scale_degree == *prev.secondary_dominant_target_degree)
    return 1.0f;

  if (prev.type == CandidateType::SecondaryDominant)
    return 0.0f;

  return 0.5f;
}

float
score_leading_tone_resolution (
  const ChordKey     &prev,
  const ChordKey     &candidate,
  const MusicalScale &scale)
{
  if (!is_leading_tone_chord (prev, scale))
    return 0.5f;

  if (is_tonic (candidate, scale))
    return 1.0f;

  auto root_val = static_cast<int> (scale.rootKey ());
  auto dominant = static_cast<MusicalNote> ((root_val + 7) % 12);
  if (candidate.root_note == dominant)
    return 0.3f;

  return 0.15f;
}

float
score_borrowed_chord (
  const CandidateChord & /*candidate*/,
  const MusicalScale & /*scale*/)
{
  return 0.5f;
}

std::vector<CandidateChord>
generate_candidates (const MusicalScale &scale, ChordCandidateCategory categories)
{
  // Guard: the 7×7 transition matrices require exactly 7 scale degrees.
  // Non-heptatonic scales (pentatonic, chromatic, etc.) would produce
  // out-of-bounds array accesses or musically meaningless results.
  const auto notes = MusicalScale::get_notes_for_type (scale.scaleType (), true);
  if (std::ranges::count (notes, true) != 7)
    return {};

  std::vector<CandidateChord> candidates;

  auto add = [&] (std::vector<CandidateChord> &&batch) {
    for (auto &c : batch)
      {
        if (std::ranges::none_of (candidates, [&] (const auto &existing) {
              return existing.chord_key == c.chord_key;
            }))
          candidates.push_back (std::move (c));
      }
  };

  if (
    static_cast<uint32_t> (categories)
    & static_cast<uint32_t> (ChordCandidateCategory::DiatonicTriads))
    add (generate_diatonic_triads (scale));

  if (
    static_cast<uint32_t> (categories)
    & static_cast<uint32_t> (ChordCandidateCategory::DiatonicSevenths))
    add (generate_diatonic_sevenths (scale));

  if (
    static_cast<uint32_t> (categories)
    & static_cast<uint32_t> (ChordCandidateCategory::SecondaryDominants))
    add (generate_secondary_dominants (scale));

  if (
    static_cast<uint32_t> (categories)
    & static_cast<uint32_t> (ChordCandidateCategory::BorrowedChords))
    add (generate_borrowed_chords (scale));

  if (
    static_cast<uint32_t> (categories)
    & static_cast<uint32_t> (ChordCandidateCategory::LeadingTone))
    add (generate_minor_key_leading_tone (scale));

  return candidates;
}

std::vector<ChordSuggestion>
suggest_next_chords (
  const ChordKey        &current_chord,
  const MusicalScale    &scale,
  ChordCandidateCategory categories,
  int                    max_results)
{
  auto candidates = generate_candidates (scale, categories);
  if (candidates.empty ())
    return {};

  const bool minor = is_minor_type (scale);

  CandidateChord current_cand;
  current_cand.chord_key = current_chord;
  current_cand.scale_degree = get_scale_degree (current_chord.root_note, scale);
  int sec_dom_target = detect_secondary_dominant_target (current_chord, scale);
  if (sec_dom_target >= 0)
    {
      current_cand.type = CandidateType::SecondaryDominant;
      current_cand.secondary_dominant_target_degree = sec_dom_target;
    }

  std::vector<ChordSuggestion> suggestions;
  suggestions.reserve (candidates.size ());

  for (const auto &cand : candidates)
    {
      if (cand.chord_key == current_chord)
        continue;

      ChordSuggestion s;
      s.chord_key = cand.chord_key;
      s.candidate_type = cand.type;

      s.functional_score =
        score_functional_compatibility (current_chord, cand.chord_key, scale);
      s.root_motion_score =
        score_root_motion (current_chord.root_note, cand.chord_key.root_note);
      s.voice_leading_score =
        score_voice_leading (current_chord, cand.chord_key);
      const float diatonic = score_diatonic_membership (cand.chord_key, scale);

      const float combined =
        kScoreWeights.functional * s.functional_score
        + kScoreWeights.root_motion * s.root_motion_score
        + kScoreWeights.voice_leading * s.voice_leading_score
        + kScoreWeights.diatonic * diatonic;

      const float seventh_mod = to_modifier (
        score_seventh_resolution (current_chord, cand.chord_key, scale),
        kMaxMagnitudes.seventh_resolution);
      const float secondary_mod = to_modifier (
        score_secondary_dominant (current_cand, cand, scale),
        kMaxMagnitudes.secondary_dominant);
      const float leading_tone_mod = to_modifier (
        score_leading_tone_resolution (current_chord, cand.chord_key, scale),
        kMaxMagnitudes.leading_tone);

      s.overall_score =
        combined + seventh_mod + secondary_mod + leading_tone_mod;

      s.scale_degree = cand.scale_degree;
      s.function =
        cand.scale_degree >= 0
          ? degree_to_function (cand.scale_degree, minor)
          : HarmonicFunction::Other;

      suggestions.push_back (std::move (s));
    }

  std::sort (
    suggestions.begin (), suggestions.end (),
    [] (const ChordSuggestion &a, const ChordSuggestion &b) {
      return a.overall_score > b.overall_score;
    });

  for (int i = 0; i < static_cast<int> (suggestions.size ()); ++i)
    suggestions[i].rank = i + 1;

  if (!suggestions.empty ())
    {
      float max_score = suggestions[0].overall_score;
      if (max_score > 0.0f)
        {
          for (auto &s : suggestions)
            s.overall_score =
              std::clamp (s.overall_score / max_score, 0.0f, 1.0f);
        }
      else
        {
          for (auto &s : suggestions)
            s.overall_score = 0.0f;
        }
    }

  // Guard against a non-positive max_results: resize() takes size_t, so a
  // negative value would implicitly convert to a huge size and throw
  // std::bad_alloc. Treat <= 0 as "no limit".
  if (max_results > 0 && static_cast<int> (suggestions.size ()) > max_results)
    suggestions.resize (max_results);

  return suggestions;
}

} // namespace zrythm::dsp::chords
