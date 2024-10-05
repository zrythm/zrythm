// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_REGION_OWNED_OBJECT_H__
#define __DSP_REGION_OWNED_OBJECT_H__

#include "dsp/arranger_object.h"
#include "dsp/region_identifier.h"

class RegionOwnedObject
    : virtual public ArrangerObject,
      public ISerializable<RegionOwnedObject>
{
public:
  virtual ~RegionOwnedObject () = default;

  /**
   * Sets the region the object belongs to and the related index inside it.
   */
  void set_region_and_index (const Region &region, int index);

  friend bool
  operator== (const RegionOwnedObject &lhs, const RegionOwnedObject &rhs);

protected:
  void copy_members_from (const RegionOwnedObject &other);

  void init_loaded_base ();

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  /** Parent region identifier for objects that are part of a region. */
  RegionIdentifier region_id_;

  /** Index in the owner region. */
  int index_ = -1;
};

/**
 * @brief CRTP implementation of RegionOwnedObject.
 *
 * @tparam RegionT Region class.
 */
template <typename RegionT>
class RegionOwnedObjectImpl : virtual public RegionOwnedObject
{
public:
  RegionOwnedObjectImpl () = default;
  RegionOwnedObjectImpl (const RegionIdentifier &region_id, int index = -1)
  {
    region_id_ = region_id;
    index_ = index;
  }

  virtual ~RegionOwnedObjectImpl () = default;

  /**
   * Gets the global (timeline-based) start Position of the object.
   *
   * @param[out] pos Position to fill in.
   */
  void get_global_start_pos (Position &pos) const;

  /**
   * If the object is part of a Region, returns it, otherwise returns NULL.
   */
  ATTR_HOT RegionT * get_region () const;
};

inline bool
operator== (const RegionOwnedObject &lhs, const RegionOwnedObject &rhs)
{
  return lhs.region_id_ == rhs.region_id_ && lhs.index_ == rhs.index_
         && static_cast<const ArrangerObject &> (lhs)
              == static_cast<const ArrangerObject &> (rhs);
}

template <typename T>
concept RegionOwnedObjectSubclass = std::derived_from<T, RegionOwnedObject>;

using RegionOwnedObjectVariant =
  std::variant<Velocity, MidiNote, ChordObject, AutomationPoint>;
using RegionOwnedObjectPtrVariant = to_pointer_variant<RegionOwnedObjectVariant>;

extern template class RegionOwnedObjectImpl<MidiRegion>;
extern template class RegionOwnedObjectImpl<AutomationRegion>;
extern template class RegionOwnedObjectImpl<ChordRegion>;

#endif // __DSP_REGION_OWNED_OBJECT_H__