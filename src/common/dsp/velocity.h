// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_VELOCITY_H__
#define __AUDIO_VELOCITY_H__

#include "common/dsp/arranger_object.h"
#include "common/dsp/region_owned_object.h"
#include "common/utils/icloneable.h"

class MidiNote;
class MidiRegion;
TYPEDEF_STRUCT_UNDERSCORED (VelocityWidget);

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
    : public RegionOwnedObjectImpl<MidiRegion>,
      public ICloneable<Velocity>,
      public ISerializable<Velocity>
{
public:
  Velocity ();

  /**
   * Creates a new Velocity with the given value.
   */
  Velocity (MidiNote * midi_note, const uint8_t vel);

  /**
   * Sets the velocity to the given value.
   *
   * The given value may exceed the bounds 0-127, and will be clamped.
   */
  void set_val (const int val);

  /**
   * Returns the owner MidiNote.
   */
  MidiNote * get_midi_note () const;

  static const char * setting_enum_to_str (guint index);

  static guint setting_str_to_enum (const char * str);

  ArrangerWidget * get_arranger () const override;

  ArrangerObjectPtr find_in_project () const override;

  std::string print_to_str () const override;

  /* these do not apply to velocities */
  ArrangerObjectPtr add_clone_to_project (bool fire_events) const override
  {
    throw ZrythmException ("Cannot add a velocity clone to a project");
  }

  ArrangerObjectPtr insert_clone_to_project () const override
  {
    throw ZrythmException ("Cannot add a velocity clone to a project");
  }

  bool validate (bool is_project, double frames_per_tick) const override;

  void init_after_cloning (const Velocity &other) override;

  void init_loaded () override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Pointer back to the MIDI note. */
  MidiNote * midi_note_;

  /** Velocity value (0-127). */
  uint8_t vel_ = 0;

  /** Velocity at drag begin - used for ramp actions only. */
  uint8_t vel_at_start_ = 0;
};

inline bool
operator== (const Velocity &lhs, const Velocity &rhs)
{
  return lhs.vel_ == rhs.vel_
         && static_cast<const RegionOwnedObject &> (lhs)
              == static_cast<const RegionOwnedObject &> (rhs)
         && static_cast<const ArrangerObject &> (lhs)
              == static_cast<const ArrangerObject &> (rhs);
}

/**
 * @}
 */

#endif
