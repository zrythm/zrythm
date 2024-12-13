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

#ifndef ZRYTHM_DSP_GRAPH_H
#define ZRYTHM_DSP_GRAPH_H

#include <semaphore>

#include "dsp/graph_node.h"
#include "utils/mpmc_queue.h"
#include "utils/types.h"

using namespace zrythm;

class GraphThread;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * @brief The Graph class represents a graph of DSP nodes.
 *
 * The Graph class manages the lifecycle of DSP nodes and their connections,
 * allowing for efficient parallel processing of the graph. It provides
 * functionality to start and terminate the processing, add and remove nodes,
 * and manage the overall graph structure.
 */
class Graph final
{
public:
  friend class IGraphBuilder;
  using GraphNode = dsp::GraphNode;
  using GraphThreadPtr = std::unique_ptr<GraphThread>;
  static constexpr int MAX_GRAPH_THREADS = 128;

  class InitialProcessor final : public dsp::IProcessable
  {
  public:
    std::string get_node_name () const override { return "Initial Processor"; }
  };

public:
  Graph ();
  ~Graph ();
  Z_DISABLE_COPY_MOVE (Graph)

  void print () const;

  GraphNode *
  find_node_for_processable (const dsp::IProcessable &processable) const;

  /**
   * Returns the max playback latency of the trigger nodes.
   */
  nframes_t get_max_route_playback_latency (bool use_setup_nodes);

  // GraphThread * get_current_thread () const;

  /**
   * @brief Updates the latencies of all nodes.
   *
   * @param use_setup_nodes
   */
  void update_latencies (bool use_setup_nodes);

  /**
   * @brief Function that creates a thread.
   *
   * @note May throw an exception if the thread cannot be created.
   */
  using ThreadCreateFunc =
    std::function<GraphThreadPtr (bool is_main, int id, Graph &graph)>;

  /**
   * Starts processing the graph.
   *
   * @param ThreadCreateFunc A function that creates a thread.
   * @param num_threads Number of threads to use. If not set, uses an
   * appropriate number based on the number of cores. If set, the number will be
   * clamped to reasonable bounds.
   * @throw ZrythmException on failure.
   */
  void start (
    ThreadCreateFunc   thread_create_func,
    std::optional<int> num_threads = std::nullopt);

  /**
   * Tell all threads to terminate.
   */
  void terminate ();

  /**
   * Called when an upstream (parent) node has completed processing.
   */
  ATTR_HOT void trigger_node (GraphNode &node);

  /**
   * Creates a new node, adds it to the graph and returns it.
   */
  GraphNode * add_node_for_processable (
    dsp::IProcessable     &node,
    const dsp::ITransport &transport);

  GraphNode * add_initial_processor (const dsp::ITransport &transport)
  {
    return add_node_for_processable (initial_processor_, transport);
  };

  void rechain ();

  /**
   * @brief Checks for cycles in the graph.
   *
   * @return Whether valid.
   */
  bool is_valid () const;

private:
  void clear_setup ();

  void set_initial_and_terminal_nodes ();

  /**
   * @brief To be called when done adding nodes to the graph.
   *
   * This will update initial and terminal nodes.
   */
  void finish_adding_nodes ();

public:
  std::vector<GraphThreadPtr> threads_;
  GraphThreadPtr              main_thread_;

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
  std::vector<std::reference_wrapper<GraphNode>> init_trigger_list_;

  /** Number of graph nodes without an outgoing edge. */
  std::vector<std::reference_wrapper<GraphNode>> terminal_nodes_;

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
   */
  GraphNodesVector graph_nodes_vector_;

  /**
   * @brief Function to be called to clear the output buffers of a node.
   */
  using ClearOutputBufferFunc = std::function<void ()>;

  /**
   * An array of pointers to ports that are exposed to the backend and are
   * outputs.
   *
   * Used to clear their buffers when returning early from the processing
   * cycle.
   */
  std::vector<ClearOutputBufferFunc> clear_external_output_buffer_funcs_;

private:
  /**
   * @brief Chain used to setup in the background.
   *
   * This is applied and cleared by @ref rechain().
   *
   * @see graph_nodes_vector_
   */
  GraphNodesVector setup_graph_nodes_vector_;

  std::vector<std::reference_wrapper<GraphNode>> setup_init_trigger_list_;

  /** Used only when constructing the graph so we can traverse the graph
   * backwards to calculate the playback latencies. */
  std::vector<std::reference_wrapper<GraphNode>> setup_terminal_nodes_;

  InitialProcessor initial_processor_;
};

/**
 * @}
 */

#endif
