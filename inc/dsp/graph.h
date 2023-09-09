// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
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

/**
 * \file
 *
 * Routing graph.
 */

#ifndef __AUDIO_GRAPH_H__
#define __AUDIO_GRAPH_H__

#include "dsp/graph_node.h"
#include "utils/types.h"

#include "zix/sem.h"
#include <pthread.h>

typedef struct GraphNode               GraphNode;
typedef struct Graph                   Graph;
typedef struct MPMCQueue               MPMCQueue;
typedef struct Port                    Port;
typedef struct Fader                   Fader;
typedef struct Track                   Track;
typedef struct SampleProcessor         SampleProcessor;
typedef struct Plugin                  Plugin;
typedef struct Position                Position;
typedef struct GraphThread             GraphThread;
typedef struct Router                  Router;
typedef struct ModulatorMacroProcessor ModulatorMacroProcessor;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define mpmc_queue_push_back_node(q, x) mpmc_queue_push_back (q, (void *) x)

#define mpmc_queue_dequeue_node(q, x) mpmc_queue_dequeue (q, (void *) x)

#define MAX_GRAPH_THREADS 128

/**
 * Graph.
 */
typedef struct Graph
{
  /** Pointer back to router for convenience. */
  Router * router;

  /** Flag to indicate if graph is currently getting
   * destroyed. */
  int destroying;

  /** List of all graph nodes (only used for memory
   * management) */
  /** key = internal pointer, value = graph node. */
  GHashTable * graph_nodes;

  /* --- caches for current graph --- */
  GraphNode * bpm_node;
  GraphNode * beats_per_bar_node;
  GraphNode * beat_unit_node;

  /** Nodes without incoming edges.
   * These run concurrently at the start of each
   * cycle to kick off processing */
  GraphNode ** init_trigger_list;
  size_t       n_init_triggers;

  /* Terminal node reference count. */
  /** Number of graph nodes without an outgoing
   * edge. */
  gint         n_terminal_nodes;
  GraphNode ** terminal_nodes;

  /** Remaining unprocessed terminal nodes in this
   * cycle. */
  volatile gint terminal_refcnt;

  /** Synchronization with main process callback. */
  ZixSem callback_start;
  ZixSem callback_done;

  /** Wake up graph node process threads. */
  ZixSem trigger;

  /** Queue containing nodes that can be
   * processed. */
  MPMCQueue * trigger_queue;

  /** Number of entries in trigger queue. */
  volatile guint trigger_queue_size;

  /** flag to exit, terminate all process-threads */
  volatile gint terminate;

  /** Number of threads waiting for work. */
  volatile guint idle_thread_cnt;

  /** Chain used to setup in the background.
   * This is applied and cleared by graph_rechain()
   */
  /** key = internal pointer, value = graph node. */
  GHashTable * setup_graph_nodes;

  GraphNode ** setup_init_trigger_list;
  size_t       num_setup_init_triggers;

  /** Used only when constructing the graph so we
   * can traverse the graph backwards to calculate
   * the playback latencies. */
  GraphNode ** setup_terminal_nodes;
  size_t       num_setup_terminal_nodes;

  /** Dummy member to make lookups work. */
  int initial_processor;

  /* ------------------------------------ */

  GraphThread * threads[MAX_GRAPH_THREADS];
  GraphThread * main_thread;
  gint          num_threads;

  /**
   * An array of pointers to ports that are exposed
   * to the backend and are outputs.
   *
   * Used to clear their buffers when returning
   * early from the processing cycle.
   */
  GPtrArray * external_out_ports;

} Graph;

void
graph_print (Graph * graph);

void
graph_destroy (Graph * graph);

GraphNode *
graph_find_node_from_port (const Graph * self, const Port * port);

GraphNode *
graph_find_node_from_plugin (const Graph * self, const Plugin * pl);

GraphNode *
graph_find_node_from_track (
  const Graph * self,
  const Track * track,
  bool          use_setup_nodes);

GraphNode *
graph_find_node_from_fader (const Graph * self, const Fader * fader);

GraphNode *
graph_find_node_from_prefader (const Graph * self, const Fader * prefader);

GraphNode *
graph_find_node_from_sample_processor (
  const Graph *           self,
  const SampleProcessor * sample_processor);

GraphNode *
graph_find_node_from_monitor_fader (const Graph * self, const Fader * fader);

GraphNode *
graph_find_node_from_modulator_macro_processor (
  const Graph *                   self,
  const ModulatorMacroProcessor * processor);

GraphNode *
graph_find_node_from_channel_send (const Graph * self, const ChannelSend * send);

GraphNode *
graph_find_initial_processor_node (const Graph * self);

GraphNode *
graph_find_hw_processor_node (
  const Graph *             self,
  const HardwareProcessor * processor);

/**
 * Creates a new node, adds it to the graph and
 * returns it.
 */
NONNULL GraphNode *
graph_create_node (Graph * self, GraphNodeType type, void * data);

/**
 * Returns the max playback latency of the trigger
 * nodes.
 */
nframes_t
graph_get_max_route_playback_latency (Graph * graph, bool use_setup_nodes);

/**
 * Called from a terminal node (from the Graph worked-thread)
 * to indicate it has completed processing.
 *
 * The thread of the last terminal node that reaches here will
 * inform the main-thread, wait, and kick off the next
 * process cycle.
 */
HOT void
graph_on_reached_terminal_node (Graph * self);

void
graph_update_latencies (Graph * self, bool use_setup_nodes);

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
graph_setup (Graph * self, const int drop_unnecessary_ports, const int rechain);

/**
 * Adds a new connection for the given
 * src and dest ports and validates the graph.
 *
 * This is a low level function. Better used via
 * ports_can_be_connected().
 *
 * @note The graph should be created before this
 *   call with graph_new() and free'd after this
 *   call with graph_free().
 *
 * @return True if ok, false if invalid.
 */
bool
graph_validate_with_connection (Graph * self, const Port * src, const Port * dest);

/**
 * Starts as many threads as there are cores.
 *
 * @return 1 if graph started, 0 otherwise.
 */
int
graph_start (Graph * graph);

/**
 * Returns a new graph.
 */
Graph *
graph_new (Router * router);

/**
 * Tell all threads to terminate.
 */
void
graph_terminate (Graph * self);

/**
 * Frees the graph and its members.
 */
void
graph_free (Graph * self);

/**
 * @}
 */

#endif
