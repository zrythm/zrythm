// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "common/dsp/channel.h"
#include "common/dsp/group_target_track.h"
#include "common/dsp/router.h"
#include "common/dsp/track.h"
#include "common/dsp/track_processor.h"
#include "common/dsp/tracklist.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

void
GroupTargetTrack::update_child_output (
  Channel *          ch,
  GroupTargetTrack * output,
  bool               recalc_graph,
  bool               pub_events)
{
  z_return_if_fail (ch);

  if (ch->has_output_)
    {
      auto track = ch->get_output_track ();
      /* disconnect Channel's output from the current output channel */
      switch (track->in_signal_type_)
        {
        case PortType::Audio:
          ch->stereo_out_->get_l ().disconnect_from (
            *PORT_CONNECTIONS_MGR, track->processor_->stereo_in_->get_l ());

          ch->stereo_out_->get_r ().disconnect_from (
            *PORT_CONNECTIONS_MGR, track->processor_->stereo_in_->get_r ());
          break;
        case PortType::Event:
          ch->midi_out_->disconnect_from (
            *PORT_CONNECTIONS_MGR, *track->processor_->midi_in_);
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

          ch->stereo_out_->get_l ().connect_to (
            *PORT_CONNECTIONS_MGR, output->processor_->stereo_in_->get_l (),
            true);
          ch->stereo_out_->get_r ().connect_to (
            *PORT_CONNECTIONS_MGR, output->processor_->stereo_in_->get_r (),
            true);
          break;
        case PortType::Event:
          ch->midi_out_->connect_to (
            *PORT_CONNECTIONS_MGR, *output->processor_->midi_in_, true);
          break;
        default:
          break;
        }
      ch->has_output_ = true;
      ch->output_name_hash_ = output->get_name_hash ();
    }
  else
    {
      ch->has_output_ = false;
      ch->output_name_hash_ = 0;
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
GroupTargetTrack::contains_child (unsigned int child_name_hash)
{
  return std::ranges::find (children_, child_name_hash) != children_.end ();
}

void
GroupTargetTrack::remove_child (
  unsigned int child_name_hash,
  bool         disconnect,
  bool         recalc_graph,
  bool         pub_events)
{
  z_return_if_fail (child_name_hash != get_name_hash ());
  z_return_if_fail (contains_child (child_name_hash));

  auto tracklist = get_tracklist ();

  auto child =
    tracklist->find_track_by_name_hash<ChannelTrack> (child_name_hash);
  z_return_if_fail (child);
  z_debug (
    "removing '%s' from '%s' - disconnect? %d", child->get_name (), get_name (),
    disconnect);

  if (disconnect)
    {
      update_child_output (
        child->get_channel ().get (), nullptr, recalc_graph, pub_events);
    }
  std::erase (children_, child_name_hash);

  z_info (
    "removed '%s' from direct out '%s' - "
    "num children: %zu",
    child->get_name ().c_str (), get_name ().c_str (), children_.size ());
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
  for (auto &child_hash : children_)
    {
      auto track =
        get_tracklist ()->find_track_by_name_hash<ChannelTrack> (child_hash);
      z_return_val_if_fail (IS_TRACK_AND_NONNULL (track), false);
      auto out_track = track->get_channel ()->get_output_track ();
      z_return_val_if_fail (this == out_track, false);
    }

  return true;
}

void
GroupTargetTrack::add_child (
  unsigned int child_name_hash,
  bool         connect,
  bool         recalc_graph,
  bool         pub_events)
{
  z_return_if_fail (IS_TRACK (this));

  z_debug (
    "adding child track with name hash {} to group %s", child_name_hash,
    get_name ());

  if (connect)
    {
      Tracklist * tracklist = get_tracklist ();
      auto        out_track =
        tracklist->find_track_by_name_hash<ChannelTrack> (child_name_hash);
      z_return_if_fail (
        IS_TRACK_AND_NONNULL (out_track) && out_track->get_channel ());
      update_child_output (
        out_track->get_channel ().get (), this, recalc_graph, pub_events);
    }

  children_.push_back (child_name_hash);
}

void
GroupTargetTrack::add_children (
  const std::vector<unsigned int> &children,
  bool                             connect,
  bool                             recalc_graph,
  bool                             pub_events)
{
  for (auto &child_hash : children)
    {
      add_child (child_hash, connect, recalc_graph, pub_events);
    }
}

int
GroupTargetTrack::find_child (unsigned int track_name_hash)
{
  auto it = std::find (children_.begin (), children_.end (), track_name_hash);
  if (it != children_.end ())
    {
      return std::distance (children_.begin (), it);
    }

  return -1;
}

void
GroupTargetTrack::update_children ()
{
  unsigned int name_hash = get_name_hash ();
  for (auto &child_hash : children_)
    {
      auto child =
        get_tracklist ()->find_track_by_name_hash<ChannelTrack> (child_hash);
      z_warn_if_fail (
        IS_TRACK (child) && child->out_signal_type_ == in_signal_type_);
      child->get_channel ()->output_name_hash_ = name_hash;
      z_debug (
        "setting output of track %s [%d] to %s [%d]", child->get_name (),
        child->pos_, get_name (), pos_);
    }
}

void
GroupTargetTrack::copy_members_from (const GroupTargetTrack &other)
{
  children_ = other.children_;
}