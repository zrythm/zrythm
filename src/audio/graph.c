/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 */


#include "audio/control_room.h"
#include "audio/engine.h"
#include "audio/fader.h"
#include "audio/graph.h"
#include "audio/graph_node.h"
#include "audio/graph_thread.h"
#include "audio/hardware_processor.h"
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
#include "utils/flags.h"
#include "utils/mem.h"
#include "utils/mpmc_queue.h"
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "utils/stoat.h"
#include "utils/string.h"

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
  int num_setup_graph_nodes =
    (int)
    g_hash_table_size (self->setup_graph_nodes);
  GraphNode * triggers[num_setup_graph_nodes];
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

  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (
    &iter, self->setup_graph_nodes);
  while (g_hash_table_iter_next (
           &iter, &key, &value))
    {
      GraphNode * n = (GraphNode *) value;
      if (n->n_childnodes > 0 ||
          n->init_refcount > 0)
        {
          return false;
        }
    }

  return true;
}

static void
clear_setup (
  Graph * self)
{
  g_hash_table_remove_all (
    self->setup_graph_nodes);
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

  /* --- swap setup nodes with graph nodes --- */
  g_return_if_fail (
    self->graph_nodes && self->setup_graph_nodes);
  GHashTable * tmp =
    g_hash_table_new_full (
      g_direct_hash, g_direct_equal, NULL, NULL);
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (
    &iter, self->graph_nodes);
  while (g_hash_table_iter_next (
           &iter, &key, &value))
    {
      g_hash_table_insert (tmp, key, value);
    }
  g_hash_table_steal_all (self->graph_nodes);

  g_hash_table_iter_init (
    &iter, self->setup_graph_nodes);
  while (g_hash_table_iter_next (
           &iter, &key, &value))
    {
      g_hash_table_insert (
        self->graph_nodes, key, value);
    }
  g_hash_table_steal_all (self->setup_graph_nodes);

  g_hash_table_iter_init (&iter, tmp);
  while (g_hash_table_iter_next (
           &iter, &key, &value))
    {
      g_hash_table_insert (
        self->setup_graph_nodes, key, value);
    }

  /* --- end --- */

  array_dynamic_swap (
    &self->init_trigger_list,
    &self->n_init_triggers,
    &self->setup_init_trigger_list,
    &self->num_setup_init_triggers);
  array_dynamic_swap (
    &self->terminal_nodes,
    &self->n_terminal_nodes,
    &self->setup_terminal_nodes,
    &self->num_setup_terminal_nodes);

  /*self->n_terminal_nodes =*/
    /*(int) self->num_setup_terminal_nodes;*/
  g_atomic_int_set (
    &self->terminal_refcnt,
    (guint) self->n_terminal_nodes);

  mpmc_queue_reserve (
    self->trigger_queue,
    (size_t)
    g_hash_table_size (self->graph_nodes));

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
  Plugin * pl,
  bool     drop_unnecessary_ports)
{
  g_return_if_fail (pl && !pl->deleting);
  GraphNode * pl_node =
    graph_find_node_from_plugin (self, pl);
  g_return_if_fail (pl_node);
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];
      g_return_if_fail (
        port_get_plugin (port, 1) != NULL);
      GraphNode * port_node =
        graph_find_node_from_port (
          self, port);
      if (drop_unnecessary_ports && !port_node &&
          port->id.type == TYPE_CONTROL)
        {
          continue;
        }
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

  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (
    &iter, self->setup_graph_nodes);
  while (g_hash_table_iter_next (
           &iter, &key, &value))
    {
      GraphNode * n = (GraphNode *) value;
      graph_node_print (n);
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

  if (!g_atomic_int_get (&self->terminate) &&
      self->main_thread)
    {
      g_message ("terminating graph");
      graph_terminate (self);
    }
  else
    {
      g_message ("graph already terminated");
    }

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

  if (owner == PORT_OWNER_TYPE_PLUGIN)
    {
      port->plugin = port_get_plugin (port, true);
      g_return_val_if_fail (
        IS_PLUGIN_AND_NONNULL (port->plugin),
        NULL);
    }

  if (port->id.track_name_hash != 0)
    {
      port->track = port_get_track (port, true);
      g_return_val_if_fail (
        IS_TRACK_AND_NONNULL (port->track), NULL);
    }

  /* reset port sources/dests */
  GPtrArray * srcs = g_ptr_array_new ();
  int num_srcs =
    port_connections_manager_get_sources_or_dests (
      PORT_CONNECTIONS_MGR, srcs, &port->id,
      true);
  port->srcs_size = (size_t) num_srcs;
  port->srcs =
    object_realloc_n (
      port->srcs, 0, port->srcs_size, Port *);
  port->src_connections =
    object_realloc_n (
      port->src_connections, 0, port->srcs_size,
      PortConnection *);
#if 0
  if (num_srcs > 0)
    g_debug (
      "%d sources for %s",
      num_srcs, port->id.label);
#endif
  for (int i = 0; i < num_srcs; i++)
    {
      PortConnection * conn =
        (PortConnection *)
        g_ptr_array_index (srcs, i);

      port->srcs[i] =
        port_find_from_identifier (conn->src_id);
      g_return_val_if_fail (port->srcs[i], NULL);
      port->src_connections[i] = conn;
    }
  port->num_srcs = num_srcs;
  g_ptr_array_unref (srcs);

  GPtrArray * dests = g_ptr_array_new ();
  int num_dests =
    port_connections_manager_get_sources_or_dests (
      PORT_CONNECTIONS_MGR, dests, &port->id,
      false);
  port->dests_size = (size_t) num_dests;
  port->dests =
    object_realloc_n (
      port->dests, 0, port->dests_size, Port *);
  port->dest_connections =
    object_realloc_n (
      port->dest_connections, 0, port->dests_size,
      PortConnection *);
#if 0
  if (num_dests > 0)
    g_debug (
      "%d dests for %s",
      num_dests, port->id.label);
#endif
  for (int i = 0; i < num_dests; i++)
    {
      PortConnection * conn =
        (PortConnection *)
        g_ptr_array_index (dests, i);

      port->dests[i] =
        port_find_from_identifier (conn->dest_id);
      g_return_val_if_fail (port->dests[i], NULL);
      port->dest_connections[i] = conn;
    }
  port->num_dests = num_dests;
  g_ptr_array_unref (dests);

  if (drop_if_unnecessary)
    {
      /* skip unnecessary control ports */
      if (port->id.type == TYPE_CONTROL
          && port->id.flags & PORT_FLAG_AUTOMATABLE)
        {
          AutomationTrack * found_at = port->at;
          if (!found_at)
            {
#if 0
              found_at =
                automation_track_find_from_port (
                  port, port->track, true);
#endif
            }
          g_return_val_if_fail (found_at, NULL);
          if (found_at->num_regions == 0
              && port->num_srcs == 0)
            {
              return NULL;
            }
        }
    }

  /* drop ports without sources and dests */
  if (
    drop_if_unnecessary
    && port->num_dests == 0
    && port->num_srcs == 0
    && owner != PORT_OWNER_TYPE_PLUGIN
    && owner != PORT_OWNER_TYPE_FADER
    && owner != PORT_OWNER_TYPE_TRACK_PROCESSOR
    && owner != PORT_OWNER_TYPE_TRACK
    &&
    owner !=
      PORT_OWNER_TYPE_MODULATOR_MACRO_PROCESSOR
    && owner != PORT_OWNER_TYPE_CHANNEL
    && owner != PORT_OWNER_TYPE_CHANNEL_SEND
    && owner != PORT_OWNER_TYPE_AUDIO_ENGINE
    && owner != PORT_OWNER_TYPE_HW
    && owner != PORT_OWNER_TYPE_TRANSPORT
    && !(port->id.flags & PORT_FLAG_MANUAL_PRESS))
    {
      return NULL;
    }
  else
    {
      /* allocate buffers to be used during
       * DSP */
      port_allocate_bufs (port);
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
      node2 =
        graph_find_node_from_port (self, src);
      g_warn_if_fail (node);
      g_warn_if_fail (node2);
#if 0
      g_debug ("graph: %s => %s",
        src->id.label, port->id.label);
#endif
      graph_node_connect (node2, node);
    }
  for (int j = 0; j < port->num_dests; j++)
    {
      Port * dest = port->dests[j];
      node2 =
        graph_find_node_from_port (self, dest);
      g_warn_if_fail (node);
      g_warn_if_fail (node2);
#if 0
      g_debug ("graph: %s => %s",
        port->id.label, dest->id.label);
#endif
      graph_node_connect (node, node2);
    }
}

/**
 * Returns the max playback latency of the trigger
 * nodes.
 */
nframes_t
graph_get_max_route_playback_latency (
  Graph * graph,
  bool    use_setup_nodes)
{
  nframes_t max = 0;
  GraphNode * node;
  for (size_t i = 0;
       i <
         (use_setup_nodes ?
            graph->num_setup_init_triggers :
            graph->n_init_triggers);
       i++)
    {
      node =
        (use_setup_nodes ?
           graph->setup_init_trigger_list[i] :
           graph->init_trigger_list[i]);
      if (node->route_playback_latency > max)
        max = node->route_playback_latency;
    }

  return max;
}

void
graph_update_latencies (
  Graph * self,
  bool    use_setup_nodes)
{
  g_message ("updating graph latencies...");

  /* reset latencies */
  GHashTable * ht =
    use_setup_nodes
    ? self->setup_graph_nodes : self->graph_nodes;
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (&iter, ht);
  while (g_hash_table_iter_next (
           &iter, &key, &value))
    {
      GraphNode * n = (GraphNode *) value;
      n->playback_latency = 0;
      n->route_playback_latency = 0;
    }

  g_hash_table_iter_init (&iter, ht);
  while (g_hash_table_iter_next (
           &iter, &key, &value))
    {
      GraphNode * n = (GraphNode *) value;
      n->playback_latency =
        graph_node_get_single_playback_latency (
          n);
      if (n->playback_latency > 0)
        {
          graph_node_set_route_playback_latency (
            n, n->playback_latency);
        }
    }

  g_message (
    "Total latencies:\n"
    "Playback: %d\n"
    "Recording: %d\n",
    graph_get_max_route_playback_latency (
      self, use_setup_nodes),
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
    &self->initial_processor);

  /* add the hardware input processor */
  graph_create_node (
    self, ROUTE_NODE_TYPE_HW_PROCESSOR,
    HW_IN_PROCESSOR);

  /* add plugins */
  Track * tr;
  Plugin * pl;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      tr = tracklist_get_track (TRACKLIST, i);
      if (!tr)
        {
          g_warning ("track not found at %d", i);
          return;
        }

      /* add the track */
      graph_create_node (
        self, ROUTE_NODE_TYPE_TRACK, tr);

      for (int j = 0; j < tr->num_modulators; j++)
        {
          pl = tr->modulators[j];

          if (!pl || pl->deleting)
            continue;

          add_plugin (self, pl);
          plugin_update_latency (pl);
        }

      /* add the modulator macro processors */
      if (tr->type == TRACK_TYPE_MODULATOR)
        {
          for (int j = 0;
               j < tr->num_modulator_macros; j++)
            {
              graph_create_node (
                self,
                ROUTE_NODE_TYPE_MODULATOR_MACRO_PROCESOR,
                tr->modulator_macros[j]);
            }
        }

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
      for (int j = 0; j < STRIP_SIZE * 2 + 1; j++)
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

      /* add sends */
      if (tr->out_signal_type == TYPE_AUDIO
          || tr->out_signal_type == TYPE_EVENT)
        {
          for (int j = 0; j < STRIP_SIZE; j++)
            {
              ChannelSend * send =
                tr->channel->sends[j];

              /* add send even if empty so that
               * graph renders properly */
#if 0
              if (channel_send_is_empty (send))
                continue;
#endif

              graph_create_node (
                self, ROUTE_NODE_TYPE_CHANNEL_SEND,
                send);
            }
        }
    }

  object_free_w_func_and_null (
    g_ptr_array_unref, self->external_out_ports);
  self->external_out_ports = g_ptr_array_new ();

  /* add ports */
  Port * port;
  GPtrArray * ports = g_ptr_array_new ();
  port_get_all (ports);
  for (size_t i = 0; i < ports->len; i++)
    {
      port = g_ptr_array_index (ports, i);
      g_return_if_fail (IS_PORT_AND_NONNULL (port));
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

#ifdef HAVE_JACK
      if (port->id.flow == FLOW_OUTPUT
          &&
          port->internal_type == INTERNAL_JACK_PORT)
        {
          g_ptr_array_add (
            self->external_out_ports, port);
        }
#endif

      GraphNode * port_node =
        add_port (
          self, port, drop_unnecessary_ports);
      (void) port_node;
#if 0
      if (port_node)
        {
          /*g_debug (*/
            /*"added port %s", port->id.label);*/
        }
      else
        {
/*#if 0*/
          char label[5000];
          port_get_full_designation (port, label);
          g_message (
            "%s: skipped port %s",
            __func__, label);
/*#endif*/
        }
#endif
    }

  /* ========================
   * now connect them
   * ======================== */

  /* connect the sample processor */
  node =
    graph_find_node_from_sample_processor (
      self, SAMPLE_PROCESSOR);
  port =
    SAMPLE_PROCESSOR->fader->stereo_out->l;
  node2 =
    graph_find_node_from_port (self, port);
  graph_node_connect (node, node2);
  port = SAMPLE_PROCESSOR->fader->stereo_out->r;
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

  /* connect the HW input processor */
  GraphNode * hw_processor_node =
    graph_find_hw_processor_node (
      self, HW_IN_PROCESSOR);
  for (int i = 0;
       i < HW_IN_PROCESSOR->num_audio_ports; i++)
    {
      port = HW_IN_PROCESSOR->audio_ports[i];
      node2 =
        graph_find_node_from_port (self, port);
      g_warn_if_fail (node2);
      graph_node_connect (hw_processor_node, node2);
    }
  for (int i = 0;
       i < HW_IN_PROCESSOR->num_midi_ports; i++)
    {
      port = HW_IN_PROCESSOR->midi_ports[i];
      node2 =
        graph_find_node_from_port (self, port);
      graph_node_connect (hw_processor_node, node2);
    }

  /* connect MIDI editor manual press */
  node2 =
    graph_find_node_from_port (
      self, AUDIO_ENGINE->midi_editor_manual_press);
  graph_node_connect (
    node2, initial_processor_node);

  /* connect the transport ports */
  node2 =
    graph_find_node_from_port (
      self, TRANSPORT->roll);
  graph_node_connect (
    node2, initial_processor_node);
  node2 =
    graph_find_node_from_port (
      self, TRANSPORT->stop);
  graph_node_connect (
    node2, initial_processor_node);
  node2 =
    graph_find_node_from_port (
      self, TRANSPORT->backward);
  graph_node_connect (
    node2, initial_processor_node);
  node2 =
    graph_find_node_from_port (
      self, TRANSPORT->forward);
  graph_node_connect (
    node2, initial_processor_node);
  node2 =
    graph_find_node_from_port (
      self, TRANSPORT->loop_toggle);
  graph_node_connect (
    node2, initial_processor_node);
  node2 =
    graph_find_node_from_port (
      self, TRANSPORT->rec_toggle);
  graph_node_connect (
    node2, initial_processor_node);

  /* connect tracks */
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      tr = TRACKLIST->tracks[i];

      /* connect the track */
      node =
        graph_find_node_from_track (self, tr, true);
      if (tr->in_signal_type == TYPE_AUDIO)
        {
          if (tr->type == TRACK_TYPE_AUDIO)
            {
              port = tr->processor->mono;
              node2 =
                graph_find_node_from_port (
                  self, port);
              graph_node_connect (node2, node);
              graph_node_connect (
                initial_processor_node, node2);
              port = tr->processor->input_gain;
              node2 =
                graph_find_node_from_port (
                  self, port);
              graph_node_connect (node2, node);
              graph_node_connect (
                initial_processor_node, node2);
            }
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
          if (track_type_has_piano_roll (tr->type) ||
              tr->type == TRACK_TYPE_CHORD)
            {
              /* connect piano roll */
              port = tr->processor->piano_roll;
              node2 =
                graph_find_node_from_port (
                  self, port);
              graph_node_connect (node2, node);
            }
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
      if (track_type_has_piano_roll (tr->type))
        {
          for (int j = 0; j < 16; j++)
            {
              for (int k = 0; k < 128; k++)
                {
                  port =
                    tr->processor->midi_cc[j * 128 + k];
                  node2 =
                    graph_find_node_from_port (
                      self, port);
                  if (node2)
                    {
                      graph_node_connect (
                        node2, node);
                    }
                }

              port =
                tr->processor->pitch_bend[j];
              node2 =
                graph_find_node_from_port (
                  self, port);
              if (node2 || !drop_unnecessary_ports)
                {
                  graph_node_connect (node2, node);
                }

              port =
                tr->processor->poly_key_pressure[j];
              node2 =
                graph_find_node_from_port (
                  self, port);
              if (node2 || !drop_unnecessary_ports)
                {
                  graph_node_connect (node2, node);
                }

              port =
                tr->processor->channel_pressure[j];
              node2 =
                graph_find_node_from_port (
                  self, port);
              if (node2 || !drop_unnecessary_ports)
                {
                  graph_node_connect (node2, node);
                }
            }
        }
      if (tr->type == TRACK_TYPE_TEMPO)
        {
          self->bpm_node = NULL;
          self->beats_per_bar_node = NULL;
          self->beat_unit_node = NULL;

          port = tr->bpm_port;
          node2 =
            graph_find_node_from_port (self, port);
          if (node2 || !drop_unnecessary_ports)
            {
              self->bpm_node = node2;
              graph_node_connect (node2, node);
            }
          port = tr->beats_per_bar_port;
          node2 =
            graph_find_node_from_port (self, port);
          if (node2 || !drop_unnecessary_ports)
            {
              self->beats_per_bar_node = node2;
              graph_node_connect (node2, node);
            }
          port = tr->beat_unit_port;
          node2 =
            graph_find_node_from_port (self, port);
          if (node2 || !drop_unnecessary_ports)
            {
              self->beat_unit_node = node2;
              graph_node_connect (node2, node);
            }
          graph_node_connect (
            node, initial_processor_node);
        }
      if (tr->type == TRACK_TYPE_MODULATOR)
        {
          graph_node_connect (
            initial_processor_node, node);

          for (int j = 0; j < tr->num_modulators;
               j++)
            {
              pl = tr->modulators[j];

              if (pl && !pl->deleting)
                {
                  connect_plugin (
                    self, pl,
                    drop_unnecessary_ports);
                  for (int k = 0;
                       k < pl->num_in_ports; k++)
                    {
                      Port * pl_port =
                        pl->in_ports[k];
                      g_return_if_fail (
                        port_get_plugin (
                          pl_port, 1) != NULL);
                      GraphNode * port_node =
                        graph_find_node_from_port (
                          self, pl_port);
                      if (drop_unnecessary_ports &&
                          !port_node &&
                          port->id.type ==
                            TYPE_CONTROL)
                        {
                          continue;
                        }
                      g_return_if_fail (port_node);
                      graph_node_connect (
                        node, port_node);
                    }
                }
            }

          /* connect the modulator macro
           * processors */
          for (int j = 0;
               j < tr->num_modulator_macros; j++)
            {
              ModulatorMacroProcessor * mmp =
                  tr->modulator_macros[j];
              GraphNode * mmp_node =
                graph_find_node_from_modulator_macro_processor (
                  self, mmp);

              port = mmp->cv_in;
              node2 =
                graph_find_node_from_port (
                  self, port);
              graph_node_connect (
                node2, mmp_node);
              port = mmp->macro;
              node2 =
                graph_find_node_from_port (
                  self, port);
              if (node2 || !drop_unnecessary_ports)
                {
                  graph_node_connect (
                    node2, mmp_node);
                }
              port = mmp->cv_out;
              node2 =
                graph_find_node_from_port (
                  self, port);
              graph_node_connect (
                mmp_node, node2);
            }
        }

      if (!track_type_has_channel (tr->type))
        continue;

      Channel * ch = tr->channel;

      Fader * prefader = ch->prefader;
      Fader * fader = ch->fader;

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
      if (node2 || !drop_unnecessary_ports)
        {
          graph_node_connect (node2, node);
        }
      port = fader->balance;
      node2 =
        graph_find_node_from_port (self, port);
      if (node2 || !drop_unnecessary_ports)
        {
          graph_node_connect (node2, node);
        }
      port = fader->mute;
      node2 =
        graph_find_node_from_port (self, port);
      if (node2 || !drop_unnecessary_ports)
        {
          graph_node_connect (node2, node);
        }

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

      for (int j = 0; j < STRIP_SIZE * 2 + 1; j++)
        {
          if (j < STRIP_SIZE)
            pl = ch->midi_fx[j];
          else if (j == STRIP_SIZE)
            pl = ch->instrument;
          else
            pl =
              ch->inserts[j - (STRIP_SIZE + 1)];

          if (pl && !pl->deleting)
            {
              connect_plugin (
                self, pl, drop_unnecessary_ports);
            }
        }

      for (int j = 0; j < STRIP_SIZE; j++)
        {
          ChannelSend * send = ch->sends[j];

          /* do not skip empty because then port
           * connection validation does not
           * detect invalid connections properly */
          /*if (channel_send_is_empty (send))*/
            /*continue;*/

          node =
            graph_find_node_from_channel_send (
              self, send);

          node2 =
            graph_find_node_from_port (
              self, send->amount);
          if (node2)
            graph_node_connect (node2, node);
          node2 =
            graph_find_node_from_port (
              self, send->enabled);
          if (node2)
            graph_node_connect (node2, node);

          if (tr->out_signal_type == TYPE_EVENT)
            {
              node2 =
                graph_find_node_from_port (
                  self, send->midi_in);
              graph_node_connect (node2, node);
              node2 =
                graph_find_node_from_port (
                  self, send->midi_out);
              graph_node_connect (node, node2);
            }
          else if (tr->out_signal_type == TYPE_AUDIO)
            {
              node2 =
                graph_find_node_from_port (
                  self, send->stereo_in->l);
              graph_node_connect (node2, node);
              node2 =
                graph_find_node_from_port (
                  self, send->stereo_in->r);
              graph_node_connect (node2, node);
              node2 =
                graph_find_node_from_port (
                  self, send->stereo_out->l);
              graph_node_connect (node, node2);
              node2 =
                graph_find_node_from_port (
                  self, send->stereo_out->r);
              graph_node_connect (node, node2);
            }
        }
    }

  for (size_t i = 0; i < ports->len; i++)
    {
      port = g_ptr_array_index (ports, i);
      if (G_UNLIKELY (port->deleting))
        continue;
      if (port->id.owner_type ==
            PORT_OWNER_TYPE_PLUGIN)
        {
          Plugin * port_pl =
            port_get_plugin (port, 1);
          if (G_UNLIKELY (port_pl->deleting))
            continue;
        }

      connect_port (self, port);
    }

  /* ========================
   * set initial and terminal nodes
   * ======================== */
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init (
    &iter, self->setup_graph_nodes);
  while (g_hash_table_iter_next (
           &iter, &key, &value))
    {
      node = (GraphNode *) value;
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

  clip_editor_set_caches (CLIP_EDITOR);
  tracklist_set_caches (TRACKLIST);

  /*graph_print (self);*/

  g_ptr_array_unref (ports);

  if (rechain)
    graph_rechain (self);
}

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
graph_validate_with_connection (
  Graph *      self,
  const Port * src,
  const Port * dest)
{
  g_return_val_if_fail (src && dest, 0);

  EngineState state;
  engine_wait_for_pause (
    AUDIO_ENGINE, &state, F_NO_FORCE);

  g_message (
    "validating for %s to %s",
    src->id.label, dest->id.label);

  graph_setup (
    self, Z_F_NO_DROP_UNNECESSARY, Z_F_NO_RECHAIN);

  /* connect the src/dest if not NULL */
  /* this code is only for creating graphs to test
   * if the connection between src->dest is valid */
  GraphNode * node, * node2;
  node =
    graph_find_node_from_port (self, src);
  g_return_val_if_fail (node, NULL);
  node2 =
    graph_find_node_from_port (self, dest);
  g_return_val_if_fail (node2, NULL);
  graph_node_connect (node, node2);
  g_return_val_if_fail (!node->terminal, false);
  g_return_val_if_fail (!node2->initial, false);

  bool valid = is_valid (self);

  g_message ("valid %d", valid);

  engine_resume (AUDIO_ENGINE, &state);

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
          g_error ("thread new failed");
          graph_terminate (graph);
          return 0;
        }
    }

  /* and the main thread */
  graph->main_thread =
    graph_thread_new (-1, 1, graph);
  if (!graph->main_thread)
    {
      g_error ("thread new failed");
      graph_terminate (graph);
      return 0;
    }

  /* breathe */
  sched_yield ();

  /* wait for all threads to go idle */
  while (
    g_atomic_int_get (&graph->idle_thread_cnt) !=
      graph->num_threads)
    {
      /* wait for all threads to go idle */
      g_message (
        "waiting for threads to go idle after "
        "creation...");
      g_usleep (10000);
    }

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
  self->terminal_nodes =
    object_new (GraphNode *);
  self->graph_nodes =
    g_hash_table_new_full (
      g_direct_hash, g_direct_equal, NULL,
      (GDestroyNotify) graph_node_free);
  self->setup_graph_nodes =
    g_hash_table_new_full (
      g_direct_hash, g_direct_equal, NULL,
      (GDestroyNotify) graph_node_free);

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
  g_message ("terminating graph...");

  /* Flag threads to terminate */
  g_atomic_int_set (&self->terminate, 1);

  while (
    g_atomic_int_get (&self->idle_thread_cnt) !=
      self->num_threads)
    {
      /* wait for all threads to go idle */
      g_message (
        "waiting for threads to go idle...");
      g_usleep (10000);
    }

  /* wake-up sleeping threads */
  int tc =
    g_atomic_int_get (&self->idle_thread_cnt);
  if (tc != self->num_threads)
    {
      g_warning (
        "expected %d idle threads, found %d",
        self->num_threads, tc);
    }
  for (int i = 0; i < tc; ++i)
    {
      zix_sem_post (&self->trigger);
    }

  /* and the main thread */
  zix_sem_post (&self->callback_start);

  /* join threads */
  for (int i = 0; i < self->num_threads; i++)
    {
      g_return_if_fail (self->threads[i]);
      void * status;
      pthread_join (
        self->threads[i]->pthread, &status);
      self->threads[i] = NULL;
    }
  g_return_if_fail (self->main_thread);
  void * status;
  pthread_join (
    self->main_thread->pthread, &status);
  self->main_thread = NULL;

  g_message ("graph terminated");
}

GraphNode *
graph_find_node_from_port (
  const Graph * self,
  const Port *  port)
{
  GraphNode * node =
    (GraphNode *)
    g_hash_table_lookup (
      self->setup_graph_nodes, port);
  if (node && node->type == ROUTE_NODE_TYPE_PORT)
    {
      g_return_val_if_fail (
        node->port == port, NULL);
      return node;
    }

  return NULL;
}

GraphNode *
graph_find_node_from_plugin (
  const Graph *  self,
  const Plugin * pl)
{
  GraphNode * node =
    (GraphNode *)
    g_hash_table_lookup (
      self->setup_graph_nodes, pl);
  if (node && node->type == ROUTE_NODE_TYPE_PLUGIN)
    return node;
  else
    return NULL;
}

GraphNode *
graph_find_node_from_track (
  const Graph * self,
  const Track * track,
  bool          use_setup_nodes)
{
  GHashTable * nodes =
    (use_setup_nodes ?
       self->setup_graph_nodes :
       self->graph_nodes);
  GraphNode * node =
    (GraphNode *)
    g_hash_table_lookup (nodes, track);
  if (node && node->type == ROUTE_NODE_TYPE_TRACK)
    return node;
  else
    return NULL;
}

GraphNode *
graph_find_node_from_fader (
  const Graph * self,
  const Fader * fader)
{
  GraphNode * node =
    (GraphNode *)
    g_hash_table_lookup (
      self->setup_graph_nodes, fader);
  if (node && node->type == ROUTE_NODE_TYPE_FADER)
    return node;
  else
    return NULL;
}

GraphNode *
graph_find_node_from_prefader (
  const Graph * self,
  const Fader * prefader)
{
  GraphNode * node =
    (GraphNode *)
    g_hash_table_lookup (
      self->setup_graph_nodes, prefader);
  if (node
      && node->type == ROUTE_NODE_TYPE_PREFADER)
    return node;
  else
    return NULL;
}

GraphNode *
graph_find_node_from_sample_processor (
  const Graph *           self,
  const SampleProcessor * sample_processor)
{
  GraphNode * node =
    (GraphNode *)
    g_hash_table_lookup (
      self->setup_graph_nodes, sample_processor);
  if (node
      &&
      node->type ==
        ROUTE_NODE_TYPE_SAMPLE_PROCESSOR)
    return node;
  else
    return NULL;
}

GraphNode *
graph_find_node_from_monitor_fader (
  const Graph * self,
  const Fader * fader)
{
  GraphNode * node =
    (GraphNode *)
    g_hash_table_lookup (
      self->setup_graph_nodes, fader);
  if (node
      &&
      node->type == ROUTE_NODE_TYPE_MONITOR_FADER)
    return node;
  else
    return NULL;
}

GraphNode *
graph_find_node_from_channel_send (
  const Graph *       self,
  const ChannelSend * send)
{
  GraphNode * node =
    (GraphNode *)
    g_hash_table_lookup (
      self->setup_graph_nodes, send);
  if (node
      &&
      node->type == ROUTE_NODE_TYPE_CHANNEL_SEND)
    return node;
  else
    return NULL;
}

GraphNode *
graph_find_initial_processor_node (
  const Graph * self)
{
  GraphNode * node =
    (GraphNode *)
    g_hash_table_lookup (
      self->setup_graph_nodes,
      &self->initial_processor);
  if (node
      &&
      node->type ==
        ROUTE_NODE_TYPE_INITIAL_PROCESSOR)
    return node;
  else
    return NULL;
}

GraphNode *
graph_find_hw_processor_node (
  const Graph *             self,
  const HardwareProcessor * processor)
{
  GraphNode * node =
    (GraphNode *)
    g_hash_table_lookup (
      self->setup_graph_nodes, processor);
  if (node
      &&
      node->type == ROUTE_NODE_TYPE_HW_PROCESSOR)
    return node;
  else
    return NULL;
}

GraphNode *
graph_find_node_from_modulator_macro_processor (
  const Graph *                   self,
  const ModulatorMacroProcessor * processor)
{
  GraphNode * node =
    (GraphNode *)
    g_hash_table_lookup (
      self->setup_graph_nodes, processor);
  if (node
      &&
      node->type ==
        ROUTE_NODE_TYPE_MODULATOR_MACRO_PROCESOR)
    return node;
  else
    return NULL;
}

/**
 * Creates a new node, adds it to the graph and
 * returns it.
 */
GraphNode *
graph_create_node (
  Graph *       self,
  GraphNodeType type,
  void *        data)
{
  GraphNode * node =
    graph_node_new (self, type, data);
  g_hash_table_insert (
    self->setup_graph_nodes, data, node);

  return node;
}

/**
 * Frees the graph and its members.
 */
void
graph_free (
  Graph * self)
{
  g_debug ("%s: freeing...", __func__);

  object_free_w_func_and_null (
    g_hash_table_unref, self->graph_nodes);
  object_zero_and_free (self->init_trigger_list);
  object_free_w_func_and_null (
    g_hash_table_unref, self->setup_graph_nodes);
  object_zero_and_free (
    self->setup_init_trigger_list);
  object_zero_and_free (
    self->terminal_nodes);

  object_free_w_func_and_null (
    g_ptr_array_unref, self->external_out_ports);

  zix_sem_destroy (&self->callback_start);
  zix_sem_destroy (&self->callback_done);
  zix_sem_destroy (&self->trigger);
  object_set_to_zero (&self->callback_start);
  object_set_to_zero (&self->callback_done);
  object_set_to_zero (&self->trigger);

  object_zero_and_free (self);

  g_debug ("%s: done", __func__);
}
