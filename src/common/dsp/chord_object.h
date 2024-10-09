// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CHORD_OBJECT_H__
#define __AUDIO_CHORD_OBJECT_H__

#include "common/dsp/arranger_object.h"
#include "common/dsp/chord_descriptor.h"
#include "common/dsp/muteable_object.h"
#include "common/dsp/region_identifier.h"
#include "common/dsp/region_owned_object.h"
#include "common/utils/icloneable.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

constexpr int CHORD_OBJECT_MAGIC = 4181694;
#define IS_CHORD_OBJECT(x) (((ChordObject *) x)->magic == CHORD_OBJECT_MAGIC)
#define IS_CHORD_OBJECT_AND_NONNULL(x) (x && IS_CHORD_OBJECT (x))

#define CHORD_OBJECT_WIDGET_TRIANGLE_W 10

#define chord_object_is_selected(r) \
  arranger_object_is_selected ((ArrangerObject *) r)

class ChordDescriptor;
class ChordRegion;

/**
 * The ChordObject class represents a chord inside a ChordRegion. It inherits
 * from MuteableObject and RegionOwnedObject.
 *
 * The class provides methods to set the region and index of the chord, get the
 * associated ChordDescriptor, and find the ChordObject corresponding to a given
 * position.
 *
 * The chord_index_ member variable stores the index of the chord in the chord
 * pad (0 being the topmost chord). The magic_ member variable is used to
 * identify valid ChordObject instances. The layout member variable is a cache
 * for the Pango layout used to draw the chord name.
 */
class ChordObject final
    : public MuteableObject,
      public RegionOwnedObjectImpl<ChordRegion>,
      public ICloneable<ChordObject>,
      public ISerializable<ChordObject>
{
public:
  // Rule of 0
  ChordObject () = default;

  ChordObject (const RegionIdentifier &region_id, int chord_index, int index)
      : ArrangerObject (Type::ChordObject),
        RegionOwnedObjectImpl (region_id, index), chord_index_ (chord_index)
  {
  }

  using RegionOwnedObjectT = RegionOwnedObjectImpl<ChordRegion>;

  /**
   * Returns the ChordDescriptor associated with this ChordObject.
   */
  ChordDescriptor * get_chord_descriptor () const;

  ArrangerObjectPtr find_in_project () const override;

  ArrangerObjectPtr add_clone_to_project (bool fire_events) const override;

  ArrangerObjectPtr insert_clone_to_project () const override;

  std::string print_to_str () const override;

  std::string gen_human_friendly_name () const override;

  friend bool operator== (const ChordObject &lhs, const ChordObject &rhs);

  bool validate (bool is_project, double frames_per_tick) const override;

  void init_after_cloning (const ChordObject &other) override;

  void init_loaded () override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** The index of the chord it belongs to (0 topmost). */
  int chord_index_ = 0;

  int magic_ = CHORD_OBJECT_MAGIC;
};

inline bool
operator== (const ChordObject &lhs, const ChordObject &rhs)
{
  return static_cast<const ArrangerObject &> (lhs)
           == static_cast<const ArrangerObject &> (rhs)
         && lhs.chord_index_ == rhs.chord_index_
         && static_cast<const ChordObject::RegionOwnedObjectT &> (lhs)
              == static_cast<const ChordObject::RegionOwnedObjectT &> (rhs)
         && static_cast<const MuteableObject &> (lhs)
              == static_cast<const MuteableObject &> (rhs);
}

DEFINE_OBJECT_FORMATTER (ChordObject, [] (const ChordObject &co) {
  return fmt::format (
    "ChordObject [{}]: chord index {}", co.pos_, co.chord_index_);
});

/**
 * @}
 */

#endif
