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
 * Routing graph node.
 */

#ifndef __AUDIO_GRAPH_NODE_H__
#define __AUDIO_GRAPH_NODE_H__

#include <stdbool.h>

#include "utils/types.h"

#include <gtk/gtk.h>

typedef struct GraphNode GraphNode;
typedef struct Graph Graph;
typedef struct PassthroughProcessor
  PassthroughProcessor;
typedef struct Port Port;
typedef struct Fader Fader;
typedef struct Track Track;
typedef struct SampleProcessor SampleProcessor;
typedef struct Plugin Plugin;

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

  /**
   * Initial processor.
   *
   * The initial processor is a dummy processor
   * in the chain processed before anything else.
   */
  ROUTE_NODE_TYPE_INITIAL_PROCESSOR,
} GraphNodeType;

/**
 * A node in the processing graph.
 */
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
  Fader *       prefader;

  /** Sample processor, if sample processor. */
  SampleProcessor * sample_processor;

  /** For debugging. */
  bool          terminal;
  bool          initial;

  /** The playback latency of the node, in
   * samples. */
  nframes_t     playback_latency;

  /** The route's playback latency. */
  nframes_t     route_playback_latency;

  GraphNodeType type;
} GraphNode;

/**
 * Returns a human friendly name of the node.
 *
 * Must be free'd.
 */
char *
graph_node_get_name (
  GraphNode * node);

void *
graph_node_get_pointer (
  GraphNode * self);

void
graph_node_print (
  GraphNode * node);

/**
 * Processes the GraphNode.
 */
void
graph_node_process (
  GraphNode *     node,
  const nframes_t nframes);

/**
 * Returns the latency of only the given port,
 * without adding the previous/next latencies.
 *
 * It returns the plugin's latency if plugin,
 * otherwise 0.
 */
nframes_t
graph_node_get_single_playback_latency (
  GraphNode * node);

/**
 * Sets the playback latency of the given node
 * recursively.
 *
 * Used only when (re)creating the graph.
 *
 * @param dest_latency The total destination
 * latency so far.
 */
void
graph_node_set_playback_latency (
  GraphNode * node,
  nframes_t   dest_latency);

/**
 * Called by an upstream node when it has completed
 * processing.
 */
void
graph_node_trigger (
  GraphNode * self);

//void
//graph_node_add_feeds (
  //GraphNode * self,
  //GraphNode * dest);

//void
//graph_node_add_depends (
  //GraphNode * self,
  //GraphNode * src);

void
graph_node_connect (
  GraphNode * from,
  GraphNode * to);

GraphNode *
graph_node_new (
  Graph * graph,
  GraphNodeType type,
  void *   data);

void
graph_node_free (
  GraphNode * node);

/**
 * @}
 */

#endif
