// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/midi_region.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_lane.h"
#include "structure/tracks/track_lane_list.h"

#define DEFINE_LANED_TRACK_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* lanesVisible */ \
  /* ================================================================ */ \
  Q_PROPERTY ( \
    bool lanesVisible READ getLanesVisible WRITE setLanesVisible NOTIFY \
      lanesVisibleChanged) \
  bool getLanesVisible () const \
  { \
    return lanes_visible_; \
  } \
  void setLanesVisible (bool visible) \
  { \
    if (lanes_visible_ == visible) \
      return; \
\
    lanes_visible_ = visible; \
    Q_EMIT lanesVisibleChanged (visible); \
  } \
\
  Q_SIGNAL void lanesVisibleChanged (bool visible); \
\
  /* ================================================================ */ \
  /* lanes */ \
  /* ================================================================ */ \
  Q_PROPERTY (TrackLaneList * lanes READ getLanes CONSTANT) \
  TrackLaneList * getLanes () const \
  { \
    return const_cast<TrackLaneList *> (&lanes_); \
  }

namespace zrythm::structure::tracks
{
/**
 * Interface for a track that has lanes.
 */
class LanedTrack : virtual public Track
{
public:
  ~LanedTrack () noexcept override = default;

  /**
   * Set lanes visible and fire events.
   */
  void set_lanes_visible (bool visible);

protected:
  LanedTrack () noexcept { }

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
template <typename TrackLaneT> class LanedTrackImpl : public LanedTrack
{
public:
  using RegionT = typename TrackLaneT::RegionT;
  using TrackLaneType = TrackLaneT;

  LanedTrackImpl ();

  ~LanedTrackImpl () override = default;

  void
  init_loaded (PluginRegistry &plugin_registry, PortRegistry &port_registry)
    override
  {
  }

  /**
   * Creates missing TrackLane's until pos.
   *
   * @return Whether a new lane was created.
   */
  bool create_missing_lanes (int pos);

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
      return std::get<TrackLaneT *> (lane)->get_soloed ();
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
  void generate_lanes () { add_lane (); }

  void clear_objects () override;

  void get_regions_in_range (
    std::vector<Region *> &regions,
    const dsp::Position *  p1,
    const dsp::Position *  p2) override;

  int get_lane_index (const TrackLaneT &lane) const
  {
    return std::ranges::find_if (
             lanes_.lanes_,
             [&] (const auto &lane_var) {
               return std::get<TrackLaneT *> (lane_var) == std::addressof (lane);
             })
           - lanes_.begin ();
  }

  TrackLaneT &get_lane_at (const size_t index)
  {
    return *std::get<TrackLaneT *> (lanes_.at (index));
  }

protected:
  void
  copy_members_from (const LanedTrackImpl &other, ObjectCloneType clone_type);

  bool validate_base () const;

  void set_playback_caches () override;

private:
  static constexpr std::string_view kLanesVisibleKey = "lanesVisible";
  static constexpr std::string_view kLanesListKey = "lanesList";
  friend void to_json (nlohmann::json &j, const LanedTrackImpl &track)
  {
    j[kLanesVisibleKey] = track.lanes_visible_;
    j[kLanesListKey] = track.lanes_;
  }
  friend void from_json (const nlohmann::json &j, LanedTrackImpl &track)
  {
    j.at (kLanesVisibleKey).get_to (track.lanes_visible_);
    j.at (kLanesListKey).get_to (track.lanes_);
  }

  void add_lane ();

public:
  /** Lanes in this track containing Regions. */
  TrackLaneList lanes_;

  /** Snapshots used during playback. */
  std::vector<std::unique_ptr<TrackLaneT>> lane_snapshots_;

  static_assert (TrackLaneSubclass<TrackLaneT>);
};

using LanedTrackVariant = std::variant<MidiTrack, InstrumentTrack, AudioTrack>;
using LanedTrackPtrVariant = to_pointer_variant<LanedTrackVariant>;

extern template class LanedTrackImpl<MidiLane>;
extern template class LanedTrackImpl<AudioLane>;

}
