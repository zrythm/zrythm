// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>

#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/bounded_object.h"
#include "structure/arrangement/muteable_object.h"

namespace zrythm::structure::arrangement
{

template <typename T>
concept RangeOfMidiNotePointers = RangeOf<T, MidiNote *>;

/**
 * A MIDI note inside a Region shown in the piano roll.
 */
class MidiNote : public ArrangerObject
{
  Q_OBJECT
  Q_PROPERTY (ArrangerObjectBounds * bounds READ bounds CONSTANT)
  Q_PROPERTY (ArrangerObjectMuteFunctionality * mute READ mute CONSTANT)
  Q_PROPERTY (int pitch READ pitch WRITE setPitch NOTIFY pitchChanged)
  Q_PROPERTY (
    int velocity READ velocity WRITE setVelocity NOTIFY velocityChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  MidiNote (const dsp::TempoMap &tempo_map, QObject * parent = nullptr);
  Z_DISABLE_COPY_MOVE (MidiNote)
  ~MidiNote () override = default;

  static constexpr midi_byte_t DEFAULT_VELOCITY = 90;

  enum class Notation : std::uint_fast8_t
  {
    Musical,
    Pitch,
  };

  // ========================================================================
  // QML Interface
  // ========================================================================

  ArrangerObjectBounds *            bounds () const { return bounds_.get (); }
  ArrangerObjectMuteFunctionality * mute () const { return mute_.get (); }

  int  pitch () const { return static_cast<int> (pitch_); }
  void setPitch (int ipitch)
  {
    ipitch = std::clamp (ipitch, 0, 127);
    const auto pitch = static_cast<midi_byte_t> (ipitch);
    if (pitch_ != pitch)
      {
        pitch_ = pitch;
        Q_EMIT pitchChanged (ipitch);
      }
  }
  /**
   * Shifts the pitch by @p delta amount.
   */
  Q_INVOKABLE void shift_pitch (int delta)
  {
    setPitch (static_cast<int> (pitch_) + delta);
  }
  Q_SIGNAL void       pitchChanged (int ipitch);
  Q_INVOKABLE QString pitchAsRichText () const
  {
    return pitch_to_string (pitch_, Notation::Musical, true).to_qstring ();
  }

  int  velocity () const { return static_cast<int> (velocity_); }
  void setVelocity (int ivelocity)
  {
    ivelocity = std::clamp (ivelocity, 0, 127);
    const auto velocity = static_cast<midi_byte_t> (ivelocity);
    if (velocity_ != velocity)
      {
        velocity_ = velocity;
        Q_EMIT velocityChanged (ivelocity);
      }
  }
  Q_SIGNAL void velocityChanged (int ivelocity);

  // ========================================================================

  /**
   * Gets the MIDI note's value as a string (eg "C#4").
   *
   * @param rich_text Use QML rich text to show the octave as a superscript.
   */
  static utils::Utf8String
  pitch_to_string (uint8_t pitch, Notation notation, bool rich_text);

private:
  /**
   * Returns the note starting first, or nullptr.
   */
  template <RangeOfMidiNotePointers Range>
  friend MidiNote * get_first_midi_note (const Range &range)
  {
    auto it = std::ranges::min_element (range, [] (const auto &a, const auto &b) {
      return a->position ()->ticks () < b->position ()->ticks ();
    });
    return (it != range.end ()) ? *it : nullptr;
  }

  /**
   * Gets last midi note
   */
  template <RangeOfMidiNotePointers Range>
  friend MidiNote * get_last_midi_note (const Range &range)
  {
    // take the reverse because max_element returns the first match, and we want
    // to return the last match in the container in this case
    const auto range_to_test = range | std::views::reverse;
    auto       it = std::ranges::max_element (
      range_to_test, [] (const auto &a, const auto &b) {
        return a->bounds ()->get_end_position_samples (false)
               < b->bounds ()->get_end_position_samples (false);
      });
    return (it != range_to_test.end ()) ? *it : nullptr;
  }

  /**
   * Returns the minimum and maximum pitches in the given range.
   */
  template <RangeOfMidiNotePointers Range>
  friend auto get_pitch_range (const Range &range)
    -> std::optional<std::pair<midi_byte_t, midi_byte_t>>
  {
    auto [min_it, max_it] =
      std::ranges::minmax_element (range, [] (const auto &a, const auto &b) {
        return a->pitch () < b->pitch ();
      });
    if (min_it == range.end () || max_it == range.end ())
      return std::nullopt;
    return std::make_pair ((*min_it)->pitch (), (*max_it)->pitch ());
  }

  friend void init_from (
    MidiNote              &obj,
    const MidiNote        &other,
    utils::ObjectCloneType clone_type);

  static constexpr auto kBoundsKey = "bounds"sv;
  static constexpr auto kMuteKey = "mute"sv;
  static constexpr auto kVelocityKey = "velocity"sv;
  static constexpr auto kPitchKey = "pitch"sv;
  friend void           to_json (nlohmann::json &j, const MidiNote &note)
  {
    to_json (j, static_cast<const ArrangerObject &> (note));
    j[kBoundsKey] = note.bounds_;
    j[kMuteKey] = note.mute_;
    j[kVelocityKey] = note.velocity_;
    j[kPitchKey] = note.pitch_;
  }
  friend void from_json (const nlohmann::json &j, MidiNote &note)
  {
    from_json (j, static_cast<ArrangerObject &> (note));
    j.at (kBoundsKey).get_to (*note.bounds_);
    j.at (kMuteKey).get_to (*note.mute_);
    j.at (kVelocityKey).get_to (note.velocity_);
    j.at (kPitchKey).get_to (note.pitch_);
  }

private:
  utils::QObjectUniquePtr<ArrangerObjectBounds>            bounds_;
  utils::QObjectUniquePtr<ArrangerObjectMuteFunctionality> mute_;

  /** Velocity. */
  std::uint8_t velocity_{ DEFAULT_VELOCITY };

  /** The note/pitch, (0-127). */
  std::uint8_t pitch_{ 60 };

  BOOST_DESCRIBE_CLASS (MidiNote, (), (), (), (bounds_, mute_, velocity_, pitch_))
};

}
