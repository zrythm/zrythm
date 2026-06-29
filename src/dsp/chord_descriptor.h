// SPDX-FileCopyrightText: © 2018-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

#include <boost/container/static_vector.hpp>
#include <nlohmann/json_fwd.hpp>

namespace zrythm::utils
{
class Utf8String;
}

namespace zrythm::dsp::chords
{
Q_NAMESPACE
QML_ELEMENT

enum class MusicalNote : std::uint8_t
{
  C = 0,
  CSharp,
  D,
  DSharp,
  E,
  F,
  FSharp,
  G,
  GSharp,
  A,
  ASharp,
  B
};
Q_ENUM_NS (MusicalNote)

/**
 * Chord type.
 */
enum class ChordType : std::uint8_t
{
  None,
  Major,
  Minor,
  Diminished,
  SuspendedFourth,
  SuspendedSecond,
  Augmented,
  Custom,
};
Q_ENUM_NS (ChordType)

/**
 * Chord accents.
 */
enum class ChordAccent : std::uint8_t
{
  None,
  /** b7 is 10 semitones from chord root, or 9
   * if the chord is diminished. */
  Seventh,
  /** Maj7 is 11 semitones from the root. */
  MajorSeventh,
  /* NOTE: all accents below assume Seventh */
  /** 13 semitones. */
  FlatNinth,
  /** 14 semitones. */
  Ninth,
  /** 15 semitones. */
  SharpNinth,
  /** 17 semitones. */
  Eleventh,
  /** 6 and 18 semitones. */
  FlatFifthSharpEleventh,
  /** 8 and 16 semitones. */
  SharpFifthFlatThirteenth,
  /** 9 and 21 semitones. */
  SixthThirteenth,
};
Q_ENUM_NS (ChordAccent)

/**
 * @brief Transposes a @ref MusicalNote by one semitone up or down with
 * wrap-around within a single octave (0-11).
 *
 * @param note The note to transpose.
 * @param up True to transpose up, false to transpose down.
 */
constexpr MusicalNote
transpose_note (MusicalNote note, bool up) noexcept
{
  return static_cast<MusicalNote> (
    (static_cast<int> (note) + (up ? 1 : 11)) % 12);
}

} // namespace zrythm::dsp::chords

namespace zrythm::dsp
{

using MusicalNote = chords::MusicalNote;
using ChordType = chords::ChordType;
using ChordAccent = chords::ChordAccent;

/**
 * Describes a musical chord by its root note, type, accent, inversion,
 * and optional bass note.
 *
 * The chord's note content is defined by intervals computed on demand from
 * (type, accent) — no cached derived state.
 */
class ChordDescriptor : public QObject
{
  Q_OBJECT
  QML_ELEMENT

public:
  static constexpr std::uint8_t kDefaultBasePitch = 36; // C2

  explicit ChordDescriptor (QObject * parent = nullptr) : QObject (parent) { }

  ChordDescriptor (
    MusicalNote                root,
    ChordType                  type,
    ChordAccent                accent = ChordAccent::None,
    int                        inversion = 0,
    std::optional<MusicalNote> bass = std::nullopt,
    QObject *                  parent = nullptr)
      : QObject (parent), root_note_ (root), type_ (type), accent_ (accent),
        inversion_ (std::clamp (inversion, minInversion (), maxInversion ())),
        bass_note_ (bass)
  {
  }

  // ========================================================================
  // QML Interface
  // ========================================================================

  Q_PROPERTY (
    zrythm::dsp::chords::MusicalNote rootNote READ rootNote WRITE setRootNote
      NOTIFY rootNoteChanged)
  Q_PROPERTY (bool hasBass READ hasBass WRITE setHasBass NOTIFY hasBassChanged)
  Q_PROPERTY (
    zrythm::dsp::chords::MusicalNote bassNote READ bassNote WRITE setBassNote
      NOTIFY bassNoteChanged)
  Q_PROPERTY (
    zrythm::dsp::chords::ChordType chordType READ chordType WRITE setChordType
      NOTIFY chordTypeChanged)
  Q_PROPERTY (
    zrythm::dsp::chords::ChordAccent chordAccent READ chordAccent WRITE
      setChordAccent NOTIFY chordAccentChanged)
  Q_PROPERTY (
    int inversion READ inversion WRITE setInversion NOTIFY inversionChanged)
  Q_PROPERTY (QString displayName READ displayName NOTIFY displayNameChanged)

  MusicalNote rootNote () const { return root_note_; }
  void        setRootNote (MusicalNote v)
  {
    if (root_note_ != v)
      {
        root_note_ = v;
        Q_EMIT rootNoteChanged (v);
        Q_EMIT displayNameChanged ();
        Q_EMIT changed ();
      }
  }

  bool hasBass () const { return bass_note_.has_value (); }
  void setHasBass (bool v)
  {
    if (hasBass () != v)
      {
        bass_note_ = v ? std::optional<MusicalNote> (root_note_) : std::nullopt;
        Q_EMIT hasBassChanged (v);
        Q_EMIT displayNameChanged ();
        Q_EMIT changed ();
      }
  }

  MusicalNote bassNote () const { return bass_note_.value_or (MusicalNote::C); }
  void        setBassNote (MusicalNote v)
  {
    bool had = hasBass ();
    if (!bass_note_.has_value () || *bass_note_ != v)
      {
        bass_note_ = v;
        if (!had)
          Q_EMIT hasBassChanged (true);
        Q_EMIT bassNoteChanged (v);
        Q_EMIT displayNameChanged ();
        Q_EMIT changed ();
      }
  }

  ChordType chordType () const { return type_; }
  void      setChordType (ChordType v)
  {
    if (type_ != v)
      {
        type_ = v;
        Q_EMIT chordTypeChanged (v);
        Q_EMIT displayNameChanged ();
        Q_EMIT changed ();
      }
  }

  ChordAccent chordAccent () const { return accent_; }
  void        setChordAccent (ChordAccent v)
  {
    if (accent_ != v)
      {
        accent_ = v;
        Q_EMIT chordAccentChanged (v);
        Q_EMIT displayNameChanged ();
        Q_EMIT changed ();
      }
  }

  int  inversion () const { return inversion_; }
  void setInversion (int v)
  {
    v = std::clamp (v, minInversion (), maxInversion ());
    if (inversion_ != v)
      {
        inversion_ = v;
        Q_EMIT inversionChanged (v);
        Q_EMIT displayNameChanged ();
        Q_EMIT changed ();
      }
  }

  QString displayName () const;

  Q_SIGNAL void rootNoteChanged (zrythm::dsp::chords::MusicalNote note);
  Q_SIGNAL void hasBassChanged (bool has);
  Q_SIGNAL void bassNoteChanged (zrythm::dsp::chords::MusicalNote note);
  Q_SIGNAL void chordTypeChanged (zrythm::dsp::chords::ChordType type);
  Q_SIGNAL void chordAccentChanged (zrythm::dsp::chords::ChordAccent accent);
  Q_SIGNAL void inversionChanged (int inversion);
  Q_SIGNAL void displayNameChanged ();
  Q_SIGNAL void changed ();

  // ========================================================================
  // Queries
  // ========================================================================

  Q_INVOKABLE bool isKeyInChord (MusicalNote key) const;
  Q_INVOKABLE bool isKeyBass (MusicalNote key) const;

  static constexpr size_t kMaxIntervals = 12;

  /**
   * Returns the semitone intervals from root for the given type and accent.
   * e.g., Major = {0, 4, 7}, Minor7 = {0, 3, 7, 10}
   */
  static boost::container::static_vector<int, kMaxIntervals>
  get_intervals_for_type_and_accent (ChordType type, ChordAccent accent);

  /**
   * Returns the semitone intervals from root for the current type and accent.
   * e.g., Major = {0, 4, 7}, Minor7 = {0, 3, 7, 10}
   */
  boost::container::static_vector<int, kMaxIntervals> getIntervals () const;

  /**
   * Stack-allocated container for chord MIDI pitches.
   * Large enough for any chord (triad + accent + bass).
   */
  struct ChordPitches
  {
    static constexpr size_t               kMaxPitches = 8;
    std::array<std::uint8_t, kMaxPitches> data{};
    size_t                                count = 0;

    auto         begin () const { return data.begin (); }
    auto         end () const { return data.begin () + count; }
    bool         empty () const { return count == 0; }
    size_t       size () const { return count; }
    std::uint8_t operator[] (size_t i) const { return data[i]; }

    friend bool operator== (const ChordPitches &, const ChordPitches &);
  };

  /**
   * Returns the MIDI pitches for this chord's voicing.
   * Applies octave placement, bass note, and inversion.
   * Stack-allocated, realtime-safe.
   */
  ChordPitches
  getMidiPitches (std::uint8_t base_pitch = kDefaultBasePitch) const;

  int maxInversion () const;
  int minInversion () const { return -maxInversion (); }

  // ========================================================================
  // String utilities
  // ========================================================================

  static utils::Utf8String chord_type_to_string (ChordType type);
  static utils::Utf8String chord_accent_to_string (ChordAccent accent);
  static utils::Utf8String note_to_string (MusicalNote note);
  utils::Utf8String        to_string () const;

  bool isEquivalent (const ChordDescriptor &other) const;

private:
  friend void to_json (nlohmann::json &j, const ChordDescriptor &c);
  friend void from_json (const nlohmann::json &j, ChordDescriptor &c);

  /** Root note. */
  MusicalNote root_note_ = MusicalNote::C;

  /** Chord type. */
  ChordType type_ = ChordType::None;

  /**
   * Chord accent.
   *
   * Does not apply to custom chords.
   */
  ChordAccent accent_ = ChordAccent::None;

  /**
   * 0 no inversion,
   * less than 0 highest note(s) drop an octave,
   * greater than 0 lowest note(s) receive an octave.
   */
  int inversion_ = 0;

  /** Bass note 1 octave below. */
  std::optional<MusicalNote> bass_note_;

  /**
   * Custom chord intervals (semitone offsets from root).
   * Only used when type_ is ChordType::Custom.
   *
   * TODO: Implement custom chord editing. When set, these intervals
   * override the standard type+accent computation in getIntervals().
   */
  std::optional<boost::container::static_vector<int, kMaxIntervals>>
    custom_intervals_;
};

} // namespace zrythm::dsp
