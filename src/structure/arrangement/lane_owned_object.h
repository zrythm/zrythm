// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/timeline_object.h"
#include "structure/tracks/track_lane_fwd.h"

namespace zrythm::structure::tracks
{
class MidiLane;
class AudioLane;
class TrackLane;
}

namespace zrythm::structure::arrangement
{
class LaneOwnedObject : virtual public TimelineObject
{
public:
  using MidiLane = structure::tracks::MidiLane;
  using AudioLane = structure::tracks::AudioLane;

  // = default deletes it for some reason on gcc
  LaneOwnedObject () noexcept { };
  ~LaneOwnedObject () noexcept override = default;
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
  void set_lane (structure::tracks::TrackLanePtrVariant lane)
  {
    owner_lane_ = lane;
  }

  void
  copy_members_from (const LaneOwnedObject &other, ObjectCloneType clone_type)
  {
    index_in_prev_lane_ = other.index_in_prev_lane_;
  }

private:
  /** Track lane that owns this object. */
  std::optional<structure::tracks::TrackLanePtrVariant> owner_lane_;

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

  BOOST_DESCRIBE_CLASS (LaneOwnedObject, (TimelineObject), (), (), ())
};

class MidiRegion;
class AudioRegion;
using LaneOwnedObjectVariant = std::variant<MidiRegion, AudioRegion>;
using LaneOwnedObjectPtrVariant = to_pointer_variant<LaneOwnedObjectVariant>;

}
