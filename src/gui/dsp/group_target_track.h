
// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Common logic for tracks that can be group targets.
 */

#ifndef __AUDIO_GROUP_TARGET_TRACK_H__
#define __AUDIO_GROUP_TARGET_TRACK_H__

#include "gui/dsp/channel_track.h"

/**
 * @brief Abstract base class for a track that can be routed to.
 *
 * Children are always `ChannelTrack`s since they require a channel to route to
 * a target.
 */
class GroupTargetTrack : virtual public ChannelTrack
{
protected:
  GroupTargetTrack () noexcept { };

public:
  ~GroupTargetTrack () override = default;
  Z_DISABLE_COPY_MOVE (GroupTargetTrack)

  /**
   * Updates the track's children.
   *
   * Used when changing track positions.
   */
  virtual void update_children () final;

  /**
   * Removes a child track from the list of children.
   */
  virtual void remove_child (
    Track::Uuid child_id,
    bool        disconnect,
    bool        recalc_graph,
    bool        pub_events) final;

  /**
   * Remove all known children.
   *
   * @param disconnect Also route the children to "None".
   */
  void
  remove_all_children (bool disconnect, bool recalc_graph, bool pub_events);

  /**
   * Adds a child track to the list of children.
   *
   * @param connect Connect the child to the group track.
   */
  void add_child (
    Track::Uuid child_id,
    bool        connect,
    bool        recalc_graph,
    bool        pub_events);

  void add_children (
    std::span<const Track::Uuid> children,
    bool                         connect,
    bool                         recalc_graph,
    bool                         pub_events);

  /**
   * Returns the index of the child matching the given hash.
   */
  int find_child (Track::Uuid track_name_hash);

protected:
  void
  copy_members_from (const GroupTargetTrack &other, ObjectCloneType clone_type);

  bool validate_base () const;

private:
  static constexpr auto kChildrenKey = "children"sv;
  friend void to_json (nlohmann::json &j, const GroupTargetTrack &track)
  {
    j[kChildrenKey] = track.children_;
  }
  friend void from_json (const nlohmann::json &j, GroupTargetTrack &track)
  {
    j.at (kChildrenKey).get_to (track.children_);
  }

  /**
   * Updates the output of the child channel (where the Channel routes to).
   */
  static void update_child_output (
    Channel *          ch,
    GroupTargetTrack * output,
    bool               recalc_graph,
    bool               pub_events);

  bool contains_child (Track::Uuid child_name_hash);

public:
  /**
   * Name hashes of tracks that are routed to this track, if group track.
   *
   * This is used when undoing track deletion.
   */
  std::vector<Track::Uuid> children_;
};

#endif /* __AUDIO_GROUP_TARGET_TRACK_H__ */
