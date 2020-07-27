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

#include <stdlib.h>

#include "audio/engine.h"
#include "audio/fader.h"
#include "audio/graph.h"
#include "audio/graph_node.h"
#include "audio/master_track.h"
#include "audio/port.h"
#include "audio/router.h"
#include "audio/sample_processor.h"
#include "audio/track.h"
#include "audio/track_processor.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/mpmc_queue.h"
#include "utils/objects.h"

#include <gtk/gtk.h>

/**
 * Returns a human friendly name of the node.
 *
 * Must be free'd.
 */
char *
graph_node_get_name (
  GraphNode * node)
{
  char str[600];
  switch (node->type)
    {
    case ROUTE_NODE_TYPE_PLUGIN:
      return
        g_strdup_printf (
          "%s (Plugin)",
          node->pl->descr->name);
    case ROUTE_NODE_TYPE_PORT:
      port_get_full_designation (node->port, str);
      return g_strdup (str);
    case ROUTE_NODE_TYPE_FADER:
      {
        Track * track =
          fader_get_track (node->fader);
        return
          g_strdup_printf (
            "%s Fader", track->name);
      }
    case ROUTE_NODE_TYPE_TRACK:
      return
        g_strdup (
          node->track->name);
    case ROUTE_NODE_TYPE_PREFADER:
      {
        Track * track =
          fader_get_track (node->prefader);
        return
          g_strdup_printf (
            "%s Pre-Fader", track->name);
      }
    case ROUTE_NODE_TYPE_MONITOR_FADER:
      return
        g_strdup ("Monitor Fader");
    case ROUTE_NODE_TYPE_SAMPLE_PROCESSOR:
      return
        g_strdup ("Sample Processor");
    case ROUTE_NODE_TYPE_INITIAL_PROCESSOR:
      return
        g_strdup ("Initial Processor");
    }
  g_return_val_if_reached (NULL);
}

void *
graph_node_get_pointer (
  GraphNode * node)
{
  switch (node->type)
    {
    case ROUTE_NODE_TYPE_PORT:
      return node->port;
      break;
    case ROUTE_NODE_TYPE_PLUGIN:
      return node->pl;
      break;
    case ROUTE_NODE_TYPE_TRACK:
      return node->track;
      break;
    case ROUTE_NODE_TYPE_FADER:
    case ROUTE_NODE_TYPE_MONITOR_FADER:
      return node->fader;
      break;
    case ROUTE_NODE_TYPE_PREFADER:
      return node->prefader;
      break;
    case ROUTE_NODE_TYPE_SAMPLE_PROCESSOR:
      return node->sample_processor;
      break;
    case ROUTE_NODE_TYPE_INITIAL_PROCESSOR:
      return NULL;
      break;
    }
  g_return_val_if_reached (NULL);
}

void
graph_node_print (
  GraphNode * node)
{
  GraphNode * dest;
  if (!node)
    {
      g_message ("(null) node");
      return;
    }

  char * name = graph_node_get_name (node);
  char * str1 =
    g_strdup_printf (
      "node [(%d) %s] refcount: %d | terminal: %s | initial: %s | playback latency: %d",
      node->id,
      name,
      node->refcount,
      node->terminal ? "yes" : "no",
      node->initial ? "yes" : "no",
      node->playback_latency);
  g_free (name);
  char * str2;
  for (int j = 0; j < node->n_childnodes; j++)
    {
      dest = node->childnodes[j];
      name = graph_node_get_name (dest);
      str2 =
        g_strdup_printf ("%s (dest [(%d) %s])",
          str1, dest->id, name);
      g_free (str1);
      g_free (name);
      str1 = str2;
    }
  g_message ("%s", str1);
  g_free (str1);
}

static void
on_node_finish (
  GraphNode * self)
{
  int feeds = 0;

  /* notify downstream nodes that depend on this
   * node */
  for (int i = 0; i < self->n_childnodes; ++i)
    {
      /* set the largest playback latency of this
       * route to the child as well */
      self->childnodes[i]->route_playback_latency =
        self->playback_latency;
        /*MAX (*/
          /*self->playback_latency,*/
          /*self->childnodes[i]->*/
            /*route_playback_latency);*/
      graph_node_trigger (self->childnodes[i]);
      feeds = 1;
    }

  /* if there are no outgoing edges, this is a
   * terminal node */
  if (!feeds)
    {
      /* notify parent graph */
      graph_on_reached_terminal_node (self->graph);
    }
}

/**
 * Processes the GraphNode.
 */
void
graph_node_process (
  GraphNode *     node,
  const nframes_t nframes)
{
  g_return_if_fail (
    node && node->graph && node->graph->router);

  /*g_message (*/
    /*"processing %s", graph_node_get_name (node));*/
  bool noroll = false;

  nframes_t local_offset =
    node->graph->router->local_offset;

  /* figure out if we are doing a no-roll */
  if (node->route_playback_latency <
      AUDIO_ENGINE->remaining_latency_preroll)
    {
      /* no roll */
      if (node->type == ROUTE_NODE_TYPE_PLUGIN)
        {
      /*g_message (*/
        /*"-- not processing: %s "*/
        /*"route latency %ld",*/
        /*node->type == ROUTE_NODE_TYPE_PLUGIN ?*/
          /*node->pl->descr->name :*/
          /*node->port->identifier.label,*/
        /*node->route_playback_latency);*/
        }
      noroll = true;

      /* if no-roll, only process terminal nodes
       * to set their buffers to 0 */
      if (!node->terminal)
        {
          goto node_process_finish;
        }
    }

  /* global positions in frames (samples) */
  long g_start_frames;

  /* only compensate latency when rolling */
  if (TRANSPORT->play_state ==
        PLAYSTATE_ROLLING)
    {
      /* if the playhead is before the loop-end
       * point and the latency-compensated position
       * is after the loop-end point it means that
       * the loop was crossed, so compensate for
       * that.
       *
       * if the position is before loop-end and
       * position + frames is after loop end (there
       * is a loop inside the range), that should be
       * handled by the ports/processors instead */
      Position playhead_copy = *PLAYHEAD;
      transport_position_add_frames (
        TRANSPORT, &playhead_copy,
        node->route_playback_latency -
          AUDIO_ENGINE->remaining_latency_preroll);
      g_start_frames = playhead_copy.frames;
    }
  else
    {
      g_start_frames = PLAYHEAD->frames;
    }

  if (node->type == ROUTE_NODE_TYPE_PLUGIN)
    {
      plugin_process (
        node->pl, g_start_frames, local_offset,
        nframes);
    }
  else if (node->type == ROUTE_NODE_TYPE_FADER)
    {
      fader_process (
        node->fader, local_offset, nframes);
    }
  else if (node->type ==
             ROUTE_NODE_TYPE_MONITOR_FADER)
    {
      fader_process (
        node->fader, local_offset, nframes);
    }
  else if (node->type == ROUTE_NODE_TYPE_PREFADER)
    {
      fader_process (
        node->prefader, local_offset, nframes);
    }
  else if (node->type ==
           ROUTE_NODE_TYPE_SAMPLE_PROCESSOR)
    {
      sample_processor_process (
        node->sample_processor, local_offset,
        nframes);
    }
  else if (node->type ==
           ROUTE_NODE_TYPE_TRACK)
    {
      if (node->track->type != TRACK_TYPE_TEMPO &&
          node->track->type != TRACK_TYPE_MARKER)
        {
          track_processor_process (
            node->track->processor,
            g_start_frames, local_offset,
            nframes);
        }
    }
  else if (node->type == ROUTE_NODE_TYPE_PORT)
    {
      /* decide what to do based on what port it
       * is */
      Port * port = node->port;
      /*PortIdentifier * id = &port->id;*/

      /* if midi editor manual press */
      if (port == AUDIO_ENGINE->
            midi_editor_manual_press)
        {
          midi_events_dequeue (
            AUDIO_ENGINE->
              midi_editor_manual_press->
                midi_events);
        }

      /* if exporting and the port is not a
       * project port, ignore it */
      else if (
        engine_is_port_own (AUDIO_ENGINE, port) &&
        AUDIO_ENGINE->exporting)
        {
        }

      else
        {
          port_sum_signal_from_inputs (
            port, g_start_frames, local_offset,
            nframes, noroll);
        }
    }

node_process_finish:
  on_node_finish (node);
}

/**
 * Called by an upstream node when it has completed
 * processing.
 */
void
graph_node_trigger (
  GraphNode * self)
{
  /* check if we can run */
  if (g_atomic_int_dec_and_test (&self->refcount))
    {
      /* reset reference count for next cycle */
      g_atomic_int_set (
        &self->refcount,
        (unsigned int) self->init_refcount);

      /* all nodes that feed this node have
       * completed, so this node be processed
       * now. */
      g_atomic_int_inc (
        &self->graph->trigger_queue_size);
      /*g_message ("triggering node, pushing back");*/
      mpmc_queue_push_back_node (
        self->graph->trigger_queue, self);
    }
}

static void
add_feeds (
  GraphNode * self,
  GraphNode * dest)
{
  /* return if already added */
  for (int i = 0; i < self->n_childnodes; i++)
    {
      if (self->childnodes[i] == dest)
        {
          return;
        }
    }

  self->childnodes =
    (GraphNode **) realloc (
      self->childnodes,
      (size_t) (1 + self->n_childnodes) *
        sizeof (GraphNode *));
  self->childnodes[self->n_childnodes++] = dest;

  self->terminal = false;
}

static void
add_depends (
  GraphNode * self,
  GraphNode * src)
{
  ++self->init_refcount;
  self->refcount = self->init_refcount;

  /* add parent nodes */
  self->parentnodes =
    (GraphNode **) realloc (
      self->parentnodes,
      (size_t) (self->init_refcount) *
        sizeof (GraphNode *));

  self->parentnodes[self->init_refcount - 1] =
    src;

  self->initial = false;
}

/**
 * Returns the latency of only the given port,
 * without adding the previous/next latencies.
 *
 * It returns the plugin's latency if plugin,
 * otherwise 0.
 */
nframes_t
graph_node_get_single_playback_latency (
  GraphNode * node)
{
  switch (node->type)
    {
    case ROUTE_NODE_TYPE_PLUGIN:
      /* latency is already set at this point */
      return node->pl->latency;
    case ROUTE_NODE_TYPE_TRACK:
      return 0;
    default:
      break;
    }

  return 0;
}

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
  nframes_t   dest_latency)
{
  /*long max_latency = 0, parent_latency;*/

  /* set this node's latency */
  node->playback_latency =
    dest_latency +
    graph_node_get_single_playback_latency (node);

  /* set route playback latency if trigger node */
  if (node->initial)
    node->route_playback_latency =
      node->playback_latency;

  GraphNode * parent;
  for (int i = 0; i < node->init_refcount; i++)
    {
      parent = node->parentnodes[i];
#if 0
      g_message ("adding playback latency from node to parent:");
      print_node (node);
      print_node (parent);
#endif
      graph_node_set_playback_latency (
        parent, node->playback_latency);
    }
}

void
graph_node_connect (
  GraphNode * from,
  GraphNode * to)
{
  g_return_if_fail (from && to);
  if (array_contains (
        from->childnodes,
        from->n_childnodes,
        to))
    return;

  add_feeds (from, to);
  add_depends (to, from);

  g_warn_if_fail (!from->terminal && !to->initial);
}

GraphNode *
graph_node_new (
  Graph * graph,
  GraphNodeType type,
  void *   data)
{
  GraphNode * node =
    calloc (1, sizeof (GraphNode));
  node->id = (int) graph->num_setup_graph_nodes;
  node->graph = graph;
  node->type = type;
  switch (type)
    {
    case ROUTE_NODE_TYPE_PLUGIN:
      node->pl = (Plugin *) data;
      break;
    case ROUTE_NODE_TYPE_PORT:
      node->port = (Port *) data;
      break;
    case ROUTE_NODE_TYPE_FADER:
      node->fader = (Fader *) data;
      break;
    case ROUTE_NODE_TYPE_MONITOR_FADER:
      node->fader = (Fader *) data;
      break;
    case ROUTE_NODE_TYPE_PREFADER:
      node->prefader = (Fader *) data;
      break;
    case ROUTE_NODE_TYPE_SAMPLE_PROCESSOR:
      node->sample_processor =
        (SampleProcessor *) data;
      break;
    case ROUTE_NODE_TYPE_TRACK:
      node->track = (Track *) data;
      break;
    case ROUTE_NODE_TYPE_INITIAL_PROCESSOR:
      break;
    }

  return node;
}

void
graph_node_free (
  GraphNode * self)
{
  free (self->childnodes);
  free (self->parentnodes);

  object_zero_and_free (self);
}
