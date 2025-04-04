// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_REGION_OWNER_H__
#define __DSP_REGION_OWNER_H__

#include "gui/dsp/audio_region.h"
#include "gui/dsp/automation_region.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/iproject_owned_object.h"
#include "gui/dsp/midi_region.h"
#include "gui/dsp/region_list.h"

#include "dsp/position.h"
#include "utils/types.h"

#define DEFINE_REGION_OWNER_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* regions */ \
  /* ================================================================ */ \
  Q_PROPERTY (RegionList * regions READ getRegions CONSTANT) \
  RegionList * getRegions () const \
  { \
    return region_list_; \
  }

/**
 * Base class for an object that can own regions.
 */
template <typename RegionT>
class RegionOwner
    : virtual public IProjectOwnedObject,
      public zrythm::utils::serialization::ISerializable<RegionOwner<RegionT>>
{
public:
  using RegionTPtr = RegionT *;

  ~RegionOwner () override = default;
  Q_DISABLE_COPY_MOVE (RegionOwner)

  void parent_base_qproperties (QObject &derived);

  /**
   * @brief Removes the given region if found.
   *
   * @param region
   * @param fire_events
   * @return Whether removed.
   */
  bool remove_region (RegionT &region, bool free_region, bool fire_events);

  /**
   * @see @ref insert_region().
   */
  void add_region (RegionTPtr region)
  {
    return insert_region (region, region_list_->rowCount ());
  }

  /**
   * Inserts an automation Region to the AutomationTrack at the given index.
   *
   * @warning This must not be used directly. Use Track.insert_region() instead.
   */
  void insert_region (RegionTPtr region, int idx);

  /**
   * Returns the region at the given position, or NULL.
   *
   * @param include_region_end Whether to include the region's end in the
   * calculation.
   */
  RegionT *
  get_region_at_pos (dsp::Position pos, bool include_region_end = false) const
  {
    auto it =
      std::ranges::find_if (region_list_->regions_, [&] (const auto &r_var) {
        return *std::get<RegionT *> (r_var)->pos_ <= pos
               && std::get<RegionT *> (r_var)->end_pos_->frames_
                      + (include_region_end ? 1 : 0)
                    > pos.frames_;
      });
    return it != region_list_->regions_.end () ? std::get<RegionT *> (*it) : nullptr;
  }

  void clear_regions ();

  void unselect_all ();

  void foreach_region (std::function<void (RegionT &)> func);

  void foreach_region (std::function<void (RegionT &)> func) const;

protected:
  RegionOwner ();

  void copy_members_from (const RegionOwner &other, ObjectCloneType clone_type);

  /**
   * @brief Optional callback after removing a region.
   *
   * @param region
   */
  virtual void after_remove_region () { }

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  RegionList * region_list_ = nullptr;

  /** Snapshots used during playback, if applicable. */
  std::vector<std::unique_ptr<RegionT>> region_snapshots_;

protected:
  /**
   * @brief Whether currently in the middle of @ref clear_regions().
   */
  bool clearing_ = false;
};

class MidiRegion;
class ChordRegion;
class AutomationRegion;
class AudioRegion;
using RegionOwnerVariant = std::variant<
  RegionOwner<MidiRegion>,
  RegionOwner<ChordRegion>,
  RegionOwner<AutomationRegion>,
  RegionOwner<AudioRegion>>;
using RegionOwnerPtrVariant = to_pointer_variant<RegionOwnerVariant>;

extern template class RegionOwner<AudioRegion>;
extern template class RegionOwner<AutomationRegion>;
extern template class RegionOwner<ChordRegion>;
extern template class RegionOwner<MidiRegion>;

#endif // __DSP_REGION_OWNER_H__
