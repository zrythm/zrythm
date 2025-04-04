// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/channel.h"
#include "gui/dsp/group_target_track.h"
#include "gui/dsp/router.h"
#include "gui/dsp/track.h"
#include "gui/dsp/track_processor.h"
#include "gui/dsp/tracklist.h"
#include "utils/rt_thread_id.h"

void
GroupTargetTrack::update_child_output (
  Channel *          ch,
  GroupTargetTrack * output,
  bool               recalc_graph,
  bool               pub_events)
{
  z_return_if_fail (ch);

  if (ch->has_output ())
    {
      auto track = ch->get_output_track ();
      /* disconnect Channel's output from the current output channel */
      switch (track->in_signal_type_)
        {
        case PortType::Audio:
          PORT_CONNECTIONS_MGR->ensure_disconnect (
            ch->get_stereo_out_ports ().first.get_uuid (),
            *track->processor_->stereo_in_left_id_);
          PORT_CONNECTIONS_MGR->ensure_disconnect (
            ch->get_stereo_out_ports ().second.get_uuid (),
            *track->processor_->stereo_in_right_id_);
          break;
        case PortType::Event:
          PORT_CONNECTIONS_MGR->ensure_disconnect (
            ch->get_midi_out_port ().get_uuid (),
            track->processor_->get_midi_in_port ().get_uuid ());
          break;
        default:
          break;
        }
    }

  if (output)
    {
      /* connect Channel's output to the given output */
      switch (output->in_signal_type_)
        {
        case PortType::Audio:
          PORT_CONNECTIONS_MGR->ensure_connect_default (
            ch->get_stereo_out_ports ().first.get_uuid (),
            *output->processor_->stereo_in_left_id_, true);
          PORT_CONNECTIONS_MGR->ensure_connect_default (
            ch->get_stereo_out_ports ().second.get_uuid (),
            *output->processor_->stereo_in_right_id_, true);
          break;
        case PortType::Event:
          PORT_CONNECTIONS_MGR->ensure_connect_default (
            ch->get_midi_out_port ().get_uuid (),
            output->processor_->get_midi_in_port ().get_uuid (), true);
          break;
        default:
          break;
        }
      ch->output_track_uuid_ = output->get_uuid ();
    }
  else
    {
      ch->output_track_uuid_ = std::nullopt;
    }

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
              update_child_output (
                child->get_channel (), nullptr, recalc_graph, pub_events);
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
  for (auto it = children_.rbegin (); it != children_.rend (); ++it)
    {
      remove_child (*it, disconnect, recalc_graph, pub_events);
    }
}

bool
GroupTargetTrack::validate_base () const
{
  for (const auto &child_id : children_)
    {
      auto track_var = get_tracklist ()->get_track (child_id);
      auto ret = std::visit (
        [&] (auto &&track) {
          using TrackT = base_type<decltype (track)>;
          if constexpr (std::derived_from<TrackT, ChannelTrack>)
            {
              auto out_track = track->get_channel ()->get_output_track ();
              z_return_val_if_fail (this == out_track, false);
            }
          return true;
        },
        *track_var);
      z_return_val_if_fail (ret, false);
    }

  return true;
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
              z_return_if_fail (out_track->get_channel ());
              update_child_output (
                out_track->get_channel (), this, recalc_graph, pub_events);
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
              z_return_if_fail (child->out_signal_type_ == in_signal_type_);
              child->get_channel ()->output_track_uuid_ = id;
              z_debug (
                "setting output of track {} [{}] to {} [{}]",
                child->get_name (), child->pos_, get_name (), pos_);
            }
        },
        *child_var);
    }
}

void
GroupTargetTrack::copy_members_from (
  const GroupTargetTrack &other,
  ObjectCloneType         clone_type)
{
  children_ = other.children_;
}
