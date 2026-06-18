// SPDX-FileCopyrightText: © 2018-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <array>

#include "dsp/chord_descriptor.h"
#include "utils/icloneable.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include <boost/describe.hpp>
#include <nlohmann/json_fwd.hpp>

namespace zrythm::dsp
{
using namespace std::string_view_literals;

/**
 * Musical scale descriptor.
 *
 * @see https://pianoscales.org/
 */
class MusicalScale : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    ScaleType scaleType READ scaleType WRITE setScaleType NOTIFY scaleTypeChanged)
  Q_PROPERTY (
    zrythm::dsp::chords::MusicalNote rootKey READ rootKey WRITE setRootKey
      NOTIFY rootKeyChanged)
  QML_ELEMENT
  QML_EXTENDED_NAMESPACE (zrythm::dsp::chords)

public:
  /**
   * Scale type (name) eg Aeolian.
   */
  enum class ScaleType : std::uint8_t
  {
    /** All keys. */
    Chromatic,

    /* --- popular scales --- */

    Major,

    /** Natural minor. */
    Minor,

    /** Major (same as SCALE_MAJOR). */
    Ionian,

    Dorian,
    Phrygian,
    Lydian,
    Mixolydian,

    /** Natural minor (same as @ref Minor). */
    Aeolian,

    Locrian,
    MelodicMinor,
    HarmonicMinor,
    WholeTone,
    MajorPentatonic,
    MinorPentatonic,
    OctatonicHalfWhole,
    OctatonicWholeHalf,

    /* --- exotic scales --- */

    /** Lydian dominant. */
    Acoustic,

    HarmonicMajor,
    PhrygianDominant,
    MajorLocrian,
    Algerian,
    Augmented,
    DoubleHarmonic,
    Chinese,
    Diminished,
    DominantDiminished,
    Egyptian,
    EightToneSpanish,
    Enigmatic,
    Geez,
    Hindu,
    Hirajoshi,
    HungarianGypsy,
    Insen,
    NeapolitanMajor,
    NeapolitanMinor,
    Oriental,
    RomanianMinor,
    Altered,
    Maqam,
    Yo,
    BebopLocrian,
    BebopDominant,
    BebopMajor,
    SuperLocrian,
    EnigmaticMinor,
    Composite,
    Bhairav,
    HungarianMinor,
    Persian,
    Iwato,
    Kumoi,
    Pelog,
    Prometheus,
    PrometheusNeapolitan,
    PrometheusLiszt,
    Balinese,
    Ragatodi,
    Japanese1,
    Japanese2,

    /* --- TODO unimplemented --- */

    Blues,
    Flamenco,
    Gypsy,
    HalfDiminished,
    In,
    Istrian,
    LydianAugmented,
    Tritone,
    UkranianDorian,
  };
  Q_ENUM (ScaleType)

public:
  MusicalScale (QObject * parent = nullptr) : QObject (parent) { }
  MusicalScale (ScaleType type, MusicalNote root, QObject * parent = nullptr)
      : MusicalScale (parent)
  {
    setScaleType (type);
    setRootKey (root);
  }
  Q_DISABLE_COPY_MOVE (MusicalScale)
  ~MusicalScale () override = default;

  // ========================================================================
  // QML Interface
  // ========================================================================

  auto scaleType () const { return type_; }
  void setScaleType (ScaleType type)
  {
    type = std::clamp (type, ScaleType::Chromatic, ScaleType::UkranianDorian);
    if (type_ != type)
      {
        type_ = type;
        Q_EMIT scaleTypeChanged (type);
      }
  }
  Q_SIGNAL void scaleTypeChanged (ScaleType type);

  auto rootKey () const { return root_key_; }
  void setRootKey (MusicalNote root_key)
  {
    root_key = std::clamp (root_key, MusicalNote::C, MusicalNote::B);
    if (root_key_ != root_key)
      {
        root_key_ = root_key;
        Q_EMIT rootKeyChanged (root_key);
      }
  }
  Q_SIGNAL void rootKeyChanged (MusicalNote root_key);

  Q_INVOKABLE QString toString () const;

  /**
   * Returns if the given key is in the given MusicalScale.
   *
   * @param note A note inside a single octave (0-11).
   */
  Q_INVOKABLE bool containsNote (MusicalNote note) const;

  Q_INVOKABLE static bool
  scaleContainsNote (ScaleType type, MusicalNote rootKey, MusicalNote note);

  Q_INVOKABLE static QString noteToString (MusicalNote note);

  /**
   * @brief Returns the display names of all 12 notes in a single octave.
   *
   * Convenience for QML consumers that need the full list in a single call
   * instead of invoking @ref noteToString 12 times.
   */
  Q_INVOKABLE static QStringList allNoteNames ();

  Q_INVOKABLE static QVariantList availableScaleTypes ();
  Q_INVOKABLE static QVariantList availableScaleTypesExotic ();

  /**
   * Scales with pre-defined triad data in @ref get_triad_types_for_type.
   * Used by the chord pad to populate chords from a scale.
   */
  Q_INVOKABLE static QVariantList availableScaleTypesWithTriads ();

  /**
   * Returns a QVariantList of diatonic chord descriptors for the given scale,
   * each as {display, rootNote, chordType}.
   */
  Q_INVOKABLE static QVariantList
  getDiatonicChords (ScaleType type, MusicalNote rootKey);

  // ========================================================================

  /**
   * Returns the notes in the given scale.
   *
   * @param ascending Whether to get the notes when ascending or descending
   * (some scales have different notes when rising/falling).
   */
  static std::array<bool, 12>
  get_notes_for_type (ScaleType type, bool ascending);

  /**
   * Returns the triads in the given scale.
   *
   * There will be as many chords are enabled notes in the scale, and the rest
   * of the array will be filled with CHORD_TYPE_NONE.
   *
   * @param ascending Whether to get the triads when ascending or descending
   * (some scales have different triads when rising/falling).
   */
  static std::array<ChordType, 12>
  get_triad_types_for_type (ScaleType type, bool ascending);

  /**
   * @brief A diatonic triad: the root note and chord type for a scale degree.
   */
  struct DiatonicTriad
  {
    MusicalNote root_note;
    ChordType   chord_type;
  };

  /**
   * @brief Returns the diatonic triads for the given scale and root note.
   *
   * Enumerates the enabled notes in the scale, pairs each with its
   * corresponding triad type, and computes the absolute root note.
   * Returns an empty result if the scale type is unimplemented.
   *
   * @param type The scale type.
   * @param root_note The root note of the scale.
   */
  static boost::container::static_vector<DiatonicTriad, 12>
  get_diatonic_triads (ScaleType type, MusicalNote root_note);

  static utils::Utf8String type_to_string (ScaleType type);

  /**
   * Prints the MusicalScale to a string.
   */
  utils::Utf8String to_string () const;

  /**
   * Returns if all of the chord's notes are in the scale.
   */
  Q_INVOKABLE bool containsChord (
    MusicalNote root,
    ChordType   type,
    ChordAccent accent = ChordAccent::None) const;

  /**
   * Returns if the accent is in the scale.
   */
  Q_INVOKABLE bool isAccentInScale (
    MusicalNote chord_root,
    ChordType   type,
    ChordAccent chord_accent) const;

  bool is_same_scale (const MusicalScale &other) const
  {
    return type_ == other.type_ && root_key_ == other.root_key_;
  }

private:
  static constexpr auto kRootKeyKey = "rootKey"sv;
  static constexpr auto kTypeKey = "type"sv;

  friend void init_from (
    MusicalScale          &obj,
    const MusicalScale    &other,
    utils::ObjectCloneType clone_type);

  friend void to_json (nlohmann::json &j, const MusicalScale &s);
  friend void from_json (const nlohmann::json &j, MusicalScale &s);

private:
  /** Identification of the scale (e.g. AEOLIAN). */
  ScaleType type_ = ScaleType::Aeolian;

  /** Root key of the scale. */
  MusicalNote root_key_ = MusicalNote::A;

  BOOST_DESCRIBE_CLASS (MusicalScale, (), (), (), (type_, root_key_))
};

} // namespace zrythm::dsp
