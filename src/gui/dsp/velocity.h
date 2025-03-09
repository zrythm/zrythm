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
      public RegionOwnedObjectImpl<MidiRegion>,
      public ICloneable<Velocity>,
      public zrythm::utils::serialization::ISerializable<Velocity>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (Velocity)
public:
  Velocity (QObject * parent = nullptr);

  /**
   * Creates a new Velocity with the given value.
   */
  Velocity (MidiNote * midi_note, uint8_t vel);

  /**
   * Sets the velocity to the given value.
   *
   * The given value may exceed the bounds 0-127, and will be clamped.
   */
  void set_val (int val);

  /**
   * Returns the owner MidiNote.
   */
  MidiNote * get_midi_note () const;

  static const char * setting_enum_to_str (size_t index);

  static size_t setting_str_to_enum (const char * str);

  std::optional<ArrangerObjectPtrVariant> find_in_project () const override;

  /* these do not apply to velocities */
  ArrangerObjectPtrVariant
  add_clone_to_project (bool fire_events) const override
  {
    throw ZrythmException ("Cannot add a velocity clone to a project");
  }

  ArrangerObjectPtrVariant insert_clone_to_project () const override
  {
    throw ZrythmException ("Cannot add a velocity clone to a project");
  }

  std::string print_to_str () const override;

  bool validate (bool is_project, double frames_per_tick) const override;

  void init_after_cloning (const Velocity &other, ObjectCloneType clone_type)
    override;

  void init_loaded () override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Pointer back to the MIDI note (this is also the QObject parent). */
  MidiNote * midi_note_ = nullptr;

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
