/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __AUDIO_ROUTING_H__
#define __AUDIO_ROUTING_H__

#include <pthread.h>

#include "utils/types.h"

#include "zix/sem.h"

typedef struct GraphNode GraphNode;
typedef struct Graph Graph;
typedef struct PassthroughProcessor
  PassthroughProcessor;
typedef struct MPMCQueue MPMCQueue;

/**
 * @addtogroup audio
 *
 * @{
 */

#define mpmc_queue_push_back_node(q,x) \
  mpmc_queue_push_back (q, (void *) x)

#define mpmc_queue_dequeue_node(q,x) \
  mpmc_queue_dequeue (q, (void *) x)

/**
 * Graph nodes can be either ports or processors.
 *
 * Processors can be plugins, faders, etc.
 */
typedef enum GraphNodeType
{
  /** Port. */
  ROUTE_NODE_TYPE_PORT,
  /** Plugin processor. */
  ROUTE_NODE_TYPE_PLUGIN,
  /** Track processor. */
  ROUTE_NODE_TYPE_TRACK,
  /** Fader/pan processor. */
  ROUTE_NODE_TYPE_FADER,
  /** Fader/pan processor for monitor. */
  ROUTE_NODE_TYPE_MONITOR_FADER,
  /** Pre-Fader passthrough processor. */
  ROUTE_NODE_TYPE_PREFADER,
  /** Sample processor. */
  ROUTE_NODE_TYPE_SAMPLE_PROCESSOR,
} GraphNodeType;

typedef struct GraphNode
{
  int           id;
  /** Ref back to the graph so we don't have to
   * pass it around. */
  Graph *      graph;

  /** outgoing edges
	 * downstream nodes to activate when this node
   * has completed processed
	 */
  GraphNode **  childnodes;
  int           n_childnodes;

  /** Incoming node count. */
  volatile gint refcount;

  /** Initial incoming node count. */
  gint          init_refcount;

  /** Used when creating the graph so we can
   * traverse it backwards to set the latencies. */
  GraphNode **  parentnodes;

  /** Port, if not a plugin or fader. */
  Port *        port;

  /** Plugin, if plugin. */
  Plugin *      pl;

  /** Fader, if fader. */
  Fader *       fader;

  Track *       track;

  /** Pre-Fader, if prefader node. */
  PassthroughProcessor * prefader;

  /** Sample processor, if sample processor. */
  SampleProcessor * sample_processor;

  /** For debugging. */
  int  terminal;
  int  initial;

  /** The playback latency of the node, in
   * samples. */
  nframes_t          playback_latency;

  /** The route's playback latency. */
  nframes_t          route_playback_latency;

  GraphNodeType type;
} GraphNode;

typedef struct Router Router;

typedef struct GraphThread
{
#ifdef HAVE_JACK
  jack_native_thread_t jthread;
#endif
  pthread_t            pthread;

  /**
   * Thread index in zrythm.
   *
   * The main thread will be -1 and the rest in
   * sequence starting from 0.
   */
  int                  id;

  /** Pointer back to the graph. */
  Graph *              graph;
} GraphThread;

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

  GraphThread          threads[16];
  GraphThread          main_thread;
  gint                 num_threads;

} Graph;

typedef struct Router
{
  Graph * graph;

  /** Number of samples to process in this cycle. */
  nframes_t   nsamples;

  /** Stored for the currently processing cycle */
  nframes_t   max_playback_latency;

  /** Current global latency offset (max latency
   * of all routes - remaining latency from
   * engine). */
  nframes_t   global_offset;

  /** Offset in the current cycle. */
  nframes_t   local_offset;

  /** Used when recalculating the graph. */
  ZixSem               graph_access;

} Router;

void
router_init (
  Router * router);

/**
 * Starts a new cycle.
 *
 * @param local_offset The local offset to start
 *   playing from in this cycle:
 *   (0 - <engine buffer size>)
 */
void
router_start_cycle (
  Router *         self,
  const nframes_t  nsamples,
  const nframes_t  local_offset,
  const Position * pos);

/**
 * Returns the max playback latency of the trigger
 * nodes.
 */
nframes_t
router_get_max_playback_latency (
  Router * router);

/**
 * Returns if the current thread is a
 * processing thread.
 */
int
router_is_processing_thread (
  Router * router);

void
graph_print (
  Graph * graph);

void
graph_destroy (
  Graph * graph);

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
 * @return 1 for ok, 0 for invalid.
 */
int
graph_validate (
  Router *     router,
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
 * Frees the graph and its members.
 */
void
graph_free (
  Graph * self);

void
router_tear_down (
  Router * self);

/**
 * @}
 */

#endif
