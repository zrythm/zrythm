// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_LANE_OWNED_OBJECT_H__
#define __DSP_LANE_OWNED_OBJECT_H__

#include "gui/dsp/timeline_object.h"
#include "gui/dsp/track_lane_fwd.h"

class MidiLane;
class AudioLane;
class TrackLane;

class LaneOwnedObject : virtual public TimelineObject
{
public:
  // using TrackLaneUuid = utils::UuidIdentifiableObject<TrackLane>::Uuid;

  LaneOwnedObject () = default;
  ~LaneOwnedObject () override = default;
  Z_DISABLE_COPY_MOVE (LaneOwnedObject);

  bool is_inserted_in_lane () const { return owner_lane_.has_value (); }

  template <typename SelfT>
  MidiLane &get_lane (this const SelfT &self)
    requires std::is_same_v<SelfT, MidiRegion>
  {
    return *std::get<MidiLane *> (*self.owner_lane_);
  }

  template <typename SelfT>
  AudioLane &get_lane (this const SelfT &self)
    requires std::is_same_v<SelfT, AudioRegion>
  {
    return *std::get<AudioLane *> (*self.owner_lane_);
  }

  template <typename SelfT> auto get_lane_index (this const SelfT &self)
  {
    return self.get_lane ().get_index_in_track ();
  }

  /**
   * Sets the track lane.
   */
  void set_lane (TrackLanePtrVariant lane) { owner_lane_ = lane; }

  void
  copy_members_from (const LaneOwnedObject &other, ObjectCloneType clone_type)
  {
    index_in_prev_lane_ = other.index_in_prev_lane_;
  }

private:
  /** Track lane that owns this object. */
  std::optional<TrackLanePtrVariant> owner_lane_;

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
  std::optional<int> index_in_prev_lane_;
};

class MidiRegion;
class AudioRegion;
using LaneOwnedObjectVariant = std::variant<MidiRegion, AudioRegion>;
using LaneOwnedObjectPtrVariant = to_pointer_variant<LaneOwnedObjectVariant>;

#endif // __DSP_LANE_OWNED_OBJECT_H__
