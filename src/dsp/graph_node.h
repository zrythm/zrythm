// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
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
#include "utils/utf8_string.h"

#include <QObject>

namespace zrythm::dsp::graph
{
class GraphNode;

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
   * @param node The node in the processing graph. Null means we are processing
   * outside of a graph context and must be handled gracefully.
   * @param sample_rate
   * @param max_block_length
   */
  virtual void prepare_for_processing (
    const GraphNode *    node,
    units::sample_rate_t sample_rate,
    nframes_t            max_block_length)
  {
  }

  [[gnu::hot]] virtual void process_block (
    EngineProcessTimeInfo  time_nfo,
    const dsp::ITransport &transport) noexcept [[clang::nonblocking]] { };

  /**
   * @brief Called to release resources allocated by @ref
   * prepare_for_processing().
   *
   * This may be called multiple times.
   */
  virtual void release_resources () { }
};

class InitialProcessor final : public QObject, public IProcessable
{
  Q_OBJECT

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

  GraphNode (NodeId id, IProcessable &processable);
  Z_DISABLE_COPY_MOVE (GraphNode)
  ~GraphNode () noexcept = default;

  /** For general debugging. */
  std::string print_node_to_str () const;

  void print_node () const;

  // For debugging purposes.
  NodeId get_id () const { return node_id_; }

  /**
   * Processes the GraphNode.
   *
   * @param remaining_preroll_frames The number of frames remaining for preroll
   * (as part of playback latency adjustment).
   * @param processing_cycle_in_progress Whether the processing cycle is
   * currently in progress (and this function is called as part of it), as
   * opposed to being called before/after a processing cycle (e.g., for some
   * special nodes that are processed before/after the actual processing).
   */
  [[gnu::hot]] void process (
    EngineProcessTimeInfo  time_nfo,
    nframes_t              remaining_preroll_frames,
    const dsp::ITransport &transport) const;

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

  /**
   * @brief Read-only access to child nodes (outgoing connections).
   */
  auto &feeds () const { return childnodes_; }

  /**
   * @brief Read-only access to parent nodes (incoming connections).
   */
  auto &depends () const { return parentnodes_; }

  bool remove_feed (const GraphNode &feed);
  bool remove_depend (const GraphNode &depend);

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
    nframes_t              remaining_preroll_frames,
    const dsp::ITransport &transport) const;

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
    EngineProcessTimeInfo &time_nfo,
    const dsp::ITransport &transport) const;

public:
  /** Incoming node count. */
  std::atomic<int> refcount_ = 0;

  /**
   * @brief The playback latency of the node, in samples.
   *
   * @see Page 116 of "The Ardour DAW - Latency Compensation and
   * Anywhere-to-Anywhere Signal Routing Systems".
   */
  nframes_t playback_latency_ = 0;

  // TODO
  /**
   * @brief The capture latency of the node, in samples.
   *
   * @see Page 116 of "The Ardour DAW - Latency Compensation and
   * Anywhere-to-Anywhere Signal Routing Systems".
   */
  nframes_t capture_latency_ = 0;

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
   * @brief Outgoing nodes.
   *
   * Downstream nodes to activate when this node has completed processing.
   */
  std::vector<std::reference_wrapper<GraphNode>> childnodes_;

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
   * @note Requires calling update_latencies() first.
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
   * @brief Sets the initial/terminal nodes.
   *
   * To be called when all nodes have been added.
   */
  void finalize_nodes () { set_initial_and_terminal_nodes (); }

  GraphNode * find_node_for_processable (const IProcessable &processable) const;

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
};

} // namespace zrythm::dsp::graph
