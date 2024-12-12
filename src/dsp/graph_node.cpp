// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2017, 2019 Robin Gareus <robin@gareus.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#include <utility>

#include "dsp/graph_node.h"
#include "dsp/itransport.h"
#include "utils/debug.h"

namespace zrythm::dsp
{

GraphNode::GraphNode (
  NodeId                 id,
  const dsp::ITransport &transport,
  dsp::IProcessable     &processable)
    : node_id_ (id), transport_ (transport), processable_ (processable)
{
}

std::string
GraphNode::print_node_to_str () const
{
  std::string name = processable_.get_node_name ();
  std::string str1 = fmt::format (
    "node [({}) {}] refcount: {} | terminal: {} | initial: {} | playback latency: {}",
    node_id_, name, refcount_.load (), terminal_, initial_, playback_latency_);
  std::string str2;
  for (auto * const dest : childnodes_)
    {
      name = dest->processable_.get_node_name ();
      str2 = fmt::format ("{} (dest [({}) {}])", str1, dest->node_id_, name);
      str1 = str2;
    }
  return str1;
}

void
GraphNode::print_node () const
{
  z_info (print_node_to_str ());
}

void
GraphNode::compensate_latency (
  EngineProcessTimeInfo &time_nfo,
  const nframes_t        remaining_preroll_frames) const
{
  /* if the playhead is before the loop-end point and the
   * latency-compensated position is after the loop-end point it means
   * that the loop was crossed, so compensate for that.
   *
   * if the position is before loop-end and position + frames is after
   * loop end (there is a loop inside the range), that should be handled
   * by the ports/processors instead */
  dsp::Position playhead_copy = transport_.get_playhead_position ();
  z_return_if_fail (route_playback_latency_ >= remaining_preroll_frames);
  transport_.position_add_frames (
    playhead_copy, route_playback_latency_ - remaining_preroll_frames);
  time_nfo.g_start_frame_ = (unsigned_frame_t) playhead_copy.frames_;
  time_nfo.g_start_frame_w_offset_ =
    (unsigned_frame_t) playhead_copy.frames_ + time_nfo.local_offset_;
}

void
GraphNode::process_chunks_after_splitting_at_loop_points (
  EngineProcessTimeInfo &time_nfo) const
{
  /* split at loop points */
  for (
    nframes_t num_processable_frames = 0;
    (num_processable_frames = std::min (
       transport_.is_loop_point_met (
         (signed_frame_t) time_nfo.g_start_frame_w_offset_, time_nfo.nframes_),
       time_nfo.nframes_))
    != 0;)
    {
      // z_debug (
      //   "splitting from {} (num processable frames {})",
      //   time_nfo.g_start_frame_w_offset_, num_processable_frames);

      /* temporarily change the nframes to avoid having to declare a separate
       * EngineProcessTimeInfo */
      nframes_t orig_nframes = time_nfo.nframes_;
      time_nfo.nframes_ = num_processable_frames;

      // process current chunk
      processable_.process_block (time_nfo);

      /* calculate the remaining frames */
      time_nfo.nframes_ = orig_nframes - num_processable_frames;

      /* loop back to loop start */
      auto [transport_loop_start_pos, transport_loop_end_pos] =
        transport_.get_loop_range_positions ();
      unsigned_frame_t frames_to_add =
        (num_processable_frames
         + (unsigned_frame_t) transport_loop_start_pos.frames_)
        - (unsigned_frame_t) transport_loop_end_pos.frames_;
      time_nfo.g_start_frame_w_offset_ += frames_to_add;
      time_nfo.g_start_frame_ += frames_to_add;
      time_nfo.local_offset_ += num_processable_frames;
    }
}

void
GraphNode::process (
  EngineProcessTimeInfo time_nfo,
  const nframes_t       remaining_preroll_frames) const
{
  // if node is bypassed, skip processing
  if (bypass_) [[unlikely]]
    {
      return;
    }

  // z_info ("processing {}", get_name ());

  /* skip if we are doing a no-roll */
  if (route_playback_latency_ < remaining_preroll_frames)
    {
      return;
    }

  /* compensate latency when rolling */
  if (transport_.get_play_state () == dsp::ITransport::PlayState::Rolling)
    {
      compensate_latency (time_nfo, remaining_preroll_frames);
    }

  process_chunks_after_splitting_at_loop_points (time_nfo);

  z_return_if_fail_cmp (
    time_nfo.g_start_frame_w_offset_, >=, time_nfo.g_start_frame_);

  if (time_nfo.nframes_ > 0)
    {
      processable_.process_block (time_nfo);
    }
}

void
GraphNode::add_feeds (GraphNode &dest)
{
  /* return if already added */
  for (auto child : childnodes_)
    {
      if (child == &dest)
        {
          return;
        }
    }

  childnodes_.push_back (&dest);

  terminal_ = false;
}

void
GraphNode::add_depends (GraphNode &src)
{
  ++init_refcount_;
  refcount_ = init_refcount_;

  /* add parent nodes */
  parentnodes_.push_back (&src);

  initial_ = false;
}

void
GraphNode::set_route_playback_latency (nframes_t dest_latency)
{
  /*long max_latency = 0, parent_latency;*/

  /* set route playback latency */
  if (dest_latency > route_playback_latency_)
    {
      route_playback_latency_ = dest_latency;
    }
  else
    {
      /* assumed that parent nodes already have the previous
       * latency set */
      /* TODO double-check if this is OK to do, needs testing */
      return;
    }

  for (auto parent : parentnodes_)
    {
      parent->set_route_playback_latency (route_playback_latency_);
#if 0
      z_info (
        "added %d route playback latency from node %s to "
        "parent %s. Total route latency on parent: %d",
        dest_latency,
        graph_node_get_name (node),
        graph_node_get_name (parent),
        parent->route_playback_latency);
#endif
    }
}

void
GraphNode::connect_to (GraphNode &target)
{
  if (
    std::find (childnodes_.begin (), childnodes_.end (), &target)
    != childnodes_.end ())
    return;

  add_feeds (target);
  target.add_depends (*this);

  z_warn_if_fail (!terminal_ && !target.initial_);
}

} // namespace zrythm::dsp
