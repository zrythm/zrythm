// SPDX-FileCopyrightText: © 2020-2022, 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>

#include "engine/session/graph_dispatcher.h"
#include "gui/backend/backend/project.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/group_target_track.h"
#include "structure/tracks/track.h"
#include "structure/tracks/tracklist.h"

namespace zrythm::structure::tracks
{
void
GroupTargetTrack::update_child_output (
  ChannelTrack      &ch_track,
  GroupTargetTrack * output,
  bool               recalc_graph,
  bool               pub_events)
{
  ch_track.set_output (
    (output != nullptr) ? std::make_optional (output->get_uuid ()) : std::nullopt);

  if (recalc_graph)
    {
      ROUTER->recalc_graph (false);
    }

  if (pub_events)
    {
      // EVENTS_PUSH (EventType::ET_CHANNEL_OUTPUT_CHANGED, ch);
    }
}

bool
GroupTargetTrack::contains_child (Track::Uuid child_name_hash)
{
  return std::ranges::find (children_, child_name_hash) != children_.end ();
}

void
GroupTargetTrack::remove_child (
  Track::Uuid child_id,
  bool        disconnect,
  bool        recalc_graph,
  bool        pub_events)
{
  z_return_if_fail (child_id != get_uuid ());
  z_return_if_fail (contains_child (child_id));

  auto * tracklist = get_tracklist ();

  auto child_var = tracklist->get_track (child_id);
  z_return_if_fail (child_var);
  std::visit (
    [&] (auto &&child) {
      using TrackT = base_type<decltype (child)>;
      if constexpr (std::derived_from<TrackT, ChannelTrack>)
        {
          z_debug (
            "removing '%s' from '%s' - disconnect? %d", child->get_name (),
            get_name (), disconnect);

          if (disconnect)
            {
              update_child_output (*child, nullptr, recalc_graph, pub_events);
            }
          std::erase (children_, child_id);

          z_info (
            "removed '%s' from direct out '%s' - "
            "num children: %zu",
            child->get_name ().c_str (), get_name ().c_str (),
            children_.size ());
        }
    },
    *child_var);
}

void
GroupTargetTrack::
  remove_all_children (bool disconnect, bool recalc_graph, bool pub_events)
{
  for (auto &it : std::views::reverse (children_))
    {
      remove_child (it, disconnect, recalc_graph, pub_events);
    }
}

void
GroupTargetTrack::add_child (
  Track::Uuid child_id,
  bool        connect,
  bool        recalc_graph,
  bool        pub_events)
{
  z_debug (
    "adding child track with name hash {} to group %s", child_id, get_name ());

  if (connect)
    {
      Tracklist * tracklist = get_tracklist ();
      auto        out_track_var = tracklist->get_track (child_id);
      std::visit (
        [&] (auto &&out_track) {
          using TrackT = base_type<decltype (out_track)>;
          if constexpr (std::derived_from<TrackT, ChannelTrack>)
            {
              z_return_if_fail (out_track->channel ());
              update_child_output (*out_track, this, recalc_graph, pub_events);
            }
        },
        *out_track_var);
    }

  children_.push_back (child_id);
}

void
GroupTargetTrack::add_children (
  std::span<const Track::Uuid> children,
  bool                         connect,
  bool                         recalc_graph,
  bool                         pub_events)
{
  for (const auto &child_hash : children)
    {
      add_child (child_hash, connect, recalc_graph, pub_events);
    }
}

int
GroupTargetTrack::find_child (Track::Uuid track_id)
{
  auto it = std::ranges::find (children_, track_id);
  if (it != children_.end ())
    {
      return std::distance (children_.begin (), it);
    }

  return -1;
}

void
GroupTargetTrack::update_children ()
{
  auto id = get_uuid ();
  for (auto &child_id : children_)
    {
      auto child_var = get_tracklist ()->get_track (child_id);
      std::visit (
        [&] (auto &&child) {
          using TrackT = base_type<decltype (child)>;
          if constexpr (std::derived_from<TrackT, ChannelTrack>)
            {
              z_return_if_fail (
                child->get_output_signal_type () == in_signal_type_);
              child->set_output (id);
              z_debug (
                "setting output of track {} [{}] to {} [{}]",
                child->get_name (), child->get_index (), get_name (), pos_);
            }
        },
        *child_var);
    }
}

void
init_from (
  GroupTargetTrack       &obj,
  const GroupTargetTrack &other,
  utils::ObjectCloneType  clone_type)
{
  obj.children_ = other.children_;
}
}
