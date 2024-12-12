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
 * ---
 */

#ifndef __AUDIO_GRAPH_H__
#define __AUDIO_GRAPH_H__

#include <semaphore>

#include "dsp/graph_node.h"
#include "gui/dsp/plugin.h"
#include "utils/mpmc_queue.h"
#include "utils/types.h"

class Port;
class SampleProcessor;
class GraphThread;
class Router;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Graph.
 */
class Graph final
{
public:
  using GraphNode = dsp::GraphNode;
  static constexpr int MAX_GRAPH_THREADS = 128;

  class InitialProcessor final : public dsp::IProcessable
  {
  public:
    std::string get_node_name () const override { return "Initial Processor"; }
  };

public:
  Graph (Router * router, SampleProcessor * sample_processor = nullptr);
  ~Graph ();

  void print () const;

  GraphNode *
  find_node_for_processable (const dsp::IProcessable &processable) const;

  /**
   * Returns the max playback latency of the trigger
   * nodes.
   */
  nframes_t get_max_route_playback_latency (bool use_setup_nodes);

  // GraphThread * get_current_thread () const;

  void update_latencies (bool use_setup_nodes);

  /*
   * Adds the graph nodes and connections, then rechains.
   *
   * @param drop_unnecessary_ports Drops any ports that don't connect anywhere.
   * @param rechain Whether to rechain or not. If we are just validating this
   * should be 0.
   */
  void setup (bool drop_unnecessary_ports, bool rechain);

  /**
   * Adds a new connection for the given src and dest ports and validates the
   * graph.
   *
   * @note This should be called on a new instance of Graph.
   *
   * @return Whether the ports can be connected (if the connection will
   * be valid and won't break the acyclicity of the graph).
   */
  bool can_ports_be_connected (const Port &src, const Port &dest);

  /**
   * Starts as many threads as there are cores.
   *
   * @throw ZrythmException on failure.
   */
  void start ();

  /**
   * Tell all threads to terminate.
   */
  void terminate ();

  /**
   * Called when an upstream (parent) node has completed processing.
   */
  ATTR_HOT void trigger_node (GraphNode &node);

private:
  /**
   * @brief Checks for cycles in the graph.
   *
   * @return Whether valid.
   */
  bool is_valid () const;

  void clear_setup ();

  void rechain ();

  void add_plugin (zrythm::gui::old_dsp::plugins::Plugin &pl);

  void connect_plugin (
    zrythm::gui::old_dsp::plugins::Plugin &pl,
    bool                                   drop_unnecessary_ports);

  GraphNode * add_port (
    PortPtrVariant          port_var,
    PortConnectionsManager &mgr,
    bool                    drop_if_unnecessary);

  /**
   * Creates a new node, adds it to the graph and returns it.
   */
  GraphNode * add_node_for_processable (dsp::IProcessable &node);

public:
  std::vector<std::unique_ptr<GraphThread>> threads_;
  std::unique_ptr<GraphThread>              main_thread_;

  /* --- caches for current graph --- */
  GraphNode * bpm_node_ = nullptr;
  GraphNode * beats_per_bar_node_ = nullptr;
  GraphNode * beat_unit_node_ = nullptr;

  /** Synchronization with main process callback. */
  /* FIXME: this should probably be binary semaphore but i left it as a
   * counting one out of caution while refactoring from ZixSem */
  std::counting_semaphore<MAX_GRAPH_THREADS> callback_start_sem_{ 0 };
  std::binary_semaphore                      callback_done_sem_{ 0 };

  /** Number of threads waiting for work. */
  std::atomic<int> idle_thread_cnt_ = 0;

  /** Nodes without incoming edges.
   * These run concurrently at the start of each
   * cycle to kick off processing */
  std::vector<GraphNode *> init_trigger_list_;

  /** Number of graph nodes without an outgoing edge. */
  std::vector<GraphNode *> terminal_nodes_;

  /** Remaining unprocessed terminal nodes in this cycle. */
  std::atomic<int> terminal_refcnt_ = 0;

  /** Wake up graph node process threads. */
  std::counting_semaphore<MAX_GRAPH_THREADS> trigger_sem_{ 0 };

  /** Queue containing nodes that can be processed. */
  MPMCQueue<GraphNode *> trigger_queue_;

  /** Number of entries in trigger queue. */
  std::atomic<int> trigger_queue_size_ = 0;

  using GraphNodesVector = std::vector<std::unique_ptr<GraphNode>>;
  /**
   * @brief List of all graph nodes.
   *
   * This manages the lifetime of graph nodes automatically.
   *
   * The key is a raw pointer to a Plugin, Port, etc.
   */
  GraphNodesVector graph_nodes_vector_;

  /** Pointer back to router for convenience. */
  Router * router_ = nullptr;

  /**
   * An array of pointers to ports that are exposed to the backend and are
   * outputs.
   *
   * Used to clear their buffers when returning early from the processing
   * cycle.
   */
  std::vector<Port *> external_out_ports_;

private:
  /**
   * @brief Chain used to setup in the background.
   *
   * This is applied and cleared by @ref rechain().
   *
   * @see graph_nodes_vector_
   */
  GraphNodesVector setup_graph_nodes_vector_;

  std::vector<GraphNode *> setup_init_trigger_list_;

  /** Used only when constructing the graph so we can traverse the graph
   * backwards to calculate the playback latencies. */
  std::vector<GraphNode *> setup_terminal_nodes_;

  /** Sample processor, if temporary graph for sample processor. */
  SampleProcessor * sample_processor_ = nullptr;

  InitialProcessor initial_processor_;
};

/**
 * @}
 */

#endif
