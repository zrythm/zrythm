// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_VELOCITY_H__
#define __AUDIO_VELOCITY_H__

#include "gui/dsp/arranger_object.h"
#include "gui/dsp/region_owned_object.h"

#include "utils/icloneable.h"

class MidiNote;
class MidiRegion;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Default velocity.
 */
constexpr uint8_t VELOCITY_DEFAULT = 90;

/**
 * The MidiNote velocity.
 */
class Velocity final
    : public QObject,
      public ICloneable<Velocity>,
      public zrythm::utils::serialization::ISerializable<Velocity>
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (int value READ getValue WRITE setValue NOTIFY valueChanged)
public:
  using VelocityValueT = midi_byte_t;
  Velocity (QObject * parent = nullptr);

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

  static const char * setting_enum_to_str (size_t index);

  static size_t setting_str_to_enum (const char * str);

  void init_after_cloning (const Velocity &other, ObjectCloneType clone_type)
    override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Pointer back to the MIDI note (this is also the QObject parent). */
  // MidiNote * midi_note_ = nullptr;

  /** Velocity value (0-127). */
  VelocityValueT vel_ = 0;

  /** Velocity at drag begin - used for ramp actions only. */
  VelocityValueT vel_at_start_ = 0;
};

inline bool
operator== (const Velocity &lhs, const Velocity &rhs)
{
  return lhs.vel_ == rhs.vel_;
}

/**
 * @}
 */

#endif
