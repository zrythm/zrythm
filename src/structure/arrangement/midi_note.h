// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include <cstdint>
#include <memory>

#include "dsp/position.h"
#include "structure/arrangement/bounded_object.h"
#include "structure/arrangement/muteable_object.h"
#include "structure/arrangement/velocity.h"

namespace zrythm::structure::arrangement
{
/**
 * A MIDI note inside a Region shown in the piano roll.
 */
class MidiNote final
    : public QObject,
      public MuteableObject,
      public RegionOwnedObject,
      public BoundedObject,
      public ICloneable<MidiNote>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (MidiNote)
  DEFINE_BOUNDED_OBJECT_QML_PROPERTIES (MidiNote)
  Q_PROPERTY (int pitch READ getPitch WRITE setPitch NOTIFY pitchChanged)

public:
  using RegionT = MidiRegion;

  DECLARE_FINAL_ARRANGER_OBJECT_CONSTRUCTORS (MidiNote)

  Q_DISABLE_COPY_MOVE (MidiNote)
  ~MidiNote () override = default;

  enum class Notation
  {
    Musical,
    Pitch,
  };

  // ========================================================================
  // QML Interface
  // ========================================================================

  int getPitch () const { return static_cast<int> (pitch_); }

  void setPitch (int ipitch)
  {
    const auto pitch = static_cast<midi_byte_t> (ipitch);
    if (pitch_ != pitch)
      {
        pitch_ = pitch;
        Q_EMIT pitchChanged ();
      }
  }
  Q_SIGNAL void pitchChanged ();

  // ========================================================================

  void set_cache_val (const uint8_t val) { cache_pitch_ = val; }

  /**
   * Gets the MIDI note's value as a string (eg "C#4").
   *
   * @param use_markup Use markup to show the octave as a superscript.
   */
  utils::Utf8String
  get_val_as_string (Notation notation, bool use_markup) const;

  /**
   * Listen to the given MidiNote.
   *
   * @param listen Turn note on if 1, or turn it
   *   off if 0.
   */
  void listen (bool listen);

  /**
   * Shifts MidiNote's position and/or value.
   *
   * @param delta Y (0-127)
   */
  void shift_pitch (int delta);

  void set_velocity (int vel) { vel_->setValue (vel); }

  /**
   * Sends a note off if currently playing and sets
   * the pitch of the MidiNote.
   * FIXME: this should not be the responsibility of the MidiNote.
   */
  void set_pitch (uint8_t val);

  ArrangerObjectPtrVariant
  add_clone_to_project (bool fire_events) const override;

  ArrangerObjectPtrVariant insert_clone_to_project () const override;

  // friend bool operator== (const MidiNote &lhs, const MidiNote &rhs);

  utils::Utf8String gen_human_friendly_name () const override;

  bool
  validate (bool is_project, dsp::FramesPerTick frames_per_tick) const override;

  void init_after_cloning (const MidiNote &other, ObjectCloneType clone_type)
    override;

private:
  static constexpr std::string_view kVelocityKey = "velocity";
  static constexpr std::string_view kPitchKey = "pitch";
  friend void to_json (nlohmann::json &j, const MidiNote &note)
  {
    to_json (j, static_cast<const ArrangerObject &> (note));
    to_json (j, static_cast<const BoundedObject &> (note));
    to_json (j, static_cast<const MuteableObject &> (note));
    to_json (j, static_cast<const RegionOwnedObject &> (note));
    j[kVelocityKey] = note.vel_;
    j[kPitchKey] = note.pitch_;
  }
  friend void from_json (const nlohmann::json &j, MidiNote &note)
  {
    from_json (j, static_cast<ArrangerObject &> (note));
    from_json (j, static_cast<BoundedObject &> (note));
    from_json (j, static_cast<MuteableObject &> (note));
    from_json (j, static_cast<RegionOwnedObject &> (note));
    j.at (kVelocityKey).get_to (*note.vel_);
    j.at (kPitchKey).get_to (note.pitch_);
  }

  friend bool operator== (const MidiNote &lhs, const MidiNote &rhs)
  {
    return lhs.pitch_ == rhs.pitch_ && *lhs.vel_ == *rhs.vel_
           && static_cast<const MuteableObject &> (lhs)
                == static_cast<const MuteableObject &> (rhs)
           && static_cast<const RegionOwnedObject &> (lhs)
                == static_cast<const RegionOwnedObject &> (rhs)
           && static_cast<const BoundedObject &> (lhs)
                == static_cast<const BoundedObject &> (rhs)
           && static_cast<const ArrangerObject &> (lhs)
                == static_cast<const ArrangerObject &> (rhs);
  }

public:
  /** Velocity. */
  QScopedPointer<Velocity> vel_;

  /** The note/pitch, (0-127). */
  uint8_t pitch_{};

  /** Cached note, for live operations. */
  uint8_t cache_pitch_{};

  /** Whether or not this note is currently listened to */
  bool currently_listened_{};

  /** The note/pitch that is currently playing, if @ref
   * currently_listened_ is true. */
  uint8_t last_listened_pitch_{};

  BOOST_DESCRIBE_CLASS (
    MidiNote,
    (MuteableObject, RegionOwnedObject, BoundedObject),
    (vel_, pitch_),
    (),
    ())
};

}
