// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/region_owned_object.h"
#include "utils/icloneable.h"

namespace zrythm::structure::arrangement
{
class MidiNote;
class MidiRegion;

/**
 * The MidiNote velocity.
 */
class Velocity final : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (int value READ getValue WRITE setValue NOTIFY valueChanged)
public:
  using VelocityValueT = midi_byte_t;
  Velocity (QObject * parent = nullptr);

  /**
   * Default value.
   */
  static constexpr VelocityValueT DEFAULT_VALUE = 90;

  // ========================================================================
  // QML Interface
  // ========================================================================

  int getValue () const { return static_cast<int> (vel_); }

  void setValue (int ival)
  {
    const auto vel = static_cast<VelocityValueT> (ival);
    if (vel_ != vel)
      {
        vel_ = vel;
        Q_EMIT valueChanged ();
      }
  }
  Q_SIGNAL void valueChanged ();

  // ========================================================================

  /**
   * Sets the velocity to the given value.
   *
   * The given value may exceed the bounds 0-127, and will be clamped.
   */
  void set_val (int val);

  /**
   * Returns the owner MidiNote.
   */
  // MidiNote * get_midi_note () const;

#if 0
  static const char * setting_enum_to_str (size_t index);

  static size_t setting_str_to_enum (const char * str);
#endif

  friend void init_from (
    Velocity              &obj,
    const Velocity        &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr std::string_view kVelocityKey = "value";
  friend void to_json (nlohmann::json &j, const Velocity &velocity)
  {
    j[kVelocityKey] = velocity.vel_;
  }
  friend void from_json (const nlohmann::json &j, Velocity &velocity)
  {
    j.at (kVelocityKey).get_to (velocity.vel_);
  }

  friend bool operator== (const Velocity &lhs, const Velocity &rhs)
  {
    return lhs.vel_ == rhs.vel_;
  }

public:
  /** Pointer back to the MIDI note (this is also the QObject parent). */
  // MidiNote * midi_note_ = nullptr;

  /** Velocity value (0-127). */
  VelocityValueT vel_ = 0;

  /** Velocity at drag begin - used for ramp actions only. */
  VelocityValueT vel_at_start_ = 0;

  BOOST_DESCRIBE_CLASS (Velocity, (), (vel_), (), ())
};

}
