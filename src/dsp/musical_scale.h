// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/chord_descriptor.h"

#include <QtQmlIntegration>

namespace zrythm::dsp
{

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
    MusicalNote rootKey READ rootKey WRITE setRootKey NOTIFY rootKeyChanged)
  QML_ELEMENT

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

public:
  MusicalScale (QObject * parent = nullptr) : QObject (parent) { }
  MusicalScale (ScaleType type, MusicalNote root, QObject * parent = nullptr)
      : MusicalScale (parent)
  {
    setScaleType (type);
    setRootKey (root);
  }
  Z_DISABLE_COPY_MOVE (MusicalScale)
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

  Q_INVOKABLE QString toString () const { return to_string ().to_qstring (); }

  /**
   * Returns if the given key is in the given MusicalScale.
   *
   * @param note A note inside a single octave (0-11).
   */
  Q_INVOKABLE bool containsNote (MusicalNote note) const;

  // ========================================================================

  /**
   * Returns the notes in the given scale.
   *
   * @param ascending Whether to get the notes when ascending or descending
   * (some scales have different notes when rising/falling).
   */
  static const bool * get_notes_for_type (ScaleType type, bool ascending);

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

  static utils::Utf8String type_to_string (ScaleType type);

  /**
   * Prints the MusicalScale to a string.
   */
  utils::Utf8String to_string () const;

  /**
   * Returns if all of the chord's notes are in the scale.
   */
  bool contains_chord (const ChordDescriptor &chord) const;

  /**
   * Returns if the accent is in the scale.
   */
  bool is_accent_in_scale (
    MusicalNote chord_root,
    ChordType   type,
    ChordAccent chord_accent) const;

  bool is_same_scale (const MusicalScale &other) const
  {
    return type_ == other.type_ && root_key_ == other.root_key_;
  }

private:
  friend void init_from (
    MusicalScale          &obj,
    const MusicalScale    &other,
    utils::ObjectCloneType clone_type)
  {
    obj.type_ = other.type_;
    obj.root_key_ = other.root_key_;
  }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE (MusicalScale, type_, root_key_)

private:
  /** Identification of the scale (e.g. AEOLIAN). */
  ScaleType type_ = ScaleType::Aeolian;

  /** Root key of the scale. */
  MusicalNote root_key_ = MusicalNote::A;

  BOOST_DESCRIBE_CLASS (MusicalScale, (), (), (), (type_, root_key_))
};

} // namespace zrythm::dsp
