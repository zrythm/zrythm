// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_REGION_OWNED_OBJECT_H__
#define __DSP_REGION_OWNED_OBJECT_H__

#include "gui/dsp/arranger_object.h"

class Region;

class RegionOwnedObject
    : virtual public ArrangerObject,
      public zrythm::utils::serialization::ISerializable<RegionOwnedObject>
{
public:
  RegionOwnedObject (ArrangerObjectRegistry &obj_registry)
      : object_registry_ (obj_registry)
  {
  }
  ~RegionOwnedObject () noexcept override = default;
  Q_DISABLE_COPY_MOVE (RegionOwnedObject)

  /**
   * Sets the region the object belongs to and the related index inside it.
   */
  void set_region_and_index (const Region &region);

  /**
   * Gets the global (timeline-based) start Position of the object.
   *
   * @param[out] pos Position to fill in.
   */
  template <typename SelfT>
  void get_global_start_pos (
    this const SelfT &self,
    dsp::Position    &pos,
    double            frames_per_tick)
  {
    auto r = self.get_region ();
    pos = *static_cast<Position *> (self.pos_);
    pos.add_ticks (r->pos_->ticks_, frames_per_tick);
  }

  /**
   * If the object is part of a Region, returns it, otherwise returns NULL.
   */
  template <typename SelfT>
  [[gnu::hot]] auto get_region (this const SelfT &self)
  {
    assert (self.region_id_.has_value ());
    return std::get<typename SelfT::RegionT *> (
      self.object_registry_.find_by_id_or_throw (*self.region_id_));
  }

  friend bool
  operator== (const RegionOwnedObject &lhs, const RegionOwnedObject &rhs);

protected:
  void
  copy_members_from (const RegionOwnedObject &other, ObjectCloneType clone_type);

  void init_loaded_base ();

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  ArrangerObjectRegistry &object_registry_;

  /** Parent region identifier for objects that are part of a region. */
  std::optional<ArrangerObject::Uuid> region_id_;

  /** Index in the owner region. */
  // int index_ = -1;
};

inline bool
operator== (const RegionOwnedObject &lhs, const RegionOwnedObject &rhs)
{
  return lhs.region_id_ == rhs.region_id_
         && static_cast<const ArrangerObject &> (lhs)
              == static_cast<const ArrangerObject &> (rhs);
}

template <typename T>
concept RegionOwnedObjectSubclass = std::derived_from<T, RegionOwnedObject>;

using RegionOwnedObjectVariant =
  std::variant<Velocity, MidiNote, ChordObject, AutomationPoint>;
using RegionOwnedObjectPtrVariant = to_pointer_variant<RegionOwnedObjectVariant>;

#endif // __DSP_REGION_OWNED_OBJECT_H__
