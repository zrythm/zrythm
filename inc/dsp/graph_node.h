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
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

/**
 * \file
 *
 * Routing graph node.
 */

#ifndef __AUDIO_GRAPH_NODE_H__
#define __AUDIO_GRAPH_NODE_H__

#include "utils/types.h"

#include <gtk/gtk.h>

TYPEDEF_STRUCT (GraphNode);
TYPEDEF_STRUCT (Graph);
TYPEDEF_STRUCT (PassthroughProcessor);
TYPEDEF_STRUCT (Port);
TYPEDEF_STRUCT (Fader);
TYPEDEF_STRUCT (Track);
TYPEDEF_STRUCT (SampleProcessor);
TYPEDEF_STRUCT (Plugin);
TYPEDEF_STRUCT (HardwareProcessor);
TYPEDEF_STRUCT (ModulatorMacroProcessor);
TYPEDEF_STRUCT (EngineProcessTimeInfo);
TYPEDEF_STRUCT (ChannelSend);

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Graph nodes can be either ports or processors.
 *
 * Processors can be plugins, faders, etc.
 */
enum class GraphNodeType
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

  /** Hardware processor. */
  ROUTE_NODE_TYPE_HW_PROCESSOR,

  ROUTE_NODE_TYPE_MODULATOR_MACRO_PROCESOR,

  /** Channel send. */
  ROUTE_NODE_TYPE_CHANNEL_SEND,
};

/**
 * A node in the processing graph.
 */
typedef struct GraphNode
{
  int id;
  /** Ref back to the graph so we don't have to
   * pass it around. */
  Graph * graph;

  /** outgoing edges
   * downstream nodes to activate when this node
   * has completed processed
   */
  GraphNode ** childnodes;
  int          n_childnodes;

  /** Incoming node count. */
  gint refcount;

  /** Initial incoming node count. */
  gint init_refcount;

  /** Used when creating the graph so we can
   * traverse it backwards to set the latencies. */
  GraphNode ** parentnodes;

  /** Port, if not a plugin or fader. */
  Port * port;

  /** Plugin, if plugin. */
  Plugin * pl;

  /** Fader, if fader. */
  Fader * fader;

  Track * track;

  /** Pre-Fader, if prefader node. */
  Fader * prefader;

  /** Sample processor, if sample processor. */
  SampleProcessor * sample_processor;

  /** Hardware processor, if hardware processor. */
  HardwareProcessor * hw_processor;

  ModulatorMacroProcessor * modulator_macro_processor;

  ChannelSend * send;

  /** For debugging. */
  bool terminal;
  bool initial;

  /** The playback latency of the node, in
   * samples. */
  nframes_t playback_latency;

  /** The route's playback latency so far. */
  nframes_t route_playback_latency;

  GraphNodeType type;
} GraphNode;

/**
 * Returns a human friendly name of the node.
 *
 * Must be free'd.
 */
char *
graph_node_get_name (GraphNode * node);

void *
graph_node_get_pointer (GraphNode * self);

void
graph_node_print_to_str (GraphNode * node, char * buf, size_t buf_sz);

void
graph_node_print (GraphNode * node);

/**
 * Processes the GraphNode.
 */
HOT void
graph_node_process (GraphNode * node, EngineProcessTimeInfo time_nfo);

/**
 * Returns the latency of only the given port, without adding
 * the previous/next latencies.
 *
 * It returns the plugin's latency if plugin, otherwise 0.
 */
HOT nframes_t
graph_node_get_single_playback_latency (GraphNode * node);

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
graph_node_set_route_playback_latency (GraphNode * node, nframes_t dest_latency);

/**
 * Called by an upstream node when it has completed
 * processing.
 */
HOT void
graph_node_trigger (GraphNode * self);

void
graph_node_connect (GraphNode * from, GraphNode * to);

GraphNode *
graph_node_new (Graph * graph, GraphNodeType type, void * data);

void
graph_node_free (GraphNode * node);

/**
 * @}
 */

#endif
