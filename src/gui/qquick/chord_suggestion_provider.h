// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/chord_descriptor.h"
#include "dsp/chord_suggestion.h"
#include "dsp/musical_scale.h"

#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui::qquick
{

class ChordSuggestionProvider : public QObject
{
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY (
    zrythm::dsp::ChordDescriptor * currentChord READ currentChord WRITE
      setCurrentChord NOTIFY currentChordChanged)
  Q_PROPERTY (
    zrythm::dsp::MusicalScale * currentScale READ currentScale WRITE
      setCurrentScale NOTIFY currentScaleChanged)
  Q_PROPERTY (
    bool includeSevenths READ includeSevenths WRITE setIncludeSevenths NOTIFY
      categoriesChanged)
  Q_PROPERTY (
    bool includeSecondaryDominants READ includeSecondaryDominants WRITE
      setIncludeSecondaryDominants NOTIFY categoriesChanged)
  Q_PROPERTY (
    bool includeBorrowedChords READ includeBorrowedChords WRITE
      setIncludeBorrowedChords NOTIFY categoriesChanged)
  Q_PROPERTY (
    QVariantList topSuggestions READ topSuggestions NOTIFY suggestionsChanged)
  Q_PROPERTY (
    int maxSuggestions READ maxSuggestions WRITE setMaxSuggestions NOTIFY
      maxSuggestionsChanged)

public:
  explicit ChordSuggestionProvider (QObject * parent = nullptr);

  zrythm::dsp::ChordDescriptor * currentChord () const
  {
    return current_chord_.data ();
  }
  void setCurrentChord (zrythm::dsp::ChordDescriptor * chord);

  /// Clears suggestions (called by auto-clear timer).
  Q_INVOKABLE void clearSuggestions ();

  /// Starts the auto-clear countdown (call when the pad is released).
  Q_INVOKABLE void scheduleClear ();

  zrythm::dsp::MusicalScale * currentScale () const
  {
    return current_scale_.data ();
  }
  void setCurrentScale (zrythm::dsp::MusicalScale * scale);

  bool includeSevenths () const { return include_sevenths_; }
  void setIncludeSevenths (bool v);

  bool includeSecondaryDominants () const
  {
    return include_secondary_dominants_;
  }
  void setIncludeSecondaryDominants (bool v);

  bool includeBorrowedChords () const { return include_borrowed_chords_; }
  void setIncludeBorrowedChords (bool v);

  QVariantList topSuggestions () const { return top_suggestions_; }

  int  maxSuggestions () const { return max_suggestions_; }
  void setMaxSuggestions (int v);

  Q_INVOKABLE float scoreForChord (zrythm::dsp::ChordDescriptor * chord) const;

  Q_SIGNAL void currentChordChanged ();
  Q_SIGNAL void currentScaleChanged ();
  Q_SIGNAL void categoriesChanged ();
  Q_SIGNAL void suggestionsChanged ();
  Q_SIGNAL void maxSuggestionsChanged ();

private:
  void recalculate ();

  QPointer<zrythm::dsp::ChordDescriptor> current_chord_;
  QPointer<zrythm::dsp::MusicalScale>    current_scale_;
  bool                                   include_sevenths_ = false;
  bool                                   include_secondary_dominants_ = false;
  bool                                   include_borrowed_chords_ = false;

  std::vector<zrythm::dsp::chords::ChordSuggestion> cached_suggestions_;
  QVariantList                                      top_suggestions_;
  int                                               max_suggestions_ = 5;
  QTimer                                            auto_clear_timer_;
};

} // namespace zrythm::gui::qquick
