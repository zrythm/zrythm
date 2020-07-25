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


#include "audio/control_room.h"
#include "audio/engine.h"
#include "audio/fader.h"
#include "audio/graph.h"
#include "audio/graph_node.h"
#include "audio/graph_thread.h"
#include "audio/modulator.h"
#include "audio/port.h"
#include "audio/router.h"
#include "audio/sample_processor.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/audio.h"
#include "utils/env.h"
#include "utils/mpmc_queue.h"
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "utils/stoat.h"

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
  Graph *  self)
{
  g_return_if_fail (self->terminal_refcnt >= 0);

  if (g_atomic_int_dec_and_test (
        &self->terminal_refcnt))
    {
      /* all terminal nodes have completed,
       * we're done with this cycle. */
      g_warn_if_fail (
        g_atomic_int_get (
          &self->trigger_queue_size) == 0);

      /* Notify caller */
      zix_sem_post (&self->callback_done);

      /* Ensure that all background threads are
       * idle.
       * When freewheeling there may be an
       * immediate restart:
       * If there are more threads than CPU cores,
       * some worker- threads may only be "on
       * the way" to become idle. */
      while (g_atomic_int_get (
               &self->idle_thread_cnt) !=
             self->num_threads)
        sched_yield ();

      if (g_atomic_int_get (&self->terminate))
        return;

      /* now wait for the next cycle to begin */
      zix_sem_wait (&self->callback_start);

      if (g_atomic_int_get (&self->terminate))
        return;

      /* reset terminal reference count */
      g_atomic_int_set (
        &self->terminal_refcnt,
        (unsigned int) self->n_terminal_nodes);

      /* and start the initial nodes */
      for (size_t i = 0;
           i < self->n_init_triggers; ++i)
        {
          g_atomic_int_inc (
            &self->trigger_queue_size);
          mpmc_queue_push_back_node (
            self->trigger_queue,
            self->init_trigger_list[i]);
        }
      /* continue in worker-thread */
    }
}

/**
 * Checks for cycles in the graph.
 */
static bool
is_valid (
  Graph * self)
{
  /* Fill up an array of trigger nodes, and make
   * it large enough so that we can append more
   * nodes to it */
  GraphNode * triggers[self->num_setup_graph_nodes];
  int num_triggers = self->num_setup_init_triggers;
  for (int i = 0; i < num_triggers; i++)
    {
      triggers[i] =
        self->setup_init_trigger_list[i];
    }

  while (num_triggers > 0)
    {
      GraphNode * n = triggers[--num_triggers];

      for (int i = n->n_childnodes - 1;
           i >= 0; i--)
        {
          GraphNode * m = n->childnodes[i];
          n->n_childnodes--;
          m->init_refcount--;
          if (m->init_refcount == 0)
            {
              triggers[num_triggers++] = m;
            }
        }
    }

  for (size_t j = 0;
       j < self->num_setup_graph_nodes; j++)
    {
      GraphNode * n = self->setup_graph_nodes[j];
      if (n->n_childnodes > 0 ||
          n->init_refcount > 0)
        {
          return 0;
        }
    }

  return 1;
}

static void
clear_setup (
  Graph * self)
{
  for (size_t i = 0;
       i < self->num_setup_graph_nodes; i++)
    {
      object_free_w_func_and_null (
        graph_node_free,
        self->setup_graph_nodes[i]);
    }
  self->num_setup_graph_nodes = 0;
  self->num_setup_init_triggers = 0;
  self->num_setup_terminal_nodes = 0;
}

static void
graph_rechain (
  Graph * self)
{
  /*g_warn_if_fail (*/
    /*g_atomic_int_get (*/
      /*&self->terminal_refcnt) == 0);*/
  g_warn_if_fail (
    g_atomic_int_get (
      &self->trigger_queue_size) == 0);

  array_dynamic_swap (
    &self->graph_nodes, &self->n_graph_nodes,
    &self->setup_graph_nodes,
    &self->num_setup_graph_nodes);
  array_dynamic_swap (
    &self->init_trigger_list,
    &self->n_init_triggers,
    &self->setup_init_trigger_list,
    &self->num_setup_init_triggers);

  self->n_terminal_nodes =
    (int) self->num_setup_terminal_nodes;
  g_atomic_int_set (
    &self->terminal_refcnt,
    (guint) self->n_terminal_nodes);

  mpmc_queue_reserve (
    self->trigger_queue,
    (size_t) self->n_graph_nodes);

  clear_setup (self);
}

static void
add_plugin (
  Graph *  self,
  Plugin * pl)
{
  g_return_if_fail (pl && !pl->deleting);
  if (pl->num_in_ports == 0 &&
      pl->num_out_ports > 0)
    graph_create_node (
      self, ROUTE_NODE_TYPE_PLUGIN, pl);
  else if (pl->num_out_ports == 0 &&
           pl->num_in_ports > 0)
    graph_create_node (
      self, ROUTE_NODE_TYPE_PLUGIN, pl);
  else if (pl->num_out_ports == 0 &&
           pl->num_in_ports == 0)
    {
    }
  else
    graph_create_node (
      self, ROUTE_NODE_TYPE_PLUGIN, pl);
}

static void
connect_plugin (
  Graph *  self,
  Plugin * pl)
{
  g_return_if_fail (pl && !pl->deleting);
  GraphNode * pl_node =
    graph_find_node_from_plugin (self, pl);
  g_return_if_fail (pl_node);
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];
      g_return_if_fail (
        port_get_plugin (
          port, 1) != NULL);
      GraphNode * port_node =
        graph_find_node_from_port (
          self, port);
      g_return_if_fail (port_node);
      graph_node_connect (port_node, pl_node);
    }
  for (int i = 0; i < pl->num_out_ports; i++)
    {
      Port * port = pl->out_ports[i];
      g_return_if_fail (
        port_get_plugin (port, 1) != NULL);
      GraphNode * port_node =
        graph_find_node_from_port (
          self, port);
      g_warn_if_fail (port_node);
      graph_node_connect (pl_node, port_node);
    }
}

void
graph_print (
  Graph * self)
{
  g_message ("==printing graph");
  /*GraphNode * node;*/
  for (size_t i = 0;
       i < self->num_setup_graph_nodes; i++)
    {
      graph_node_print (self->setup_graph_nodes[i]);
    }
  g_message (
    "num trigger nodes %zu | "
    /*"num max trigger nodes %d | "*/
    "num terminal nodes %zu",
    self->num_setup_init_triggers,
    /*self->trigger_queue_size,*/
    self->num_setup_terminal_nodes);
  g_message ("==finish printing graph");
}

void
graph_destroy (
  Graph * self)
{
  g_message (
    "destroying graph %p (router g1 %p g2 %p)",
    self, self->router->graph,
    self->router->graph);
  self->destroying = 1;

  graph_terminate (self);

  /* wait for threads to finish */
  g_usleep (1000);

  graph_free (self);
}

/**
 * Add the port to the nodes.
 *
 * @param drop_if_unnecessary Drops the port
 *   if it doesn't connect anywhere.
 *
 * @return The graph node, if created.
 */
static GraphNode *
add_port (
  Graph *    self,
  Port *     port,
  const bool drop_if_unnecessary)
{
  PortOwnerType owner = port->id.owner_type;

  /* drop ports without sources and dests */
  if (
    drop_if_unnecessary &&
    port->num_dests == 0 &&
    port->num_srcs == 0 &&
    owner != PORT_OWNER_TYPE_PLUGIN &&
    owner != PORT_OWNER_TYPE_FADER &&
    owner != PORT_OWNER_TYPE_MONITOR_FADER &&
    owner != PORT_OWNER_TYPE_PREFADER &&
    owner != PORT_OWNER_TYPE_TRACK_PROCESSOR &&
    owner != PORT_OWNER_TYPE_TRACK &&
    owner != PORT_OWNER_TYPE_BACKEND &&
    owner != PORT_OWNER_TYPE_SAMPLE_PROCESSOR)
    {
      return NULL;
    }
  else
    {
      return
        graph_create_node (
          self, ROUTE_NODE_TYPE_PORT, port);
    }
}

/**
 * Connect the port as a node.
 */
static void
connect_port (
  Graph * self,
  Port * port)
{
  GraphNode * node =
    graph_find_node_from_port (self, port);
  GraphNode * node2;
  for (int j = 0; j < port->num_srcs; j++)
    {
      Port * src = port->srcs[j];
      node2 = graph_find_node_from_port (self, src);
      g_warn_if_fail (node);
      g_warn_if_fail (node2);
      graph_node_connect (node2, node);
    }
  for (int j = 0; j < port->num_dests; j++)
    {
      Port * dest = port->dests[j];
      node2 = graph_find_node_from_port (self, dest);
      g_warn_if_fail (node);
      g_warn_if_fail (node2);
      graph_node_connect (node, node2);
    }
}

/**
 * Returns the max playback latency of the trigger
 * nodes.
 */
nframes_t
graph_get_max_playback_latency (
  Graph * graph)
{
  nframes_t max = 0;
  GraphNode * node;
  for (size_t i = 0;
       i < graph->n_init_triggers; i++)
    {
      node =  graph->init_trigger_list[i];
      if (node->playback_latency > max)
        max = node->playback_latency;
    }

  return max;
}

void
graph_update_latencies (
  Graph * self,
  bool    use_setup_nodes)
{
  g_message ("updating graph latencies...");

  for (size_t i = 0;
       i <
         (use_setup_nodes ?
           self->num_setup_graph_nodes :
           (size_t) self->n_graph_nodes);
       i++)
    {
      GraphNode * node =
        (use_setup_nodes ?
          self->setup_graph_nodes[i] :
          self->graph_nodes[i]);
      if (node->terminal)
        {
          graph_node_set_playback_latency (node, 0);
        }
    }

  g_message (
    "Total latencies:\n"
    "Playback: %d\n"
    "Recording: %d\n",
    graph_get_max_playback_latency (self),
    0);
}

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
  const int rechain)
{
  int i, j;
  GraphNode * node, * node2;

  /* ========================
   * first add all the nodes
   * ======================== */

  /* add the sample processor */
  graph_create_node (
    self, ROUTE_NODE_TYPE_SAMPLE_PROCESSOR,
    SAMPLE_PROCESSOR);

  /* add the monitor fader */
  graph_create_node (
    self, ROUTE_NODE_TYPE_MONITOR_FADER,
    MONITOR_FADER);

  /* add the initial processor */
  graph_create_node (
    self, ROUTE_NODE_TYPE_INITIAL_PROCESSOR,
    NULL);

  /* add plugins */
  Track * tr;
  Plugin * pl;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      tr = TRACKLIST->tracks[i];
      g_warn_if_fail (tr);

      /* add the track */
      graph_create_node (
        self, ROUTE_NODE_TYPE_TRACK, tr);

      if (!tr->channel)
        continue;

      /* add the fader */
      graph_create_node (
        self, ROUTE_NODE_TYPE_FADER,
        tr->channel->fader);

      /* add the prefader */
      graph_create_node (
        self, ROUTE_NODE_TYPE_PREFADER,
        tr->channel->prefader);

      /* add plugins */
      for (j = 0; j < STRIP_SIZE * 2 + 1; j++)
        {
          if (j < STRIP_SIZE)
            pl = tr->channel->midi_fx[j];
          else if (j == STRIP_SIZE)
            pl = tr->channel->instrument;
          else
            pl =
              tr->channel->inserts[
                j - (STRIP_SIZE + 1)];

          if (!pl || pl->deleting)
            continue;

          add_plugin (self, pl);
          plugin_update_latency (pl);
        }
      for (j = 0; j < tr->num_modulators; j++)
        {
          pl = tr->modulators[j]->plugin;

          if (!pl || pl->deleting)
            continue;

          add_plugin (self, pl);
          plugin_update_latency (pl);
        }
    }

  /* add ports */
  int max_size = 20;
  Port ** ports =
    calloc ((size_t) max_size, sizeof (Port *));
  int num_ports;
  Port * port;
  port_get_all (
    &ports, &max_size, true, &num_ports);
  for (i = 0; i < num_ports; i++)
    {
      port = ports[i];
      g_warn_if_fail (port);
      if (port->deleting)
        continue;
      if (port->id.owner_type ==
            PORT_OWNER_TYPE_PLUGIN)
        {
          Plugin * port_pl =
            port_get_plugin (port, 1);
          if (port_pl->deleting)
            continue;
        }

      GraphNode * port_node =
        add_port (
          self, port, drop_unnecessary_ports);
      if (!port_node)
        {
          g_message (
            "%s: skipped port %s",
            __func__, port->id.label);
        }
    }

  /* ========================
   * now connect them
   * ======================== */

  /* connect the sample processor */
  node =
    graph_find_node_from_sample_processor (
      self, SAMPLE_PROCESSOR);
  port = SAMPLE_PROCESSOR->stereo_out->l;
  node2 =
    graph_find_node_from_port (self, port);
  graph_node_connect (node, node2);
  port = SAMPLE_PROCESSOR->stereo_out->r;
  node2 =
    graph_find_node_from_port (self, port);
  graph_node_connect (node, node2);

  /* connect the monitor fader */
  node =
    graph_find_node_from_monitor_fader (
      self, MONITOR_FADER);
  port = MONITOR_FADER->stereo_in->l;
  node2 =
    graph_find_node_from_port (self, port);
  graph_node_connect (node2, node);
  port = MONITOR_FADER->stereo_in->r;
  node2 =
    graph_find_node_from_port (self, port);
  graph_node_connect (node2, node);
  port = MONITOR_FADER->stereo_out->l;
  node2 =
    graph_find_node_from_port (self, port);
  graph_node_connect (node, node2);
  port = MONITOR_FADER->stereo_out->r;
  node2 =
    graph_find_node_from_port (self, port);
  graph_node_connect (node, node2);

  GraphNode * initial_processor_node =
    graph_find_initial_processor_node (self);

  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      tr = TRACKLIST->tracks[i];

      /* connect the track */
      node = graph_find_node_from_track (self, tr);
      g_warn_if_fail (node);
      if (tr->in_signal_type == TYPE_AUDIO)
        {
          port = tr->processor->stereo_in->l;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node2, node);
          graph_node_connect (
            initial_processor_node, node2);
          port = tr->processor->stereo_in->r;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node2, node);
          graph_node_connect (
            initial_processor_node, node2);
          port = tr->processor->stereo_out->l;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node, node2);
          port = tr->processor->stereo_out->r;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node, node2);
        }
      else if (tr->in_signal_type == TYPE_EVENT)
        {
          port = tr->processor->midi_in;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node2, node);
          graph_node_connect (
            initial_processor_node, node2);
          port = tr->processor->midi_out;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node, node2);
        }
      if (track_has_piano_roll (tr))
        {
          for (j = 0;
               j < NUM_MIDI_AUTOMATABLES * 16; j++)
            {
              port =
                tr->processor->midi_automatables[j];
              node2 =
                graph_find_node_from_port (
                  self, port);
              graph_node_connect (node2, node);
            }
        }
      if (tr->type == TRACK_TYPE_TEMPO)
        {
          port = tr->bpm_port;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node2, node);
          port = tr->time_sig_port;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node2, node);
          graph_node_connect (
            node, initial_processor_node);
        }

      if (!tr->channel)
        continue;

      Fader * fader;
      Fader * prefader;
      fader = tr->channel->fader;
      prefader = tr->channel->prefader;

      /* connect the fader */
      node =
        graph_find_node_from_fader (self, fader);
      g_warn_if_fail (node);
      if (fader->type == FADER_TYPE_AUDIO_CHANNEL)
        {
          /* connect ins */
          port = fader->stereo_in->l;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node2, node);
          port = fader->stereo_in->r;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node2, node);

          /* connect outs */
          port = fader->stereo_out->l;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node, node2);
          port = fader->stereo_out->r;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node, node2);
        }
      else if (fader->type ==
                 FADER_TYPE_MIDI_CHANNEL)
        {
          port = fader->midi_in;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node2, node);
          port = fader->midi_out;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node, node2);
        }
      port = fader->amp;
      node2 =
        graph_find_node_from_port (self, port);
      graph_node_connect (node2, node);
      port = fader->balance;
      node2 =
        graph_find_node_from_port (self, port);
      graph_node_connect (node2, node);
      port = fader->mute;
      node2 =
        graph_find_node_from_port (self, port);
      graph_node_connect (node2, node);

      /* connect the prefader */
      node =
        graph_find_node_from_prefader (
          self, prefader);
      g_warn_if_fail (node);
      if (prefader->type ==
            FADER_TYPE_AUDIO_CHANNEL)
        {
          port = prefader->stereo_in->l;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node2, node);
          port = prefader->stereo_in->r;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node2, node);
          port = prefader->stereo_out->l;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node, node2);
          port = prefader->stereo_out->r;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node, node2);
        }
      else if (prefader->type ==
                 FADER_TYPE_MIDI_CHANNEL)
        {
          port = prefader->midi_in;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node2, node);
          port = prefader->midi_out;
          node2 =
            graph_find_node_from_port (self, port);
          graph_node_connect (node, node2);
        }

      for (j = 0; j < STRIP_SIZE * 2 + 1; j++)
        {
          if (j < STRIP_SIZE)
            pl = tr->channel->midi_fx[j];
          else if (j == STRIP_SIZE)
            pl = tr->channel->instrument;
          else
            pl =
              tr->channel->inserts[
                j - (STRIP_SIZE + 1)];

          if (pl && !pl->deleting)
            connect_plugin (self, pl);
        }

      for (j = 0; j < tr->num_modulators; j++)
        {
          pl = tr->modulators[j]->plugin;

          if (pl && !pl->deleting)
            connect_plugin (self, pl);
        }
    }

  for (i = 0; i < num_ports; i++)
    {
      port = ports[i];
      if (port->deleting)
        continue;
      if (port->id.owner_type ==
            PORT_OWNER_TYPE_PLUGIN)
        {
          Plugin * port_pl =
            port_get_plugin (port, 1);
          if (port_pl->deleting)
            continue;
        }

      connect_port (
        self, port);
    }

  /* ========================
   * set initial and terminal nodes
   * ======================== */
  for (size_t ii = 0;
       ii < self->num_setup_graph_nodes; ii++)
    {
      node = self->setup_graph_nodes[ii];
      if (node->n_childnodes == 0)
        {
          /* terminal node */
          node->terminal = true;

          self->setup_terminal_nodes =
            (GraphNode **) realloc (
              self->setup_terminal_nodes,
              (size_t)
              (1 + self->num_setup_terminal_nodes) *
                sizeof (GraphNode *));
          self->setup_terminal_nodes[
            self->num_setup_terminal_nodes++] =
              node;
        }
      if (node->init_refcount == 0)
        {
          /* initial node */
          node->initial = true;

          self->setup_init_trigger_list =
            (GraphNode**)realloc (
              self->setup_init_trigger_list,
              (size_t)
              (1 + self->num_setup_init_triggers) *
                sizeof (GraphNode*));
          self->setup_init_trigger_list[
            self->num_setup_init_triggers++] = node;
        }
    }

  /* ========================
   * calculate latencies of each port and each
   * processor
   * ======================== */

  graph_update_latencies (self, true);

  /* ========================
   * set up caches to tracks, channels, plugins,
   * automation tracks, etc.
   *
   * this is because indices can be changed by the
   * GUI thread while the graph is running
   * TODO or maybe not needed since there is a lock
   * now
   * ======================== */

  /*graph_print (self);*/

  /* free ports */
  free (ports);

  if (rechain)
    graph_rechain (self);
}

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
  const Port * dest)
{
  g_return_val_if_fail (src && dest, 0);

  graph_setup (self, 0, 0);

  /* connect the src/dest if not NULL */
  /* this code is only for creating graphs to test
   * if the connection between src->dest is valid */
  GraphNode * node, * node2;
  node =
    graph_find_node_from_port (self, src);
  node2 =
    graph_find_node_from_port (self, dest);
  graph_node_connect (node, node2);

  bool valid = is_valid (self);

  return valid;
}

/**
 * Starts as many threads as there are cores.
 *
 * @return 1 if graph started, 0 otherwise.
 */
int
graph_start (
  Graph * graph)
{
  int num_cores =
    MIN (
      MAX_GRAPH_THREADS,
      audio_get_num_cores ());
  graph->num_threads =
    env_get_int (
      "ZRYTHM_DSP_THREADS",
      num_cores - 2);
  g_warn_if_fail (graph->num_threads >= 0);

  graph->num_threads =
    MAX (graph->num_threads, 0);

  /* create worker threads (num cores - 2 because
   * the main thread will become a worker too, so
   * in total N_CORES - 1 threads */
  for (int i = 0; i < graph->num_threads; i++)
    {
      graph->threads[i] =
        graph_thread_new (i, 0, graph);
      if (!graph->threads[i])
        {
          graph_terminate (graph);
          return 0;
        }
    }

  /* and the main thread */
  graph->main_thread =
    graph_thread_new (-1, 1, graph);
  if (!graph->main_thread)
    {
      graph_terminate (graph);
      return 0;
    }

  /* breathe */
  sched_yield ();

  return 1;
}

/**
 * Returns a new graph.
 */
Graph *
graph_new (
  Router * router)
{
  g_return_val_if_fail (router, NULL);

  Graph * self = object_new (Graph);

  self->router = router;
  self->trigger_queue = mpmc_queue_new ();
  self->init_trigger_list =
    object_new (GraphNode *);
  self->graph_nodes = object_new (GraphNode *);

  zix_sem_init (&self->callback_start, 0);
  zix_sem_init (&self->callback_done, 0);
  zix_sem_init (&self->trigger, 0);

  g_atomic_int_set (&self->terminal_refcnt, 0);
  g_atomic_int_set (&self->terminate, 0);
  g_atomic_int_set (&self->idle_thread_cnt, 0);
  g_atomic_int_set (&self->trigger_queue_size, 0);

  return self;
}

/**
 * Tell all threads to terminate.
 */
void
graph_terminate (
  Graph * self)
{
  /* Flag threads to terminate */
  g_atomic_int_set (&self->terminate, 1);

  /* wake-up sleeping threads */
  int tc =
    g_atomic_int_get (&self->idle_thread_cnt);
  g_warn_if_fail (tc == self->num_threads);
  for (int i = 0; i < tc; ++i)
    {
      zix_sem_post (&self->trigger);
    }

  /* and the main thread */
  zix_sem_post (&self->callback_start);

  /* join threads */
#ifdef HAVE_JACK
  if (AUDIO_ENGINE->audio_backend ==
        AUDIO_BACKEND_JACK)
    {
      for (int i = 0; i < self->num_threads; i++)
        {
#ifdef HAVE_JACK_CLIENT_STOP_THREAD
          jack_client_stop_thread (
            AUDIO_ENGINE->client,
            self->threads[i]->jthread);
#else
          pthread_join (
            self->threads[i]->jthread, NULL);
#endif // HAVE_JACK_CLIENT_STOP_THREAD
        }
#ifdef HAVE_JACK_CLIENT_STOP_THREAD
      jack_client_stop_thread (
        AUDIO_ENGINE->client,
        self->main_thread->jthread);
#else
      pthread_join (
        self->main_thread->jthread, NULL);
#endif // HAVE_JACK_CLIENT_STOP_THREAD
    }
  else
    {
#endif // HAVE_JACK
      for (int i = 0; i < self->num_threads; i++)
        {
          pthread_join (
            self->threads[i]->pthread, NULL);
        }
      pthread_join (
        self->main_thread->pthread, NULL);
#ifdef HAVE_JACK
    }
#endif
}

GraphNode *
graph_find_node_from_port (
  Graph * graph,
  const Port * port)
{
  GraphNode * node;
  for (size_t i = 0;
       i < graph->num_setup_graph_nodes; i++)
    {
      node = graph->setup_graph_nodes[i];
      if (node->type == ROUTE_NODE_TYPE_PORT &&
          node->port == port)
        return node;
    }
  return NULL;
}

GraphNode *
graph_find_node_from_plugin (
  Graph * graph,
  Plugin * pl)
{
  GraphNode * node;
  for (size_t i = 0;
       i < graph->num_setup_graph_nodes; i++)
    {
      node = graph->setup_graph_nodes[i];
      if (node->type == ROUTE_NODE_TYPE_PLUGIN &&
          node->pl == pl)
        return node;
    }
  return NULL;
}

GraphNode *
graph_find_node_from_track (
  Graph * graph,
  Track * track)
{
  GraphNode * node;
  for (size_t i = 0;
       i < graph->num_setup_graph_nodes; i++)
    {
      node = graph->setup_graph_nodes[i];
      if (node->type == ROUTE_NODE_TYPE_TRACK &&
          node->track == track)
        return node;
    }
  return NULL;
}

GraphNode *
graph_find_node_from_fader (
  Graph * graph,
  Fader * fader)
{
  GraphNode * node;
  for (size_t i = 0;
       i < graph->num_setup_graph_nodes; i++)
    {
      node = graph->setup_graph_nodes[i];
      if (node->type == ROUTE_NODE_TYPE_FADER &&
          node->fader == fader)
        return node;
    }
  return NULL;
}

GraphNode *
graph_find_node_from_prefader (
  Graph * graph,
  Fader * prefader)
{
  GraphNode * node;
  for (size_t i = 0;
       i < graph->num_setup_graph_nodes; i++)
    {
      node = graph->setup_graph_nodes[i];
      if (node->type == ROUTE_NODE_TYPE_PREFADER &&
          node->prefader == prefader)
        return node;
    }
  return NULL;
}

GraphNode *
graph_find_node_from_sample_processor (
  Graph * graph,
  SampleProcessor * sample_processor)
{
  GraphNode * node;
  for (size_t i = 0;
       i < graph->num_setup_graph_nodes; i++)
    {
      node = graph->setup_graph_nodes[i];
      if (node->type ==
            ROUTE_NODE_TYPE_SAMPLE_PROCESSOR &&
          node->sample_processor ==
            sample_processor)
        return node;
    }
  return NULL;
}

GraphNode *
graph_find_node_from_monitor_fader (
  Graph * graph,
  Fader * fader)
{
  GraphNode * node;
  for (size_t i = 0;
       i < graph->num_setup_graph_nodes; i++)
    {
      node = graph->setup_graph_nodes[i];
      if (node->type ==
            ROUTE_NODE_TYPE_MONITOR_FADER &&
          node->fader == fader)
        return node;
    }
  return NULL;
}

GraphNode *
graph_find_initial_processor_node (
  Graph * graph)
{
  GraphNode * node;
  for (size_t i = 0;
       i < graph->num_setup_graph_nodes; i++)
    {
      node = graph->setup_graph_nodes[i];
      if (node->type ==
            ROUTE_NODE_TYPE_INITIAL_PROCESSOR)
        return node;
    }
  return NULL;
}

/**
 * Creates a new node, adds it to the graph and
 * returns it.
 */
GraphNode *
graph_create_node (
  Graph * graph,
  GraphNodeType type,
  void * data)
{
  graph->setup_graph_nodes =
    (GraphNode **) realloc (
      graph->setup_graph_nodes,
      (size_t) (1 + graph->num_setup_graph_nodes) *
      sizeof (GraphNode *));

  GraphNode * node =
    graph_node_new (graph, type, data);
  graph->setup_graph_nodes[
    graph->num_setup_graph_nodes++] = node;

  return node;
}

/**
 * Frees the graph and its members.
 */
void
graph_free (
  Graph * self)
{
  g_message ("%s: freeing...", __func__);

  for (int i = 0; i < self->n_graph_nodes; ++i)
    {
      object_free_w_func_and_null (
        graph_node_free, self->graph_nodes[i]);
    }
  object_zero_and_free (self->graph_nodes);
  object_zero_and_free (self->init_trigger_list);
  object_zero_and_free (self->setup_graph_nodes);
  object_zero_and_free (
    self->setup_init_trigger_list);

  zix_sem_destroy (&self->callback_start);
  zix_sem_destroy (&self->callback_done);
  zix_sem_destroy (&self->trigger);
  object_set_to_zero (&self->callback_start);
  object_set_to_zero (&self->callback_done);
  object_set_to_zero (&self->trigger);

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
}
