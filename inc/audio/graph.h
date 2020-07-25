/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (C) 2017, 2019 Robin Gareus <robin@gareus.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Routing graph.
 */

#ifndef __AUDIO_GRAPH_H__
#define __AUDIO_GRAPH_H__

#include <pthread.h>

#include "audio/graph_node.h"
#include "utils/types.h"

#include "zix/sem.h"

typedef struct GraphNode GraphNode;
typedef struct Graph Graph;
typedef struct MPMCQueue MPMCQueue;
typedef struct Port Port;
typedef struct Fader Fader;
typedef struct Track Track;
typedef struct SampleProcessor SampleProcessor;
typedef struct Plugin Plugin;
typedef struct Position Position;
typedef struct GraphThread GraphThread;
typedef struct Router Router;

/**
 * @addtogroup audio
 *
 * @{
 */

#define mpmc_queue_push_back_node(q,x) \
  mpmc_queue_push_back (q, (void *) x)

#define mpmc_queue_dequeue_node(q,x) \
  mpmc_queue_dequeue (q, (void *) x)

#define MAX_GRAPH_THREADS 128

/**
 * Graph.
 */
typedef struct Graph
{
  /** Pointer back to router for convenience. */
  Router *     router;

  /** Flag to indicate if graph is currently getting
   * destroyed. */
  int          destroying;

  /** List of all graph nodes (only used for memory
   * management) */
  GraphNode **  graph_nodes;
  int           n_graph_nodes;

  /** Nodes without incoming edges.
   * These run concurrently at the start of each
   * cycle to kick off processing */
  GraphNode ** init_trigger_list;
  size_t       n_init_triggers;

  /* Terminal node reference count. */
  /** Number of graph nodes without an outgoing
   * edge. */
  gint          n_terminal_nodes;

  /** Remaining unprocessed terminal nodes in this
   * cycle. */
  volatile gint terminal_refcnt;

  /** Synchronization with main process callback. */
  ZixSem          callback_start;
  ZixSem          callback_done;

  /** Wake up graph node process threads. */
  ZixSem          trigger;

  /** Queue containing nodes that can be
   * processed. */
  MPMCQueue *     trigger_queue;

  /** Number of entries in trigger queue. */
  volatile guint  trigger_queue_size;

  /** flag to exit, terminate all process-threads */
  volatile gint     terminate;

  /** Number of threads waiting for work. */
  volatile guint      idle_thread_cnt;

  /** Chain used to setup in the background.
   * This is applied and cleared by graph_rechain()
   */
  GraphNode **         setup_graph_nodes;
  size_t               num_setup_graph_nodes;
  GraphNode **         setup_init_trigger_list;
  size_t               num_setup_init_triggers;

  /** Used only when constructing the graph so we
   * can traverse the graph backwards to calculate
   * the playback latencies. */
  GraphNode **         setup_terminal_nodes;
  size_t               num_setup_terminal_nodes;

  /* ------------------------------------ */

  GraphThread *        threads[MAX_GRAPH_THREADS];
  GraphThread *        main_thread;
  gint                 num_threads;

} Graph;

void
graph_print (
  Graph * graph);

void
graph_destroy (
  Graph * graph);

GraphNode *
graph_find_node_from_port (
  Graph * graph,
  const Port * port);

GraphNode *
graph_find_node_from_plugin (
  Graph * graph,
  Plugin * pl);

GraphNode *
graph_find_node_from_track (
  Graph * graph,
  Track * track);

GraphNode *
graph_find_node_from_fader (
  Graph * graph,
  Fader * fader);

GraphNode *
graph_find_node_from_prefader (
  Graph * graph,
  Fader * prefader);

GraphNode *
graph_find_node_from_sample_processor (
  Graph * graph,
  SampleProcessor * sample_processor);

GraphNode *
graph_find_node_from_monitor_fader (
  Graph * graph,
  Fader * fader);

GraphNode *
graph_find_initial_processor_node (
  Graph * graph);

/**
 * Creates a new node, adds it to the graph and
 * returns it.
 */
GraphNode *
graph_create_node (
  Graph * graph,
  GraphNodeType type,
  void * data);

/**
 * Returns the max playback latency of the trigger
 * nodes.
 */
nframes_t
graph_get_max_playback_latency (
  Graph * graph);

/* called from a terminal node (from the Graph
 * worked-thread) to indicate it has completed
 * processing.
 *
 * The thread of the last terminal node that
 * reaches here will inform the main-thread, wait,
 * and kick off the next process cycle.
 */
void
graph_on_reached_terminal_node (
  Graph *  self);

void
graph_update_latencies (
  Graph * self,
  bool    use_setup_nodes);

/*
 * Adds the graph nodes and connections, then
 * rechains.
 *
 * @param drop_unnecessary_ports Drops any ports
 *   that don't connect anywhere.
 * @param rechain Whether to rechain or not. If
 *   we are just validating this should be 0.
 */
void
graph_setup (
  Graph *   self,
  const int drop_unnecessary_ports,
  const int rechain);

/**
 * Adds a new connection for the given
 * src and dest ports and validates the graph.
 *
 * @note The graph should be created before this call
 *   with graph_new() and free'd after this call with
 *   graph_free().
 *
 * @return True if ok, false if invalid.
 */
bool
graph_validate_with_connection (
  Graph *      self,
  const Port * src,
  const Port * dest);

/**
 * Starts as many threads as there are cores.
 *
 * @return 1 if graph started, 0 otherwise.
 */
int
graph_start (
  Graph * graph);

/**
 * Returns a new graph.
 */
Graph *
graph_new (
  Router * router);

/**
 * Tell all threads to terminate.
 */
void
graph_terminate (
  Graph * self);

/**
 * Frees the graph and its members.
 */
void
graph_free (
  Graph * self);

/**
 * @}
 */

#endif
