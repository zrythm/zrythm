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

#include "config.h"

#include "audio/audio_track.h"
#include "audio/engine.h"
#ifdef HAVE_JACK
#include "audio/engine_jack.h"
#endif
#include "audio/instrument_track.h"
#include "audio/pan.h"
#include "audio/port.h"
#include "audio/routing.h"
#include "audio/track.h"
#include "project.h"
#include "utils/arrays.h"

#ifdef HAVE_JACK
#include <jack/thread.h>
#endif

#define add_initial_trigger_node(router,node) \
  array_append ((router)->initial_trigger_nodes, \
                (router)->num_initial_trigger_nodes, \
                node);
#define add_trigger_node(router,node) \
  array_append ((router)->trigger_nodes, \
                (router)->num_trigger_nodes, \
                node);

static void
print_node (RouteNode * node, int i)
{
  RouteNode * src, * dest;
  if (!node)
    {
      g_message ("(null) node");
      return;
    }

  char * str1 =
    g_strdup_printf (
      "node %d [(%d) %s] refcount: %d",
      i,
      node->id,
      node->type == ROUTE_NODE_TYPE_PLUGIN ?
        node->pl->descr->name :
        node->port->label,
      node->refcount);
  char * str2;
  for (int j = 0; j < node->refcount; j++)
    {
      src = node->srcs[j];
      str2 =
        g_strdup_printf ("%s (src [(%d) %s])",
          str1,
          src->id,
          src->type ==
            ROUTE_NODE_TYPE_PLUGIN ?
              src->pl->descr->name :
              src->port->label);
      g_free (str1);
      str1 = str2;
    }
  for (int j = 0; j < node->num_dests; j++)
    {
      dest = node->dests[j];
      str2 =
        g_strdup_printf ("%s (dest [(%d) %s])",
          str1,
          dest->id,
          dest->type ==
            ROUTE_NODE_TYPE_PLUGIN ?
              dest->pl->descr->name :
              dest->port->label);
      g_free (str1);
      str1 = str2;
    }
  g_message ("%s", str1);
  g_free (str1);
}

/*static RouteNode **/
/*find_matching_node (*/
  /*Router * router,*/
  /*RouteNode * in)*/
/*{*/
  /*RouteNode * node;*/
  /*for (int i = 0; i < router->registry->len; i++)*/
    /*{*/
      /*node =*/
        /*g_array_index (router->registry,*/
                       /*RouteNode *, i);*/
      /*if ((node->type == ROUTE_NODE_TYPE_PORT &&*/
           /*node->port == in->port) ||*/
          /*(node->type == ROUTE_NODE_TYPE_PLUGIN &&*/
           /*node->pl == in->pl))*/
        /*return node;*/
    /*}*/
  /*g_warn_if_reached ();*/
  /*return NULL;*/
/*}*/

static void
nodes_disconnect (
  Router * router,
  RouteNode * src,
  RouteNode * dest)
{
  g_warn_if_fail (src != NULL);
  g_warn_if_fail (dest != NULL);

  /*[>int dests->len = src->dests->len;<]*/
  /*[>int srcs->len = dest->srcs->len;<]*/
  /*[>g_message ("num srcs before: %d", srcs->len);<]*/
  /*[>zix_sem_wait (&router->route_operation_sem);<]*/

  /*[> disconnect dest from src <]*/
  if (array_contains (src->dests,
                      src->num_dests,
                      dest))
    {
      array_delete (src->dests,
                    src->num_dests, dest);
    }
  /*[> disconnect src from dest <]*/
  /*[>for (int i = 0; i < dest->srcs->len; i++)<]*/
    /*[>{<]*/
      /*[>g_message ("src %d: %p", i, dest->srcs[i]);<]*/
      /*[>print_node (dest->srcs[i], -300);<]*/
      /*[>g_message ("given src: %p", src);<]*/
      /*[>print_node (src, -300);<]*/
    /*[>}<]*/
    if (array_contains (dest->srcs,
                      dest->init_refcount,
                      src))
    {
       /*FIXME srcs->len should be atomic */
      array_delete (dest->srcs, dest->init_refcount, src);
    }
      /*g_array_remove_index_fast (*/
        /*dest->srcs, dest_idx);*/

      /*[> add trigger node <]*/
      /*if (dest->srcs->len == 0)*/
        /*array_append (router->trigger_nodes,*/
                      /*router->num_trigger_nodes,*/
                      /*dest);*/
    /*[>}<]*/

  /*[>zix_sem_post (&router->route_operation_sem);<]*/
  /*[>g_message ("num srcs after: %d", dest->srcs->len);<]*/
}

/**
 * Gets next unclaimed trigger node in the array and
 * marks it as claimed.
 */
static inline RouteNode *
get_next_trigger_node (
  Router * router,
  int      pop)
{
  if (router->num_trigger_nodes == 0)
    {
      /*g_message ("no trigger nodes");*/
      return NULL;
    }

  if (pop)
    {
      g_atomic_int_dec_and_test (
        &router->num_trigger_nodes);
      return router->trigger_nodes[
          router->num_trigger_nodes];
    }
  else
    {
      return router->trigger_nodes[
        router->num_trigger_nodes - 1];
    }
}

/**
 * Processes the RouteNode and returns a new trigger
 * node.
 */
static void
process_trigger_node (
  Router * router,
  RouteNode * node)
{
  Channel * chan;

  if (node->type == ROUTE_NODE_TYPE_PLUGIN)
    {
      plugin_process (node->pl);
    }
  else if (node->type == ROUTE_NODE_TYPE_PORT)
    {
      /* decide what to do based on what port it is */
      Port * port = node->port;

      /* if piano roll */
      if (port->flags & PORT_FLAG_PIANO_ROLL)
        {
          chan = port->owner_ch;

          if (chan->track->type ==
                TRACK_TYPE_INSTRUMENT)
            {
              /* panic MIDI if necessary */
              if (g_atomic_int_get (
                    &AUDIO_ENGINE->panic))
                {
                  midi_panic (
                    port->midi_events);
                }
              /* get events from track if playing */
              else if (TRANSPORT->play_state ==
                       PLAYSTATE_ROLLING)
                {
                  if (TRANSPORT->recording &&
                        chan->track->recording)
                    {
                      channel_handle_recording (chan);
                    }

                  /* fill midi events to pass to ins plugin */
                  instrument_track_fill_midi_events (
                    (InstrumentTrack *)chan->track,
                    &PLAYHEAD,
                    AUDIO_ENGINE->block_length,
                    port->midi_events);
                }
              midi_events_dequeue (
                port->midi_events);
            }

        }

      /* if midi editor manual press */
      else if (port == AUDIO_ENGINE->
            midi_editor_manual_press)
        {
          port_clear_buffer (port);
          midi_events_dequeue (
            AUDIO_ENGINE->
              midi_editor_manual_press->
                midi_events);
        }

      /* if channel stereo in */
      else if (port->type == TYPE_AUDIO &&
          port->flow == FLOW_INPUT &&
          port->owner_ch)
        {
          chan = port->owner_ch;

          /* fill stereo in buffers with info from
           * the current clip */
          int ret;
          switch (chan->track->type)
            {
            case TRACK_TYPE_AUDIO:
              if (g_atomic_int_get (
                &chan->filled_stereo_in_bufs))
                break;
              ret =
                g_atomic_int_compare_and_exchange (
                  &chan->filled_stereo_in_bufs,
                  0, 1);
              if (ret)
                audio_track_fill_stereo_in_buffers (
                  (AudioTrack *)chan->track,
                  chan->stereo_in);
              break;
            case TRACK_TYPE_MASTER:
            case TRACK_TYPE_BUS:
              port_sum_signal_from_inputs (
                port);
              break;
            case TRACK_TYPE_INSTRUMENT:
            case TRACK_TYPE_CHORD:
              port_sum_signal_from_inputs (port);
              break;
            }
        }

      /* if channel stereo out port */
      else if (port->owner_ch &&
          port->type == TYPE_AUDIO &&
          port->flow == FLOW_OUTPUT)
        {
          /* if muted clear it */
          if (port->owner_ch->track->mute ||
                (mixer_has_soloed_channels () &&
                   !port->owner_ch->track->solo &&
                   port->owner_ch != MIXER->master))
            {
              /* (already cleared) */
              /*port_clear_buffer (port);*/
            }
          /* if not muted/soloed process it */
          else
            {
              port_sum_signal_from_inputs (
                port);

              /* apply pan */
              port_apply_pan (
                port,
                port->owner_ch->fader.pan,
                PAN_LAW_MINUS_3DB,
                PAN_ALGORITHM_SINE_LAW);

              /* apply fader */
              port_apply_fader (
                port,
                port->owner_ch->fader.amp);
            }
          /*g_atomic_int_set (*/
            /*&port->owner_ch->processed, 1);*/
        }

      /* if JACK stereo out */
      else if (port->owner_jack &&
          port->type == TYPE_AUDIO &&
          port->flow == FLOW_OUTPUT)
        {
          if (!AUDIO_ENGINE->exporting)
            {
#ifdef HAVE_JACK
              float * out =
                (float *)
                jack_port_get_buffer (
                  JACK_PORT_T (port->data),
                  AUDIO_ENGINE->nframes);

              /* by this time, the Master channel should have its
               * Stereo Out ports filled. pass their buffers to JACK's
               * buffers */
              for (int i = 0; i < AUDIO_ENGINE->nframes; i++)
                {
                  out[i] = port->srcs[0]->buf[i];
                }

              /* avoid unused warnings */
              (void) out;
#endif
            }
        }

      else
        {
          port_sum_signal_from_inputs (port);
        }
    }

  /* set the processed node's refcount to
   * init_refcount */
  g_atomic_int_set (&node->refcount,
                    node->init_refcount);

  /* decrement ref count of dests */
  for (int i = 0; i < node->num_dests; i++)
    {
      /* if became 0 */
      if (g_atomic_int_dec_and_test (
            &node->dests[i]->refcount))
        {
          add_trigger_node (
            router,
            node->dests[i]);
        }
    }
}

static inline void
dec_and_test (Router * router)
{
  /*g_message ("***** no new trigger nodes found, finishing this thread ****");*/

  if (g_atomic_int_dec_and_test (
    &router->num_active_threads))
    {
      zix_sem_post (&router->finish_sem);
    }
}

static void *
work (void * arg)
{
  RouteNode * node;
  Router * router = (Router *) arg;

  /* loop forever */
  while (1)
    {
      /*g_message ("waiting for start");*/
      zix_sem_wait (&router->start_cycle_sem);
      /*g_message ("waited for start");*/

      /* loop in the current cycle */
      while (1)
        {
          if (g_atomic_int_get (
            &router->stop_threads))
            break;

          /* get trigger node */
          node = get_next_trigger_node (router, 1);
          /*g_message ("trig node %s",*/
                     /*node->type == ROUTE_NODE_TYPE_PLUGIN ?*/
                     /*node->pl->descr->name :*/
                     /*node->port->label);*/
          if (node)
            {
              /* process trigger node */
              process_trigger_node (router, node);
              /*g_message ("processed trigger node");*/

              /* find next trigger node */
              node =
                get_next_trigger_node (router, 0);

              /* decrement num active threads if
               * no more trigger nodes */
              if (!node)
                {
                  dec_and_test (router);
                  break;
                }
            }
          else
            {
              dec_and_test (router);
              break;
            }
          /*router_print (router);*/
        }
      /*router_print (router);*/
    }
  return 0;
}

void
router_init_threads (
  Router * router)
{
  int num_cores = audio_get_num_cores ();
  /*int num_cores = 2;*/

    for (int i = 0; i < num_cores - 1; i++)
    {
#ifdef HAVE_JACK
      jack_client_create_thread (
        AUDIO_ENGINE->client,
        &router->threads[i],
        jack_client_real_time_priority (
          AUDIO_ENGINE->client),
        jack_is_realtime (AUDIO_ENGINE->client),
        work,
        router);

      if (router->threads[i] == -1)
        {
          g_warning ("%lu: Failed creating thread %d",
                   router->threads[i], i);
          return;
        }
      router->num_threads++;
#endif
    }
}

static void
nodes_connect (
  Router * router,
  RouteNode * src,
  RouteNode * dest)
{
  g_warn_if_fail (src != NULL);
  g_warn_if_fail (dest != NULL);
  /*zix_sem_wait (&router->route_operation_sem);*/
  nodes_disconnect (router, src, dest);
  src->dests[src->num_dests++] = dest;
  dest->srcs[dest->init_refcount++] = src;
  /*zix_sem_post (&router->route_operation_sem);*/
}

static RouteNode *
find_node_from_port (
  Router * router,
  Port * port)
{
  RouteNode * node;
  for (int i = 0; i < router->registry->len; i++)
    {
      node =
        g_array_index (router->registry,
                       RouteNode *, i);
      if (node->type == ROUTE_NODE_TYPE_PORT &&
          node->port->id == port->id)
        return node;
    }
  return NULL;
}

static RouteNode *
find_node_from_plugin (
  Router * router,
  Plugin * pl)
{
  RouteNode * node;
  for (int i = 0; i < router->registry->len; i++)
    {
      node =
        g_array_index (router->registry,
                       RouteNode *, i);
      if (node->type == ROUTE_NODE_TYPE_PLUGIN &&
          node->pl->id == pl->id)
        return node;
    }
  return NULL;
}

/*static RouteNode **/
/*find_node_by_id (*/
  /*Router * router,*/
  /*int      id)*/
/*{*/
  /*return*/
    /*g_array_index (router->registry,*/
                   /*RouteNode *, id);*/
/*}*/

static void
add_node_to_registry (
  Router * router,
  RouteNode * node)
{
  /*zix_sem_wait (&router->route_operation_sem);*/
  node->id = router->registry->len;
  g_array_append_val (
    router->registry, node);

  /*zix_sem_post (&router->route_operation_sem);*/

  if (node->type == ROUTE_NODE_TYPE_PORT)
    g_hash_table_insert (
      router->port_nodes,
      &node->port->id,
      node);
  else if (node->type == ROUTE_NODE_TYPE_PLUGIN)
    g_hash_table_insert (
      router->plugin_nodes,
      &node->pl->id,
      node);
}

/**
 * Creates a RouteNode out of a Port and connects it
 * to dests/srcs.
 *
 * If it has no inputs, the node is also added in the
 * trigger nodes.
 */
static void
add_plugin_node (
  Router * router,
  Plugin * pl)
{
  /* if node exists return */
  if (find_node_from_plugin (router, pl))
    return;

  int i;

  RouteNode * node = calloc (1, sizeof (RouteNode));
  node->pl = pl;
  node->type = ROUTE_NODE_TYPE_PLUGIN;
  add_node_to_registry (router, node);
}

/**
 * Creates a RouteNode out of a Port and connects it
 * to dests/srcs.
 *
 * If it has no inputs, the node is also added in the
 * trigger nodes.
 */
static RouteNode *
add_port_node (
  Router * router,
  Port *   port)
{
  /* if node exists return */
  /*router_print (router);*/
  if (find_node_from_port (router, port))
    return NULL;

  /*g_message ("node for %s not found, adding",*/
             /*port->label);*/

  int i;

  RouteNode * node = calloc (1, sizeof (RouteNode));
  node->port = port;
  node->type = ROUTE_NODE_TYPE_PORT;
  add_node_to_registry (router, node);

  /* add to trigger nodes if no srcs */
  /*if (port->srcs->len == 0 &&*/
      /*!(port->owner_pl && port->flow == FLOW_OUTPUT))*/
    /*add_initial_trigger_node (*/
      /*router, node);*/

  RouteNode * src, * dest;
  for (i = 0; i < port->num_srcs; i++)
    {
      src =
        find_node_from_port (router, port->srcs[i]);

      /* create node if doesn't exist */
      if (!src)
        src = add_port_node (router, port->srcs[i]);

      /* connect nodes */
      nodes_connect (router, src, node);
    }
  for (i = 0; i < port->num_dests; i++)
    {
      dest =
        find_node_from_port (router, port->dests[i]);

      /* create node if doesn't exist */
      if (!dest)
        dest = add_port_node (router, port->dests[i]);

      /* connect nodes */
      nodes_connect (router, node, dest);
    }

  /* if port is outgoing from a plugin, the plugin is
   * its source */
  if (port->owner_pl &&
      port->flow == FLOW_OUTPUT)
    {
      add_plugin_node (router, port->owner_pl);
      nodes_connect (
        router,
        find_node_from_plugin (router,
                               port->owner_pl),
        node);
    }

  /* if port is going into a plugin, the plugin is
   * its dest */
  if (port->owner_pl &&
      port->flow == FLOW_INPUT)
    {
      add_plugin_node (router, port->owner_pl);
      nodes_connect (
        router,
        node,
        find_node_from_plugin (router,
                               port->owner_pl));
    }

  /*g_message ("***** this node ******");*/
  /*print_node (node, -500);*/
  /*g_message ("***** end ******");*/
  /*g_message ("**srcs ");*/
  /*for (int i = 0; i < node->srcs->len; i++)*/
    /*{*/
      /*print_node (node->srcs[i], -500);*/
    /*}*/
  /*g_message ("**dests ");*/
  /*for (int i = 0; i < node->dests->len; i++)*/
    /*{*/
      /*print_node (node->dests[i], -500);*/
    /*}*/
  /*g_message ("**end ");*/
  return node;
}

void
router_destroy (
  Router * router)
{
  g_atomic_int_set (&router->stop_threads, 1);
  for (int i = 0; i < router->num_threads; i++)
    {
      zix_sem_post (&router->start_cycle_sem);
    }

  /* wait for threads to stop */
  g_usleep (1000);

  free (router);
}

/*void*/
/*router_refresh_from (*/
  /*Router * router,*/
  /*Router * graph)*/
/*{*/
  /*[>g_message ("**REFRESHING**");<]*/
  /*RouteNode * orig, * cache;*/
  /*int i, j;*/
  /*router->num_trigger_nodes = 0;*/
  /*for (i = 0; i < router->registry->len; i++)*/
    /*{*/
      /*orig =*/
        /*g_array_index (graph->registry,*/
                       /*RouteNode *, i);*/
      /*cache =*/
        /*g_array_index (router->registry,*/
                       /*RouteNode *, i);*/
      /*[>g_message ("1a");<]*/
      /*[>print_node (orig, -500);<]*/
      /*[>g_message ("2a");<]*/
      /*[>print_node (cache, -500);<]*/
      /*cache->claimed = 0;*/
      /*for (j = 0; j < orig->srcs->len; j++)*/
        /*{*/
          /*if (orig->srcs[j]->type ==*/
                /*ROUTE_NODE_TYPE_PLUGIN)*/
            /*cache->srcs[j] =*/
              /*g_hash_table_lookup (*/
                /*router->plugin_nodes,*/
                /*&orig->srcs[j]->pl->id);*/
              /*[>find_node_from_plugin (<]*/
                /*[>router, orig->srcs[j]->pl);<]*/
          /*else if (orig->srcs[j]->type ==*/
                     /*ROUTE_NODE_TYPE_PORT)*/
            /*cache->srcs[j] =*/
              /*g_hash_table_lookup (*/
                /*router->port_nodes,*/
                /*&orig->srcs[j]->port->id);*/
              /*[>find_node_from_port (<]*/
                /*[>router, orig->srcs[j]->port);<]*/
        /*}*/
      /*cache->srcs->len = orig->srcs->len;*/
      /*[>g_message ("--dests of node above:");<]*/
      /*for (j = 0; j < orig->dests->len; j++)*/
        /*{*/
          /*[>cache->dests[j] =<]*/
            /*[>find_matching_node (router,<]*/
                                /*[>orig->dests[j]);<]*/
            /*[>find_node_by_id (router,<]*/
                             /*[>orig->dests[j]->id);<]*/
          /*[>print_node (cache->dests[j], -400);<]*/
          /*if (orig->dests[j]->type ==*/
                /*ROUTE_NODE_TYPE_PLUGIN)*/
            /*cache->dests[j] =*/
              /*g_hash_table_lookup (*/
                /*router->plugin_nodes,*/
                /*&orig->dests[j]->pl->id);*/
              /*[>find_node_from_plugin (<]*/
                /*[>router, orig->dests[j]->pl);<]*/
          /*else if (orig->dests[j]->type ==*/
                     /*ROUTE_NODE_TYPE_PORT)*/
            /*cache->dests[j] =*/
              /*g_hash_table_lookup (*/
                /*router->port_nodes,*/
                /*&orig->dests[j]->port->id);*/
              /*[>find_node_from_port (<]*/
                /*[>router, orig->dests[j]->port);<]*/
        /*}*/
      /*[>g_message ("--end");<]*/
      /*cache->dests->len = orig->dests->len;*/

      /*if (cache->srcs->len == 0)*/
        /*array_append (router->trigger_nodes,*/
                      /*router->num_trigger_nodes,*/
                      /*cache);*/

      /*[>g_message ("1b");<]*/
      /*[>print_node (orig, -500);<]*/
      /*[>g_message ("2b");<]*/
      /*[>print_node (cache, -500);<]*/
    /*}*/
  /*g_message ("finish refreshing, num trigger nodes %d",*/
             /*router->num_trigger_nodes);*/
  /*[>g_message ("**FINISHED REFRESHING**");<]*/
/*}*/

/*static int cmpfunc (const void * a, const void * b)*/
/*{*/
  /*return ((*(RouteNode* const *)a)->refcount -*/
          /*(*(RouteNode* const *)b)->refcount);*/
/*}*/

/**
 * Starts a new cycle.
 */
void
router_start_cycle (
  Router * cache)
{
  for (int i = 0; i < cache->num_init_trigger_nodes;
       i++)
    {
      cache->trigger_nodes[i] =
        cache->init_trigger_nodes[i];
    }
  cache->num_trigger_nodes =
    cache->num_init_trigger_nodes;
  /*g_message ("starting cycle");*/
  g_atomic_int_set (&cache->num_active_threads,
                    cache->num_threads);
  for (int i = 0; i < cache->num_threads; i++)
    {
      /*g_message ("posting %d", i);*/
      /*zix_sem_post (&cache->trigger_sem);*/
      zix_sem_post (&cache->start_cycle_sem);
    }

  /*RouteNode * node;*/
  /*while (node = get_next_trigger_node (cache, 0))*/
    /*{*/
      /*zix_sem_post (&cache->trigger_sem);*/
    /*}*/

  /*g_message ("waiting for finish");*/
  zix_sem_wait (&cache->finish_sem);
  /*g_message ("finished waiting for finish");*/
}

void
router_print (
  Router * router)
{
  g_message ("==printing router");
  RouteNode * node;
  for (int i = 0; i < router->registry->len; i++)
    {
      print_node (
        g_array_index (router->registry,
                       RouteNode *, i),
        i);
    }
  g_message ("==finish printing router");
}

Router *
router_new ()
{
  Router * self = calloc (1, sizeof (Router));
  /*zix_sem_init (&self->route_operation_sem, 1);*/

  self->port_nodes =
    g_hash_table_new (
      g_int_hash,
      g_int_equal);
  self->plugin_nodes =
    g_hash_table_new (
      g_int_hash,
      g_int_equal);
  self->registry =
    g_array_new (0, 0, sizeof (RouteNode *));

  /* add ports */
  Port * port;
  for (int i = 0; i < PROJECT->num_ports; i++)
    {
      port = project_get_port (i);
      if (!port) continue;

      /*g_message ("adding port node %s",*/
                 /*port->label);*/
      add_port_node (self, port);
    }

  RouteNode * node;
  for (int i = 0; i < self->registry->len; i++)
    {
      node =
        g_array_index (self->registry,
                       RouteNode *, i);

      /* add trigger node */
      if (node->init_refcount == 0)
        {
          array_append (self->init_trigger_nodes,
                        self->num_init_trigger_nodes,
                        node);
          array_append (self->trigger_nodes,
                        self->num_trigger_nodes,
                        node);
        }
      node->refcount = node->init_refcount;
    }

  g_message ("num trigger nodes %d",
             self->num_init_trigger_nodes);

  /*router_print (self);*/
  /*qsort (self->registry,*/
         /*self->num_nodes,*/
         /*sizeof (RouteNode *),*/
         /*cmpfunc);*/

  /*zix_sem_init (&self->trigger_sem, 0);*/
  /*zix_sem_init (&self->initing_sem, 0);*/
  router_init_threads (self);

  zix_sem_init (&self->finish_sem, 0);

  return self;
}
