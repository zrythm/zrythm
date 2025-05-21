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

#ifndef ZRYTHM_DSP_GRAPH_SCHEDULER_H
#define ZRYTHM_DSP_GRAPH_SCHEDULER_H

#include <semaphore>

#include "dsp/graph_node.h"
#include "utils/mpmc_queue.h"
#include "utils/rt_thread_id.h"

namespace zrythm::dsp
{

class GraphThread;

/**
 * @brief Manages the scheduling and execution of a graph of DSP nodes.
 *
 * The GraphScheduler class is responsible for managing the lifecycle of the
 * graph nodes, including starting and terminating the processing threads,
 * triggering the processing of nodes, and coordinating the overall execution
 * of the graph.
 *
 * The class provides methods to rechain the graph nodes, start and terminate
 * the processing threads, and run a processing cycle. It also includes
 * various internal data structures and synchronization primitives to
 * facilitate the efficient parallel processing of the graph.
 */
class GraphScheduler
{
  friend class GraphThread;
  using GraphThreadPtr = std::unique_ptr<GraphThread>;

public:
  static constexpr int MAX_GRAPH_THREADS = 128;

public:
  GraphScheduler ();
  ~GraphScheduler ();
  Z_DISABLE_COPY_MOVE (GraphScheduler); // copy/move don't make sense here

  /**
   * @brief Steals the nodes from the given collection and prepares for
   * processing.
   */
  void rechain_from_node_collection (dsp::GraphNodeCollection &&nodes);

  /**
   * Starts the threads that will be processing the graph.
   *
   * @param num_threads Number of threads to use. If not set, uses an
   * appropriate number based on the number of cores. If set, the number will be
   * clamped to reasonable bounds.
   * @throw ZrythmException on failure.
   */
  void start_threads (std::optional<int> num_threads = std::nullopt);

  /**
   * Tell all threads to terminate.
   */
  void terminate_threads ();

  /**
   * Called when an upstream (parent) node has completed processing.
   */
  [[gnu::hot]] void trigger_node (GraphNode &node);

  auto &get_nodes () { return graph_nodes_; }

  void
  run_cycle (EngineProcessTimeInfo time_nfo, nframes_t remaining_preroll_frames);

  /**
   * @brief Returns whether the given thread is one of the graph threads.
   */
  bool contains_thread (RTThreadId::IdType thread_id);

  auto get_time_nfo () { return time_nfo_; }
  auto get_remaining_preroll_frames () { return remaining_preroll_frames_; }

  void clear_external_output_buffers ();

private:
  std::vector<GraphThreadPtr> threads_;
  GraphThreadPtr              main_thread_;

  std::vector<std::reference_wrapper<dsp::IProcessable>>
    processables_that_need_external_buffer_clear_when_returning_early_from_processing_cycle_;

  /**
   * @brief Time info for the current process cycle.
   */
  EngineProcessTimeInfo time_nfo_;

  /**
   * @brief Number of preroll frames remaining in the current process cycle.
   */
  nframes_t remaining_preroll_frames_{};

  /** Synchronization with main process callback. */
  /* FIXME: this should probably be binary semaphore but i left it as a
   * counting one out of caution while refactoring from ZixSem */
  std::counting_semaphore<MAX_GRAPH_THREADS> callback_start_sem_{ 0 };
  std::binary_semaphore                      callback_done_sem_{ 0 };

  /** Number of threads waiting for work. */
  std::atomic<int> idle_thread_cnt_ = 0;

  /** Wake up graph node process threads. */
  std::counting_semaphore<MAX_GRAPH_THREADS> trigger_sem_{ 0 };

  /** Queue containing nodes that can be processed. */
  MPMCQueue<GraphNode *> trigger_queue_;

  /**
   * @brief Live graph nodes.
   */
  dsp::GraphNodeCollection graph_nodes_;

  /** Remaining unprocessed terminal nodes in this cycle. */
  std::atomic<int> terminal_refcnt_ = 0;
};

} // namespace zrythm::dsp

#endif
