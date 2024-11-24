// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_LANE_OWNED_OBJECT_H__
#define __DSP_LANE_OWNED_OBJECT_H__

#include "gui/dsp/timeline_object.h"

class Region;
template <typename RegionT> class TrackLaneImpl;
class MidiLane;
class AudioLane;
template <typename TrackLaneT> class LanedTrackImpl;

class LaneOwnedObject : virtual public TimelineObject
{
public:
  ~LaneOwnedObject () override = default;

protected:
  void copy_members_from (const LaneOwnedObject &other)
  {
    index_in_prev_lane_ = other.index_in_prev_lane_;
  }

public:
  /**
   * Object's index in the previous lane (before being moved to a new
   * lane/track).
   *
   * Used at runtime when duplicating objects in new lanes/tracks so we can
   * put the object back to its place before creating new copies.
   *
   * @see arranger_selections_action_do().
   */
  int index_in_prev_lane_ = 0;
};

template <typename RegionT>
class LaneOwnedObjectImpl : virtual public LaneOwnedObject
{
public:
  ~LaneOwnedObjectImpl () override = default;

  using TrackLaneT =
    std::conditional_t<std::is_same_v<RegionT, MidiRegion>, MidiLane, AudioLane>;
  using LanedTrackT = LanedTrackImpl<TrackLaneT>;

  /**
   * Get lane.
   */
  TrackLaneT * get_lane () const;

  /**
   * Sets the track lane.
   */
  void set_lane (TrackLaneT &lane);

public:
  /** Pointer to the track lane that owns this object. */
  TrackLaneT * owner_lane_ = nullptr;
};

template <typename RegionT>
concept LaneOwnedRegionSubclass =
  DerivedButNotBase<RegionT, Region>
  && DerivedButNotBase<RegionT, LaneOwnedObjectImpl<RegionT>>;

class MidiRegion;
class AudioRegion;
using LaneOwnedObjectVariant = std::variant<MidiRegion, AudioRegion>;

extern template class LaneOwnedObjectImpl<MidiRegion>;
extern template class LaneOwnedObjectImpl<AudioRegion>;

#endif // __DSP_LANE_OWNED_OBJECT_H__
