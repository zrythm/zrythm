// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <set>

#include "dsp/chord_suggestion.h"

#include <gtest/gtest.h>

namespace zrythm::dsp::chords
{

// ============================================================================
// Placeholder / data type test
// ============================================================================

TEST (ChordSuggestionTest, Placeholder)
{
  ChordSuggestion suggestion{};
  suggestion.overall_score = 0.5f;
  EXPECT_FLOAT_EQ (suggestion.overall_score, 0.5f);
}

// ============================================================================
// is_minor_type
// ============================================================================

TEST (ChordSuggestionTest, IsMinorType)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  MusicalScale a_minor (MusicalScale::ScaleType::Minor, MusicalNote::A);
  MusicalScale d_dorian (MusicalScale::ScaleType::Dorian, MusicalNote::D);
  MusicalScale g_mixolydian (
    MusicalScale::ScaleType::Mixolydian, MusicalNote::G);

  EXPECT_FALSE (is_minor_type (c_major));
  EXPECT_TRUE (is_minor_type (a_minor));
  EXPECT_TRUE (is_minor_type (d_dorian));
  EXPECT_FALSE (is_minor_type (g_mixolydian));
}

// ============================================================================
// Root motion
// ============================================================================

TEST (ChordSuggestionTest, RootMotionDescendingFifth)
{
  EXPECT_FLOAT_EQ (score_root_motion (MusicalNote::G, MusicalNote::C), 1.0f);
}

TEST (ChordSuggestionTest, RootMotionAscendingFifth)
{
  EXPECT_FLOAT_EQ (score_root_motion (MusicalNote::C, MusicalNote::G), 0.5f);
}

TEST (ChordSuggestionTest, RootMotionTritone)
{
  EXPECT_NEAR (
    score_root_motion (MusicalNote::C, MusicalNote::FSharp), 0.1f, 0.001f);
}

TEST (ChordSuggestionTest, RootMotionUnison)
{
  EXPECT_FLOAT_EQ (score_root_motion (MusicalNote::C, MusicalNote::C), 0.0f);
}

// ============================================================================
// Voice leading
// ============================================================================

TEST (ChordSuggestionTest, VoiceLeadingSharedTones)
{
  ChordKey c_major{ MusicalNote::C, ChordType::Major };
  ChordKey a_minor{ MusicalNote::A, ChordType::Minor };
  ChordKey f_sharp_major{ MusicalNote::FSharp, ChordType::Major };

  EXPECT_GT (
    score_voice_leading (c_major, a_minor),
    score_voice_leading (c_major, f_sharp_major));
}

// ============================================================================
// Diatonic membership
// ============================================================================

TEST (ChordSuggestionTest, DiatonicMembership)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);

  ChordKey c{ MusicalNote::C, ChordType::Major };
  EXPECT_NEAR (score_diatonic_membership (c, c_major), 1.0f, 0.001f);

  ChordKey f_sharp{ MusicalNote::FSharp, ChordType::Major };
  EXPECT_NEAR (score_diatonic_membership (f_sharp, c_major), 0.0f, 0.001f);

  ChordKey bb{ MusicalNote::ASharp, ChordType::Major };
  EXPECT_NEAR (score_diatonic_membership (bb, c_major), 2.0f / 3.0f, 0.01f);
}

// ============================================================================
// Functional compatibility
// ============================================================================

TEST (ChordSuggestionTest, FunctionalCompatibilityAuthenticCadence)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  ChordKey     g{ MusicalNote::G, ChordType::Major };
  ChordKey     c{ MusicalNote::C, ChordType::Major };
  EXPECT_NEAR (score_functional_compatibility (g, c, c_major), 1.0f, 0.05f);
}

TEST (ChordSuggestionTest, FunctionalCompatibilityRetrogressive)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  ChordKey     g{ MusicalNote::G, ChordType::Major };
  ChordKey     f{ MusicalNote::F, ChordType::Major };
  EXPECT_NEAR (score_functional_compatibility (g, f, c_major), 0.2f, 0.05f);
}

TEST (ChordSuggestionTest, FunctionalCompatibilityHarmonicMinorVToI)
{
  MusicalScale a_minor (MusicalScale::ScaleType::Minor, MusicalNote::A);
  ChordKey     e_major{ MusicalNote::E, ChordType::Major };
  ChordKey     a_minor_chord{ MusicalNote::A, ChordType::Minor };
  EXPECT_NEAR (
    score_functional_compatibility (e_major, a_minor_chord, a_minor), 1.0f,
    0.05f);
}

TEST (ChordSuggestionTest, FunctionalCompatibilityChromaticPrev)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  ChordKey     d7{ MusicalNote::D, ChordType::Major, ChordAccent::Seventh };
  ChordKey     g{ MusicalNote::G, ChordType::Major };
  EXPECT_NEAR (score_functional_compatibility (d7, g, c_major), 0.5f, 0.05f);
}

TEST (ChordSuggestionTest, FunctionalCompatibilityBorrowedChord)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  ChordKey     bb{ MusicalNote::ASharp, ChordType::Major };
  ChordKey     c{ MusicalNote::C, ChordType::Major };
  EXPECT_NEAR (score_functional_compatibility (bb, c, c_major), 0.85f, 0.05f);
}

// ============================================================================
// Candidate generation
// ============================================================================

TEST (ChordSuggestionTest, GenerateDiatonicTriadsCMajor)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  auto         candidates =
    generate_candidates (c_major, ChordCandidateCategory::DiatonicTriads);

  int diatonic_count = 0;
  for (const auto &c : candidates)
    if (c.type == CandidateType::DiatonicTriad)
      ++diatonic_count;
  EXPECT_EQ (diatonic_count, 7);

  EXPECT_EQ (candidates[0].chord_key.root_note, MusicalNote::C);
  EXPECT_EQ (candidates[0].chord_key.type, ChordType::Major);
}

TEST (ChordSuggestionTest, GenerateDiatonicTriadsAMinor)
{
  MusicalScale a_minor (MusicalScale::ScaleType::Minor, MusicalNote::A);
  auto         candidates =
    generate_candidates (a_minor, ChordCandidateCategory::DiatonicTriads);

  int diatonic_count = 0;
  for (const auto &c : candidates)
    if (c.type == CandidateType::DiatonicTriad)
      ++diatonic_count;
  EXPECT_EQ (diatonic_count, 7);

  EXPECT_EQ (candidates[0].chord_key.root_note, MusicalNote::A);
  EXPECT_EQ (candidates[0].chord_key.type, ChordType::Minor);
}

// The LeadingTone category is independent — requesting only DiatonicTriads
// must NOT produce a leading-tone candidate.
TEST (ChordSuggestionTest, GenerateDiatonicTriadsExcludesLeadingTone)
{
  MusicalScale a_minor (MusicalScale::ScaleType::Minor, MusicalNote::A);
  auto         candidates =
    generate_candidates (a_minor, ChordCandidateCategory::DiatonicTriads);

  for (const auto &c : candidates)
    {
      if (c.type == CandidateType::LeadingTone)
        FAIL ()
          << "Leading tone generated when only DiatonicTriads "
             "was requested";
    }
}

// Requesting the LeadingTone flag (alone or combined) yields the raised
// leading-tone diminished in a minor key.
TEST (ChordSuggestionTest, GenerateLeadingToneWhenRequested)
{
  MusicalScale a_minor (MusicalScale::ScaleType::Minor, MusicalNote::A);
  auto         candidates =
    generate_candidates (a_minor, ChordCandidateCategory::LeadingTone);

  bool has_leading_tone = false;
  for (const auto &c : candidates)
    {
      if (c.type == CandidateType::LeadingTone)
        {
          EXPECT_EQ (c.chord_key.root_note, MusicalNote::GSharp);
          EXPECT_EQ (c.chord_key.type, ChordType::Diminished);
          has_leading_tone = true;
        }
    }
  EXPECT_TRUE (has_leading_tone);
}

TEST (ChordSuggestionTest, GenerateSecondaryDominantsCMajor)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  auto         candidates = generate_candidates (
    c_major,
    ChordCandidateCategory::DiatonicTriads
      | ChordCandidateCategory::SecondaryDominants);

  std::vector<MusicalNote> dom_roots;
  for (const auto &c : candidates)
    {
      if (c.type == CandidateType::SecondaryDominant)
        dom_roots.push_back (c.chord_key.root_note);
    }
  std::sort (dom_roots.begin (), dom_roots.end ());
  EXPECT_EQ (dom_roots.size (), 5);
}

// DiatonicTriads flag respected — requesting only SecondaryDominants
// should NOT produce diatonic triads.
TEST (ChordSuggestionTest, GenerateOnlySecondaryDominants)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  auto         candidates =
    generate_candidates (c_major, ChordCandidateCategory::SecondaryDominants);

  for (const auto &c : candidates)
    {
      if (c.type == CandidateType::DiatonicTriad)
        FAIL ()
          << "Diatonic triad generated when only SecondaryDominants "
             "was requested";
    }
  EXPECT_FALSE (candidates.empty ());
}

// Non-heptatonic scales return empty candidates.
TEST (ChordSuggestionTest, GenerateFromChromaticReturnsEmpty)
{
  MusicalScale chromatic (MusicalScale::ScaleType::Chromatic, MusicalNote::C);
  auto         candidates =
    generate_candidates (chromatic, ChordCandidateCategory::DiatonicTriads);
  EXPECT_TRUE (candidates.empty ());
}

TEST (ChordSuggestionTest, GenerateFromPentatonicReturnsEmpty)
{
  MusicalScale pentatonic (
    MusicalScale::ScaleType::MajorPentatonic, MusicalNote::C);
  auto candidates =
    generate_candidates (pentatonic, ChordCandidateCategory::DiatonicTriads);
  EXPECT_TRUE (candidates.empty ());
}

// ============================================================================
// Conditional scoring
// ============================================================================

TEST (ChordSuggestionTest, LeadingToneResolutionToTonic)
{
  MusicalScale a_minor (MusicalScale::ScaleType::Minor, MusicalNote::A);
  ChordKey     g_sharp_dim{ MusicalNote::GSharp, ChordType::Diminished };
  ChordKey     a_minor_chord{ MusicalNote::A, ChordType::Minor };
  EXPECT_NEAR (
    score_leading_tone_resolution (g_sharp_dim, a_minor_chord, a_minor), 1.0f,
    0.05f);
}

TEST (ChordSuggestionTest, LeadingToneResolutionNonTonic)
{
  MusicalScale a_minor (MusicalScale::ScaleType::Minor, MusicalNote::A);
  ChordKey     g_sharp_dim{ MusicalNote::GSharp, ChordType::Diminished };
  ChordKey     c_major{ MusicalNote::C, ChordType::Major };
  float score = score_leading_tone_resolution (g_sharp_dim, c_major, a_minor);
  EXPECT_LT (score, 0.3f);
}

TEST (ChordSuggestionTest, LeadingToneResolutionNotLeadingTone)
{
  MusicalScale a_minor (MusicalScale::ScaleType::Minor, MusicalNote::A);
  ChordKey     c_major{ MusicalNote::C, ChordType::Major };
  ChordKey     d_minor{ MusicalNote::D, ChordType::Minor };
  EXPECT_NEAR (
    score_leading_tone_resolution (c_major, d_minor, a_minor), 0.5f, 0.001f);
}

// Dominant 7th resolves by diatonic step (half OR whole step).
TEST (ChordSuggestionTest, SeventhResolutionDominantToTonic)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  ChordKey     g7{ MusicalNote::G, ChordType::Major, ChordAccent::Seventh };
  ChordKey     c{ MusicalNote::C, ChordType::Major };
  // G7's 7th (F) resolves to E (half step down) — in C major.
  EXPECT_NEAR (score_seventh_resolution (g7, c, c_major), 1.0f, 0.001f);
}

TEST (ChordSuggestionTest, SeventhResolutionDominantToMinorWholeStep)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  ChordKey     e7{ MusicalNote::E, ChordType::Major, ChordAccent::Seventh };
  ChordKey     a_minor{ MusicalNote::A, ChordType::Minor };
  // E7's 7th (D) resolves to C (whole step down) — diatonic in C major.
  EXPECT_NEAR (score_seventh_resolution (e7, a_minor, c_major), 1.0f, 0.001f);
}

TEST (ChordSuggestionTest, SeventhResolutionSecondaryDominantWholeStep)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  ChordKey     a7{ MusicalNote::A, ChordType::Major, ChordAccent::Seventh };
  ChordKey     d_minor{ MusicalNote::D, ChordType::Minor };
  // A7's 7th (G) resolves to F (whole step down) — diatonic in C major.
  EXPECT_NEAR (score_seventh_resolution (a7, d_minor, c_major), 1.0f, 0.001f);
}

TEST (ChordSuggestionTest, SeventhResolutionUnresolved)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  ChordKey     g7{ MusicalNote::G, ChordType::Major, ChordAccent::Seventh };
  ChordKey     d_minor{ MusicalNote::D, ChordType::Minor };
  // G7's 7th (F) resolves to E — D minor does not contain E. So resolution note
  // E is not in Dm → score 0.0.
  EXPECT_NEAR (score_seventh_resolution (g7, d_minor, c_major), 0.0f, 0.001f);
}

// Major 7th is neutral (consonant, no resolution needed).
TEST (ChordSuggestionTest, SeventhResolutionMajorSeventhNeutral)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  ChordKey cmaj7{ MusicalNote::C, ChordType::Major, ChordAccent::MajorSeventh };
  ChordKey f{ MusicalNote::F, ChordType::Major };
  EXPECT_NEAR (score_seventh_resolution (cmaj7, f, c_major), 0.5f, 0.001f);
}

// ============================================================================
// Pipeline integration
// ============================================================================

TEST (ChordSuggestionTest, SuggestFromCmajorC)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  ChordKey     c{ MusicalNote::C, ChordType::Major };
  auto         suggestions =
    suggest_next_chords (c, c_major, ChordCandidateCategory::DiatonicTriads, 3);

  EXPECT_EQ (suggestions.size (), 3);
  std::set<MusicalNote> top3_roots;
  for (const auto &s : suggestions)
    top3_roots.insert (s.chord_key.root_note);
  EXPECT_TRUE (top3_roots.count (MusicalNote::F));
  EXPECT_TRUE (top3_roots.count (MusicalNote::G));
}

TEST (ChordSuggestionTest, SuggestFromCmajorG)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);
  ChordKey     g{ MusicalNote::G, ChordType::Major };
  auto         suggestions =
    suggest_next_chords (g, c_major, ChordCandidateCategory::DiatonicTriads, 1);

  ASSERT_FALSE (suggestions.empty ());
  EXPECT_EQ (suggestions[0].chord_key.root_note, MusicalNote::C);
}

TEST (ChordSuggestionTest, SuggestFromAminorEmajor)
{
  MusicalScale a_minor (MusicalScale::ScaleType::Minor, MusicalNote::A);
  ChordKey     e_major{ MusicalNote::E, ChordType::Major };
  auto         suggestions = suggest_next_chords (
    e_major, a_minor, ChordCandidateCategory::DiatonicTriads, 1);

  ASSERT_FALSE (suggestions.empty ());
  EXPECT_EQ (suggestions[0].chord_key.root_note, MusicalNote::A);
  EXPECT_EQ (suggestions[0].chord_key.type, ChordType::Minor);
}

} // namespace zrythm::dsp::chords
