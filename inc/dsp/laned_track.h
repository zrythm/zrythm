// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_LANED_TRACK_H__
#define __AUDIO_LANED_TRACK_H__

#include "dsp/audio_region.h"
#include "dsp/midi_region.h"
#include "dsp/track.h"
#include "dsp/track_lane.h"

/**
 * Interface for a track that has lanes.
 */
class LanedTrack : virtual public Track
{
public:
  virtual ~LanedTrack () = default;

  /**
   * Set lanes visible and fire events.
   */
  void set_lanes_visible (bool visible);

public:
  /** Flag to set lanes visible or not. */
  bool lanes_visible_ = false;

  /** Lane index of region before recording paused. */
  int last_lane_idx_ = 0;

  /**
   * Last lane created during this drag.
   *
   * This is used to prevent creating infinite lanes when you want to track a
   * region from the last lane to the track below. Only 1 new lane will be
   * created in case the user wants to move the region to a new lane instead
   * of the track below.
   *
   * Used when moving regions vertically.
   */
  int last_lane_created_ = 0;

  /** Block auto-creating or deleting lanes. */
  bool block_auto_creation_and_deletion_ = false;
};

/**
 * Interface for a track that has lanes.
 */
template <typename RegionT>
class LanedTrackImpl
    : virtual public LanedTrack,
      public ISerializable<LanedTrackImpl<RegionT>>
{
public:
  // Rule of 0
  LanedTrackImpl () { add_lane (false); }

  virtual ~LanedTrackImpl () = default;

  using RegionType = RegionT;
  using TrackLaneT = TrackLaneImpl<RegionT>;

  // void init () override { add_lane (false); }

  void init_loaded () override
  {
    for (auto &lane : lanes_)
      {
        lane->init_loaded (this);
      }
  }

  /**
   * Creates missing TrackLane's until pos.
   *
   * @return Whether a new lane was created.
   */
  bool create_missing_lanes (const int pos)
  {
    if (block_auto_creation_and_deletion_)
      return false;

    bool created_new_lane = false;
    while (static_cast<size_t> (pos + 2) > lanes_.size ())
      {
        add_lane (false);
        created_new_lane = true;
      }
    return created_new_lane;
  }

  /**
   * Removes the empty last lanes of the Track (except the last one).
   */
  void remove_empty_last_lanes ();

  /**
   * Returns whether the track has any soloed lanes.
   */
  bool has_soloed_lanes () const
  {
    return std::ranges::any_of (lanes_, [] (const auto &lane) {
      return lane->get_soloed ();
    });
  }

  /**
   * @brief Gets the total height of all visible lanes (if any).
   */
  double get_visible_lane_heights () const;

  /**
   * Generates lanes for the track.
   *
   * Should be called as soon as the track is created.
   */
  void generate_lanes () { add_lane (true); }

  void clear_objects () override;

  void get_regions_in_range (
    std::vector<Region *> &regions,
    const Position *       p1,
    const Position *       p2) override
  {
    for (auto &lane : lanes_)
      {
        for (auto &region : lane->regions_)
          {
            add_region_if_in_range (p1, p2, regions, region.get ());
          }
      }
  }

protected:
  void copy_members_from (const LanedTrackImpl &other)
  {
    clone_unique_ptr_container (lanes_, other.lanes_);
    for (auto &lane : lanes_)
      {
        lane->track_ = this;
      }
    lanes_visible_ = other.lanes_visible_;
  }

  bool validate_base () const;

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

  void set_playback_caches () override;

  void update_name_hash (unsigned int new_name_hash) override;

private:
  void add_lane (bool fire_events);

public:
  /** Lanes in this track containing Regions. */
  std::vector<std::unique_ptr<TrackLaneT>> lanes_;

  /** Snapshots used during playback. */
  std::vector<std::unique_ptr<TrackLaneT>> lane_snapshots_;
};

using LanedTrackVariant = std::variant<MidiTrack, InstrumentTrack, AudioTrack>;
using LanedTrackPtrVariant = to_pointer_variant<LanedTrackVariant>;

extern template class LanedTrackImpl<MidiRegion>;
extern template class LanedTrackImpl<AudioRegion>;

#endif // __AUDIO_LANED_TRACK_H__