/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/** \file
 */

#ifndef __AUDIO_ROUTING_H__
#define __AUDIO_ROUTING_H__

#include "utils/sem.h"

typedef struct Graph Router;

typedef enum GraphNodeType
{
  ROUTE_NODE_TYPE_PORT,
  ROUTE_NODE_TYPE_PLUGIN,
} GraphNodeType;

typedef struct GraphNode GraphNode;
typedef struct Graph Graph;
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

  volatile gint refcount;
  gint          init_refcount;

  /** Port, if not a plugin or fader. */
  Port *        port;

  /** Plugin, if plugin. */
  Plugin *      pl;

  /** For debugging. */
  int  terminal;
  int  initial;

  /** Set to a specific number and checked to see
   * if this is a valid node. */
  //int  validate;

  GraphNodeType type;
} GraphNode;

/**
 * Graph.
 */
typedef struct Graph
{
  /** List of all graph nodes (only used for memory management) */
  GraphNode **  graph_nodes;
  int           n_graph_nodes;

	/** Nodes without incoming edges.
	 * These run concurrently at the start of each cycle to kick off processing */
  GraphNode ** init_trigger_list;
  int         n_init_triggers;

  /* Terminal node reference count. */
  /** Number of graph nodes without an outgoing
   * edge. */
  gint          terminal_node_count;

  /** Remaining unprocessed terminal nodes in this
   * cycle. */
  volatile gint terminal_refcnt;

  /** Synchronization with main process callback. */
  ZixSem          callback_start;
  ZixSem          callback_done;

  /** Wake up graph node process threads. */
  ZixSem          trigger;

  /** flag to exit, terminate all process-threads */
  volatile gint     terminate;

  /* these following are protected by
   * _trigger_mutex */
  pthread_mutex_t trigger_mutex;

  /** Number of threads waiting for work. */
  volatile int      idle_thread_cnt;

  /** Working trigger nodes to be updated while
   * processing. */
  GraphNode ** trigger_queue;
  int  n_trigger_queue;
  /** Max size - preallocated array. */
  int  trigger_queue_size;

  /* ------------------------------------ */

#ifdef HAVE_JACK
  jack_native_thread_t threads[16];
  jack_native_thread_t main_thread;
#endif
  gint                  num_threads;

  /** Hash tables for quick finding of nodes based on
   * the ID of the plugin/port. */
  GHashTable * port_nodes;
  GHashTable * plugin_nodes;

} Graph;

/**
 * Creates a graph.
 *
 * Should be used every time the graph is changed.
 */
Graph *
router_new ();

/**
 * Initializes as many RT threads as there are cores,
 * -1.
 */
void
graph_init_threads (
  Graph * graph);

/**
 * Starts a new cycle.
 */
void
router_start_cycle (
  Graph * cache);

void
graph_print (
  Graph * graph);

void
router_destroy (
  Graph * graph);

#endif
