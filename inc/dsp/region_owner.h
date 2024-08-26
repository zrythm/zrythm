// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_REGION_OWNER_H__
#define __DSP_REGION_OWNER_H__

#include <memory>
#include <ranges>
#include <vector>

#include "dsp/audio_region.h"
#include "dsp/automation_region.h"
#include "dsp/chord_region.h"
#include "dsp/iproject_owned_object.h"
#include "dsp/midi_region.h"
#include "dsp/position.h"
#include "utils/types.h"

/**
 * Interface for an object that can own regions.
 */
class RegionOwner : virtual public IProjectOwnedObject
{
public:
  virtual ~RegionOwner () = default;

  /**
   * @brief Get all the regions
   *
   * @note Not realtime-safe.
   */
  // virtual std::vector<RegionVariant *> get_regions () = 0;
};

/**
 * Interface for an object that can own regions.
 */
template <typename RegionT>
class RegionOwnerImpl
    : virtual public RegionOwner,
      public ISerializable<RegionOwnerImpl<RegionT>>
{
public:
  using SharedRegionPtr = std::shared_ptr<RegionT>;

  virtual ~RegionOwnerImpl () = default;

  /**
   * @brief Removes the given region if found.
   *
   * @param region
   * @param fire_events
   * @return Whether removed.
   */
  bool remove_region (RegionT &region, bool fire_events);

  /**
   * @see @ref insert_region().
   */
  SharedRegionPtr add_region (SharedRegionPtr region)
  {
    return insert_region (region, regions_.size ());
  }

  /**
   * Inserts an automation Region to the AutomationTrack at the given index.
   *
   * @warning This must not be used directly. Use Track.insert_region() instead.
   */
  SharedRegionPtr insert_region (SharedRegionPtr region, int idx);

  /**
   * Returns the region at the given position, or NULL.
   *
   * @param include_region_end Whether to include the region's end in the
   * calculation.
   */
  RegionT *
  get_region_at_pos (Position pos, bool include_region_end = false) const
  {
    auto it = std::ranges::find_if (regions_, [&] (const auto &r) {
      return r->pos_ <= pos
             && r->end_pos_.frames_ + (include_region_end ? 1 : 0) > pos.frames_;
    });
    return it != regions_.end () ? it->get () : nullptr;
  }

  void clear_regions ();

  void unselect_all ()
  {
    for (auto &region : regions_)
      {
        region->select (false, false, false);
      }
  }

protected:
  void copy_members_from (const RegionOwnerImpl &other)
  {
    clone_ptr_vector (regions_, other.regions_);
  }

  /**
   * @brief Optional callback before removing a region.
   *
   * @param region
   */
  virtual void before_remove_region (RegionT &region) { }

  /**
   * @brief Optional callback after removing a region.
   *
   * @param region
   */
  virtual void after_remove_region () { }

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  /**
   * @brief Regions in this region owner.
   *
   * @note must always be sorted by position.
   */
  std::vector<SharedRegionPtr> regions_;

  /** Snapshots used during playback, if applicable. */
  std::vector<std::unique_ptr<RegionT>> region_snapshots_;
};

class MidiRegion;
class ChordRegion;
class AutomationRegion;
class AudioRegion;
using RegionOwnerVariant = std::variant<
  RegionOwnerImpl<MidiRegion>,
  RegionOwnerImpl<ChordRegion>,
  RegionOwnerImpl<AutomationRegion>,
  RegionOwnerImpl<AudioRegion>>;
using RegionOwnerPtrVariant = to_pointer_variant<RegionOwnerVariant>;

extern template class RegionOwnerImpl<AudioRegion>;
extern template class RegionOwnerImpl<AutomationRegion>;
extern template class RegionOwnerImpl<ChordRegion>;
extern template class RegionOwnerImpl<MidiRegion>;

#endif // __DSP_REGION_OWNER_H__