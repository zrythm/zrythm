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
 */

/**
 * \file
 *
 * Routing graph.
 */

#ifndef __AUDIO_ROUTING_H__
#define __AUDIO_ROUTING_H__

#include <pthread.h>
#include "zix/sem.h"

typedef struct GraphNode GraphNode;
typedef struct Graph Graph;
typedef struct PassthroughProcessor
  PassthroughProcessor;

/**
 * @addtogroup audio
 *
 * @{
 */

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
  /** Fader/pan processor. */
  ROUTE_NODE_TYPE_FADER,
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

  /** Pre-Fader, if prefader node. */
  PassthroughProcessor * prefader;

  /** Sample processor, if sample processor. */
  SampleProcessor * sample_processor;

  /** For debugging. */
  int  terminal;
  int  initial;

  /** The playback latency of the node, in
   * samples. */
  long          playback_latency;

  /** The route's playback latency. */
  long          route_playback_latency;

  /** Set to a specific number and checked to see
   * if this is a valid node. */
  //int  validate;

  GraphNodeType type;
} GraphNode;

typedef struct Router Router;

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

  /** List of all graph nodes (only used for memory management) */
  GraphNode **  graph_nodes;
  int           n_graph_nodes;

	/** Nodes without incoming edges.
	 * These run concurrently at the start of each
   * cycle to kick off processing */
  GraphNode ** init_trigger_list;
  int         n_init_triggers;

  /* Terminal node reference count. */
  /** Number of graph nodes without an outgoing
   * edge. */
  gint          terminal_node_count;

  /** Used only when constructing the graph so we
   * can traverse the graph backwards to calculate
   * the playback latencies. */
  GraphNode *   terminal_nodes[20];

  /** Remaining unprocessed terminal nodes in this
   * cycle. */
  volatile gint terminal_refcnt;

  /** Working trigger nodes to be updated while
   * processing. */
  GraphNode ** trigger_queue;
  int  n_trigger_queue;
  /** Max size - preallocated array. */
  int  trigger_queue_size;

  /** Synchronization with main process callback. */
  ZixSem          callback_start;
  ZixSem          callback_done;

  /** Wake up graph node process threads. */
  ZixSem          trigger;

  /* these following are protected by
   * _trigger_mutex */
  pthread_mutex_t trigger_mutex;

  /** flag to exit, terminate all process-threads */
  volatile gint     terminate;

  /** Number of threads waiting for work. */
  volatile int      idle_thread_cnt;

  /* ------------------------------------ */

  /**
   * Threads.
   *
   * If backend is JACK, jack threads are used.
   * */
#ifdef HAVE_JACK
  jack_native_thread_t jthreads[16];
  jack_native_thread_t jmain_thread;
#endif
  pthread_t            main_thread;
  pthread_t            threads[16];
  gint                 num_threads;

} Graph;

typedef struct Router
{
  Graph * graph1;
  Graph * graph2;

  /** Number of samples to process in this cycle. */
  int     nsamples;

  /** Stored for the currently processing cycle */
  long    max_playback_latency;

  /** Current global latency offset (max latency
   * of all routes - remaining latency from
   * engine). */
  int     global_offset;

  /** Offset in the current cycle. */
  int     local_offset;

} Router;

/**
 * Returns a new graph, or NULL if the graph is
 * invalid.
 *
 * Optionally adds a new connection for the given
 * src and dest ports.
 *
 * Should be used every time the graph is changed.
 */
Graph *
graph_new (
  Router * router,
  Port *   src,
  Port *   dest);

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
  const int        nsamples,
  const int        local_offset,
  const Position * pos);

/**
 * Returns the max playback latency of the trigger
 * nodes.
 */
long
router_get_max_playback_latency (
  Router * router);

void
graph_print (
  Graph * graph);

void
graph_destroy (
  Graph * graph);

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
