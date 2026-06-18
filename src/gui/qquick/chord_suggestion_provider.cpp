// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "gui/qquick/chord_suggestion_provider.h"

namespace zrythm::gui::qquick
{

ChordSuggestionProvider::ChordSuggestionProvider (QObject * parent)
    : QObject (parent)
{
  auto_clear_timer_.setSingleShot (true);
  auto_clear_timer_.setInterval (3000); // 3 seconds
  QObject::connect (&auto_clear_timer_, &QTimer::timeout, this, [this] () {
    clearSuggestions ();
  });
}

void
ChordSuggestionProvider::clearSuggestions ()
{
  current_chord_ = nullptr;
  cached_suggestions_.clear ();
  top_suggestions_.clear ();
  Q_EMIT currentChordChanged ();
  Q_EMIT suggestionsChanged ();
}

void
ChordSuggestionProvider::scheduleClear ()
{
  auto_clear_timer_.start ();
}

void
ChordSuggestionProvider::setCurrentChord (zrythm::dsp::ChordDescriptor * chord)
{
  // Stop auto-clear timer while a pad is held.
  auto_clear_timer_.stop ();

  if (current_chord_ != chord)
    {
      current_chord_ = chord;
      Q_EMIT currentChordChanged ();
      recalculate ();
    }
}

void
ChordSuggestionProvider::setCurrentScale (zrythm::dsp::MusicalScale * scale)
{
  if (current_scale_ != scale)
    {
      current_scale_ = scale;
      Q_EMIT currentScaleChanged ();
      recalculate ();
    }
}

void
ChordSuggestionProvider::setIncludeSevenths (bool v)
{
  if (include_sevenths_ != v)
    {
      include_sevenths_ = v;
      Q_EMIT categoriesChanged ();
      recalculate ();
    }
}

void
ChordSuggestionProvider::setIncludeSecondaryDominants (bool v)
{
  if (include_secondary_dominants_ != v)
    {
      include_secondary_dominants_ = v;
      Q_EMIT categoriesChanged ();
      recalculate ();
    }
}

void
ChordSuggestionProvider::setIncludeBorrowedChords (bool v)
{
  if (include_borrowed_chords_ != v)
    {
      include_borrowed_chords_ = v;
      Q_EMIT categoriesChanged ();
      recalculate ();
    }
}

void
ChordSuggestionProvider::setMaxSuggestions (int v)
{
  if (max_suggestions_ != v)
    {
      max_suggestions_ = v;
      Q_EMIT maxSuggestionsChanged ();
      recalculate ();
    }
}

float
ChordSuggestionProvider::scoreForChord (
  zrythm::dsp::ChordDescriptor * chord) const
{
  if (!current_chord_ || !chord)
    return -1.0f;

  namespace ch = zrythm::dsp::chords;
  ch::ChordKey key{
    chord->rootNote (), chord->chordType (), chord->chordAccent ()
  };

  // The currently-pressed chord is excluded from candidates — return -1.0
  // so the UI shows it as neutral (not red/0.0).
  if (
    current_chord_->rootNote () == key.root_note
    && current_chord_->chordType () == key.type
    && current_chord_->chordAccent () == key.accent)
    return -1.0f;

  // If no suggestions have been calculated (e.g., no scale available),
  // return neutral so pads don't turn red.
  if (cached_suggestions_.empty ())
    return -1.0f;

  for (const auto &s : cached_suggestions_)
    {
      if (s.chord_key == key)
        return s.overall_score;
    }
  return -1.0f;
}

void
ChordSuggestionProvider::recalculate ()
{
  if (!current_chord_ || !current_scale_)
    {
      cached_suggestions_.clear ();
      top_suggestions_.clear ();
      Q_EMIT suggestionsChanged ();
      return;
    }

  namespace ch = zrythm::dsp::chords;
  auto cats =
    ch::ChordCandidateCategory::DiatonicTriads
    | ch::ChordCandidateCategory::LeadingTone;
  if (include_sevenths_)
    cats |= ch::ChordCandidateCategory::DiatonicSevenths;
  if (include_secondary_dominants_)
    cats |= ch::ChordCandidateCategory::SecondaryDominants;
  if (include_borrowed_chords_)
    cats |= ch::ChordCandidateCategory::BorrowedChords;

  ch::ChordKey current_key{
    current_chord_->rootNote (), current_chord_->chordType (),
    current_chord_->chordAccent ()
  };

  cached_suggestions_ =
    ch::suggest_next_chords (current_key, *current_scale_, cats, 0);

  top_suggestions_.clear ();
  int top_n =
    std::min (max_suggestions_, static_cast<int> (cached_suggestions_.size ()));
  for (int i = 0; i < top_n; ++i)
    {
      const auto &s = cached_suggestions_[i];
      QVariantMap entry;
      entry["chordName"] = s.chord_key.make_descriptor ().displayName ();
      entry["score"] = s.overall_score;
      entry["rank"] = s.rank;
      entry["rootNote"] = static_cast<int> (s.chord_key.root_note);
      entry["chordType"] = static_cast<int> (s.chord_key.type);
      entry["chordAccent"] = static_cast<int> (s.chord_key.accent);
      top_suggestions_.append (entry);
    }

  Q_EMIT suggestionsChanged ();
}

} // namespace zrythm::gui::qquick
