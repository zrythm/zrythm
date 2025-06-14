// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
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

#pragma once

#include "dsp/itransport.h"
#include "utils/types.h"

namespace zrythm::dsp::graph
{

/**
 * @brief Interface for objects that can be processed in the DSP graph.
 *
 * The IProcessable interface defines the basic functionality required for
 * objects that can be processed as part of the DSP graph. It provides methods
 * to get the node name, the single playback latency, and to process the block
 * of audio/MIDI data.
 *
 * Implementations of this interface are expected to be used as nodes in the
 * DSP graph, where they will be processed during the audio/MIDI processing
 * cycle.
 */
class IProcessable
{
public:
  virtual ~IProcessable () { release_resources (); }

  /**
   * Returns a human friendly name of the node.
   */
  virtual utils::Utf8String get_node_name () const = 0;

  /**
   * Returns the latency of only the given processable, without adding the
   * previous/next latencies.
   */
  [[gnu::hot]] virtual nframes_t get_single_playback_latency () const
  {
    return 0;
  }

  /**
   * @brief Called to allocate resources required for processing.
   *
   * @param sample_rate
   * @param max_block_length
   */
  virtual void
  prepare_for_processing (sample_rate_t sample_rate, nframes_t max_block_length)
  {
  }

  /**
   * @brief Called to perform processing.
   *
   * @param time_nfo Time block to perform processing for.
   * @param inputs Incoming IProcessable signals. The processable may use these
   * eg, to sum their content.
   */
  [[gnu::hot]] virtual void process_block (
    EngineProcessTimeInfo                 time_nfo,
    std::span<const IProcessable * const> inputs) { };

  /**
   * @brief Called to release resources allocated by @ref
   * prepare_for_processing().
   *
   * This may be called multiple times.
   */
  virtual void release_resources () { }
};

class InitialProcessor final : public IProcessable
{
public:
  utils::Utf8String get_node_name () const override
  {
    return u8"Initial Processor";
  }
};

/**
 * @brief Represents a node in a DSP graph.
 *
 * GraphNode is a fundamental building block of the DSP graph, responsible for
 * processing audio/MIDI data. It encapsulates the necessary functions and
 * properties to handle the processing, latency compensation, and connection
 * management within the graph.
 *
 * The class provides the following key features:
 * - Configurable processing function and name getter
 * - Playback latency management and compensation
 * - Ability to connect to other GraphNode instances
 * - Skipping of processing for muting/disabling the node
 *
 * GraphNode is designed to be used as part of the larger DSP graph system,
 * providing the necessary functionality to handle the individual nodes and
 * their interactions.
 */
class GraphNode
{
public:
  using NodeId = int;

  GraphNode (
    NodeId                 id,
    const dsp::ITransport &transport,
    IProcessable          &processable);
  Q_DISABLE_COPY_MOVE (GraphNode)

  /** For general debugging. */
  std::string print_node_to_str () const;

  void print_node () const;

  /**
   * Processes the GraphNode.
   *
   * @param remaining_preroll_frames The number of frames remaining for preroll
   * (as part of playback latency adjustment).
   */
  [[gnu::hot]] void
  process (EngineProcessTimeInfo time_nfo, nframes_t remaining_preroll_frames)
    const;

  nframes_t get_single_playback_latency () const
  {
    return processable_.get_single_playback_latency ();
  }

  /**
   * Sets the playback latency of the given node recursively.
   *
   * Used only when (re)creating the graph.
   *
   * @param dest_latency The total destination latency so far.
   */
  void set_route_playback_latency (nframes_t dest_latency);

  void connect_to (GraphNode &target);

  /**
   * Sets whether processing should be skipped for this node.
   *
   * When set to true, the node's processing function will be bypassed,
   * effectively muting/disabling the node while keeping it in the graph.
   *
   * @param skip True to skip processing, false to process normally
   */
  void set_skip_processing (bool skip) { bypass_ = skip; }

  IProcessable &get_processable () { return processable_; }

private:
  void add_feeds (GraphNode &dest);
  void add_depends (GraphNode &src);

  /**
   * Handles latency compensation when transport is rolling.
   *
   * Adjusts the time info based on the difference between route playback
   * latency and remaining preroll frames. This ensures proper timing when
   * processing nodes with different latencies in the signal chain.
   *
   * @param time_nfo Time info to be adjusted
   * @param remaining_preroll_frames Frames remaining in preroll period
   */
  [[gnu::hot]] void compensate_latency (
    EngineProcessTimeInfo &time_nfo,
    nframes_t              remaining_preroll_frames) const;

  /**
   * Processes audio in chunks when loop points are encountered.
   *
   * Splits processing into multiple chunks when the playhead crosses the
   * transport loop points, ensuring seamless audio playback during looping.
   * Updates time info to handle loop point transitions correctly.
   *
   * @param time_nfo Time info containing frame counts and offsets
   */
  [[gnu::hot]] void process_chunks_after_splitting_at_loop_points (
    EngineProcessTimeInfo &time_nfo) const;

public:
  /** Incoming node count. */
  std::atomic<int> refcount_ = 0;

  /** The playback latency of the node, in samples. */
  nframes_t playback_latency_ = 0;

  /**
   * @brief Outgoing nodes.
   *
   * Downstream nodes to activate when this node has completed processing.
   */
  std::vector<std::reference_wrapper<GraphNode>> childnodes_;

  /** Initial incoming node count. */
  int init_refcount_ = 0;

  /** The route's playback latency so far. */
  nframes_t route_playback_latency_ = 0;

  /* For debugging. These are set by
   * GraphNodeCollection.set_initial_and_terminal_nodes(). */
  bool terminal_ = false;
  bool initial_ = false;

private:
  NodeId node_id_ = 0;

  /**
   * @brief Incoming nodes.
   *
   * Used when creating the graph so we can traverse it backwards to set the
   * latencies.
   */
  std::vector<std::reference_wrapper<GraphNode>> parentnodes_;

  /**
   * @brief This is just a copy of the processables in @ref parentnodes_ to be
   * passed in IProcessable::process_block().
   */
  std::vector<const IProcessable *> parent_processables_;

  const dsp::ITransport &transport_;

  IProcessable &processable_;

  /**
   * @brief Flag to skip processing.
   */
  bool bypass_ = false;
};

/**
 * @brief Manages the collection of graph nodes.
 *
 * This class holds a main collection of graph nodes, as well as subsets of
 * those nodes that are initial trigger nodes and terminal nodes.
 */
class GraphNodeCollection
{
public:
  /**
   * Returns the max playback latency of the trigger nodes.
   *
   * @note Requires calling set_initial_and_terminal_nodes() first.
   */
  nframes_t get_max_route_playback_latency () const;

  /**
   * @brief Updates the latencies of all nodes.
   */
  void update_latencies ();

  /**
   * @brief Updates the initial and terminal nodes based on @ref graph_nodes_.
   */
  void set_initial_and_terminal_nodes ();

  /**
   * @brief To be called when all nodes have been added.
   */
  void finalize_nodes ()
  {
    set_initial_and_terminal_nodes ();
    update_latencies ();
  }

  GraphNode * find_node_for_processable (const IProcessable &processable) const;

  void add_special_node (GraphNode &node);

public:
  /**
   * @brief All nodes in the graph.
   */
  std::vector<std::unique_ptr<GraphNode>> graph_nodes_;

  /**
   * @brief A subset of @ref graph_nodes_ that are trigger nodes.
   *
   * Trigger nodes are nodes without incoming edges. They run
   * concurrently at the start of each cycle to kick off processing.
   */
  std::vector<std::reference_wrapper<GraphNode>> trigger_nodes_;

  /**
   * @brief A subset of @ref graph_nodes_ that are terminal nodes.
   *
   * Terminal nodes are nodes without outgoing edges.
   */
  std::vector<std::reference_wrapper<GraphNode>> terminal_nodes_;

  std::unique_ptr<InitialProcessor> initial_processor_;

  /**
   * @brief Special nodes that are processed separately at the start of
   * each cycle.
   *
   * This is currently used for tempo track ports.
   *
   * Ideally, this hack should be removed and the BPM node should be
   * processed like the other nodes, but I haven't figured how that would
   * work yet due to changes in BPM requiring position conversions. Maybe
   * schedule the BPM node to be processed first in the graph? What about
   * splitting at n samples or at loop points (how does that affect position
   * conversions)? Should there even be a BPM node?
   */
  std::vector<std::reference_wrapper<GraphNode>> special_nodes_;
};

} // namespace zrythm::dsp::graph
