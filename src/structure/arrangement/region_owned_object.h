// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"

namespace zrythm::structure::arrangement
{

class Region;

class RegionOwnedObject : virtual public ArrangerObject
{
public:
  RegionOwnedObject (ArrangerObjectRegistry &obj_registry) noexcept
      : object_registry_ (obj_registry)
  {
  }
  ~RegionOwnedObject () override = default;
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
    this const SelfT  &self,
    dsp::Position     &pos,
    dsp::FramesPerTick frames_per_tick)
  {
    auto r = self.get_region ();
    pos = self.get_position ();
    pos.add_ticks (r->get_position ().ticks_, frames_per_tick);
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

private:
  static constexpr std::string_view kRegionIdKey = "regionId";
  friend void to_json (nlohmann::json &j, const RegionOwnedObject &object)
  {
    j[kRegionIdKey] = object.region_id_;
  }
  friend void from_json (const nlohmann::json &j, RegionOwnedObject &object)
  {
    j.at (kRegionIdKey).get_to (object.region_id_);
  }

  friend bool
  operator== (const RegionOwnedObject &lhs, const RegionOwnedObject &rhs)
  {
    return lhs.region_id_ == rhs.region_id_
           && static_cast<const ArrangerObject &> (lhs)
                == static_cast<const ArrangerObject &> (rhs);
  }

public:
  ArrangerObjectRegistry &object_registry_;

  /** Parent region identifier for objects that are part of a region. */
  std::optional<ArrangerObject::Uuid> region_id_;

  BOOST_DESCRIBE_CLASS (RegionOwnedObject, (ArrangerObject), (region_id_), (), ())
};

template <typename T>
concept RegionOwnedObjectSubclass = std::derived_from<T, RegionOwnedObject>;

} // namespace zrythm::structure::arrangement
