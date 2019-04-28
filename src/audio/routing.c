/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 * Copyright (C) 2017, 2019 Robin Gareus <robin@gareus.org>
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
#ifdef HAVE_PORT_AUDIO
#include "audio/engine_pa.h"
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

#define MAGIC_NUMBER 76322

static inline void
graph_reached_terminal_node (Graph* self);

/* called from a node when all its incoming edges have
 * completed processing and the node can run.
 * It is added to the "work queue" */
static inline void
graph_trigger (
  Graph* graph,
  GraphNode* n)
{
  /*g_message ("locking trigger mutex in graph trigger");*/
	pthread_mutex_lock (&graph->trigger_mutex);
	g_warn_if_fail (
    graph->n_trigger_queue <
    graph->trigger_queue_size);
  /*g_message ("appending %s to trigger nodes",*/
             /*n->type == ROUTE_NODE_TYPE_PORT ?*/
             /*n->port->label :*/
             /*n->pl->descr->name);*/
	graph->trigger_queue[
    graph->n_trigger_queue++] = n;
  pthread_mutex_unlock (&graph->trigger_mutex);
}

/**
 * Called by an upstream node when it has completed
 * processing.
 */
static inline void
node_trigger (
  GraphNode * self)
{
  /*g_message ("triggering %s",*/
             /*self->type == ROUTE_NODE_TYPE_PORT ?*/
             /*self->port->label :*/
             /*self->pl->descr->name);*/
  /* check if we can run */
	if (g_atomic_int_dec_and_test (&self->refcount))
    {
      /*g_message ("all nodes that feed it have completed");*/
        /* reset reference count for next cycle */
      g_atomic_int_set (&self->refcount,
                        self->init_refcount);
      /*if (self->type == ROUTE_NODE_TYPE_PLUGIN)*/
      /*g_message ("reset refcount to %d",*/
                 /*self->refcount);*/
      /* all nodes that feed this node have
       * completed, so this node be processed
       * now. */
      graph_trigger (self->graph, self);
    }
}

static inline void
node_finish (
  GraphNode * self)
{
  int feeds = 0;

	/* notify downstream nodes that depend on this
   * node */
	for (int i = 0; i < self->n_childnodes; ++i)
    {
      /*g_message ("notifying dest %d", i);*/
      node_trigger (self->childnodes[i]);
      feeds = 1;
    }

	/* if there are no outgoing edges, this is a
   * terminal node */
	if (!feeds)
    {
      /* notify parent graph */
      graph_reached_terminal_node (self->graph);
    }
}

static void
print_node (GraphNode * node)
{
  GraphNode * dest;
  if (!node)
    {
      g_message ("(null) node");
      return;
    }

  char * str1 =
    g_strdup_printf (
      "node [(%d) %s] refcount: %d | terminal: %s | initial: %s",
      node->id,
      node->type == ROUTE_NODE_TYPE_PLUGIN ?
        node->pl->descr->name :
        node->port->label,
      node->refcount,
      node->terminal ? "yes" : "no",
      node->initial ? "yes" : "no");
  char * str2;
  for (int j = 0; j < node->n_childnodes; j++)
    {
      dest = node->childnodes[j];
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

/**
 * Processes the GraphNode and returns a new trigger
 * node.
 */
static void
node_process (
  GraphNode * node)
{
  int i;
  Channel * chan;

  /*g_message ("num trigger nodes %d, max_trigger nodes %d", node->graph->n_trigger_queue, node->graph->trigger_queue_size);*/
  /*for (i = 0; i < node->graph->n_trigger_queue; i++)*/
    /*{*/
      /*GraphNode * n = node->graph->trigger_queue[i];*/
      /*g_message ("trigger node %d: %s",*/
                 /*i,*/
                 /*n->type == ROUTE_NODE_TYPE_PORT ?*/
                 /*n->port->label :*/
                 /*n->pl->descr->name);*/
    /*}*/

  if (node->type == ROUTE_NODE_TYPE_PLUGIN)
    {
      /*g_message ("processing plugin %s",*/
                 /*node->pl->descr->name);*/
      plugin_process (node->pl);
    }
  else if (node->type == ROUTE_NODE_TYPE_PORT)
    {
      /* decide what to do based on what port it is */
      Port * port = node->port;
      /*g_message ("processing port %s",*/
                 /*node->port->label);*/

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
                AUDIO_ENGINE->pan_law,
                AUDIO_ENGINE->pan_algo);

              /* apply fader */
              port_apply_fader (
                port,
                port->owner_ch->fader.amp);
            }
          /*g_atomic_int_set (*/
            /*&port->owner_ch->processed, 1);*/
        }

      /* if JACK stereo out */
      else if (port->owner_backend &&
          port->type == TYPE_AUDIO &&
          port->flow == FLOW_OUTPUT)
        {
          if (!AUDIO_ENGINE->exporting)
            {
              float * out;
              switch (AUDIO_ENGINE->audio_backend)
                {
                case AUDIO_BACKEND_JACK:
#ifdef HAVE_JACK
                  out =
                    (float *)
                    jack_port_get_buffer (
                      JACK_PORT_T (port->data),
                      AUDIO_ENGINE->nframes);

                  /* by this time, the Master channel should have its
                   * Stereo Out ports filled. pass their buffers to JACK's
                   * buffers */
                  int nframes = AUDIO_ENGINE->nframes;
                  for (i = 0;
                       i < nframes;
                       i++)
                    {
                      out[i] = port->srcs[0]->buf[i];
                    }

                  /* avoid unused warnings */
                  (void) out;
#endif

                  break;
                case AUDIO_BACKEND_PORT_AUDIO:
#ifdef HAVE_PORT_AUDIO
                  if (g_atomic_int_get (
                    &AUDIO_ENGINE->
                      filled_stereo_out_bufs))
                    break;
                  int ret =
                    g_atomic_int_compare_and_exchange (
                      &AUDIO_ENGINE->
                        filled_stereo_out_bufs,
                      0, 1);
                  if (ret)
                    engine_pa_fill_stereo_out_buffs (
                      AUDIO_ENGINE);
#endif
                  break;
                case AUDIO_BACKEND_ALSA:
                case AUDIO_BACKEND_DUMMY:
                  break;
                default:
                  break;
                }
            }
        }

      else
        {
          port_sum_signal_from_inputs (port);
        }
    }
}

static inline void
node_run (GraphNode * self)
{
  /*graph_print (self->graph);*/
  node_process (self);
  node_finish (self);
}

static inline void
add_feeds (
  GraphNode * self,
  GraphNode * dest)
{
  for (int i = 0; i < self->n_childnodes; i++)
    if (self->childnodes[i] == dest)
      return;

  self->childnodes =
    (GraphNode **) realloc (
      self->childnodes,
      (1 + self->n_childnodes) *
        sizeof (GraphNode *));
  self->childnodes[self->n_childnodes++] = dest;
}

static inline void
add_depends (
  GraphNode * self)
{
  ++self->init_refcount;
  self->refcount = self->init_refcount;
}

/*static int cnt = 0;*/

static void *
graph_worker_thread (void * g)
{
	Graph * graph = (Graph*) g;
  GraphNode* to_run = NULL;
  int wakeup, i;
	do
    {
      /*g_usleep (5000);*/
      /*graph_print (self);*/

      to_run = NULL;

      /*g_message ("locking trigger mutex in worker thread");*/
      pthread_mutex_lock (&graph->trigger_mutex);

      if (graph->terminate)
        {
          pthread_mutex_unlock (
            &graph->trigger_mutex);
          return 0;
        }

      if (graph->n_trigger_queue > 0)
        to_run =
          graph->trigger_queue[
            --graph->n_trigger_queue];
      /*g_message ("acquired trig node, num_trig_nodes now: %d", self->n_trigger_queue);*/

      /*g_message ("idle threads: %d, trigger nodes: %d",*/
                 /*graph->idle_thread_cnt,*/
                 /*graph->n_trigger_queue);*/
      wakeup =
        MIN (graph->idle_thread_cnt,
             graph->n_trigger_queue);

      /* wake up threads */
      graph->idle_thread_cnt -= wakeup;
      /*g_message ("wake up %d", wakeup);*/
      for (i = 0; i < wakeup; i++)
        {
          /*g_message ("posting trigger for wakeup %d",*/
                     /*i);*/
          zix_sem_post (&graph->trigger);
        }

      while (!to_run)
        {
          /* wait for work, fall asleep */
          /*g_message ("waiting for work, incrementing idle thread count");*/
          graph->idle_thread_cnt++;
          /*g_message ("idle thread cnt %d",*/
                     /*graph->idle_thread_cnt);*/
          g_warn_if_fail (
            graph->idle_thread_cnt <=
            graph->num_threads);
          pthread_mutex_unlock (
            &graph->trigger_mutex);
          /*g_message ("waiting trigger");*/
          zix_sem_wait (&graph->trigger);
          /*g_message ("waited");*/

          if (graph->terminate)
            return 0;

          /* try to find some work to do */
          /*g_message ("locking to find some work");*/
          pthread_mutex_lock (
            &graph->trigger_mutex);
          if (graph->n_trigger_queue > 0)
            to_run =
              graph->trigger_queue[
                --graph->n_trigger_queue];
        }
      pthread_mutex_unlock (&graph->trigger_mutex);

      /* process graph-node */
      node_run (to_run);

    } while (!graph->terminate);
	return 0;
}

__attribute__((annotate("realtime")))
static void *
graph_main_thread (void * arg)
{
  g_message ("THREAD CREATED");
  Graph * graph = (Graph *) arg;

  /* wait for initial process callback */
  /*g_message ("waiting callback start");*/
  zix_sem_wait (&graph->callback_start);

  g_message ("received message callback start in main thread of graph %p", graph);

  /*g_message ("locking in main thread");*/
  pthread_mutex_lock (&graph->trigger_mutex);

  if (!graph->destroying)
    {
      /* Can't run without a graph */
      g_warn_if_fail (graph->n_graph_nodes > 0);
      g_warn_if_fail (graph->n_init_triggers > 0);
      g_warn_if_fail (graph->terminal_node_count > 0);
    }

	/* bootstrap trigger-list.
	 * (later this is done by
   * Graph_reached_terminal_node)*/
	for (int i = 0;
       i < graph->n_init_triggers; ++i)
    {
      g_warn_if_fail (
        graph->n_trigger_queue <
        graph->trigger_queue_size);
      /*g_message ("init trigger node %d, num trigger nodes %d, max_trigger nodes %d", i, self->n_trigger_queue, self->trigger_queue_size);*/
      graph->trigger_queue[
        graph->n_trigger_queue++] =
          graph->init_trigger_list[i];
    }
	pthread_mutex_unlock (&graph->trigger_mutex);

	/* after setup, the main-thread just becomes
   * a normal worker */
  return graph_worker_thread (graph);
}

/**
 * Initializes as many threads as there are cores.
 */
void
graph_init_threads (
  Graph * graph)
{
  int num_cores = audio_get_num_cores ();
  /*g_message ("num cores %d", num_cores);*/
  /*int num_cores = 16;*/

  /* create worker threads (num cores - 2 because
   * the main thread will become a worker too, so
   * in total N_CORES - 1 threads */
    for (int i = 0; i < num_cores - 2; i++)
    {
      graph->num_threads++;
      /*zix_sem_post (&graph->callback_start);*/
      /*zix_sem_post (&graph->callback_done);*/
#ifdef HAVE_JACK
      if (AUDIO_ENGINE->audio_backend ==
            AUDIO_BACKEND_JACK)
        {
          jack_client_create_thread (
            AUDIO_ENGINE->client,
            &graph->jthreads[i],
            jack_client_real_time_priority (
              AUDIO_ENGINE->client),
            jack_is_realtime (AUDIO_ENGINE->client),
            graph_worker_thread,
            graph);

          if ((int) graph->jthreads[i] == -1)
            {
              g_warning (
                "%lu: Failed creating thread %d",
                graph->jthreads[i], i);
              return;
            }
        }
      else
        {
#endif
          pthread_create (
            &graph->threads[i], NULL,
            &graph_worker_thread, graph);
          g_message ("created thread %d", i);
#ifdef HAVE_JACK
        }
#endif
    }

  /* and the main thread */
#ifdef HAVE_JACK
  if (AUDIO_ENGINE->audio_backend ==
        AUDIO_BACKEND_JACK)
    {
      jack_client_create_thread (
        AUDIO_ENGINE->client,
        &graph->jmain_thread,
        jack_client_real_time_priority (
          AUDIO_ENGINE->client),
        jack_is_realtime (AUDIO_ENGINE->client),
        graph_main_thread,
        graph);

      if ((int) graph->jmain_thread == -1)
        {
          g_warning (
            "%lu: Failed creating main thread",
            graph->jmain_thread);
          return;
        }
    }
  else
    {
#endif
      pthread_create (
        &graph->main_thread, NULL,
        &graph_main_thread, graph);
      g_message ("created main thread");
#ifdef HAVE_JACK
    }
#endif

  /* breathe */
  sched_yield ();
}

static void
node_connect (
  GraphNode * from,
  GraphNode * to)
{
  if (array_contains (from->childnodes,
                      from->n_childnodes,
                      to))
    return;

  add_feeds (from, to);
  add_depends (to);
}

static GraphNode *
find_node_from_port (
  Graph * graph,
  Port * port)
{
  GraphNode * node;
  for (int i = 0; i < graph->n_graph_nodes; i++)
    {
      node = graph->graph_nodes[i];
      if (node->type == ROUTE_NODE_TYPE_PORT &&
          node->port->id == port->id)
        return node;
    }
  return NULL;
}

static GraphNode *
find_node_from_plugin (
  Graph * graph,
  Plugin * pl)
{
  GraphNode * node;
  for (int i = 0; i < graph->n_graph_nodes; i++)
    {
      node = graph->graph_nodes[i];
      if (node->type == ROUTE_NODE_TYPE_PLUGIN &&
          node->pl->id == pl->id)
        return node;
    }
  return NULL;
}

static inline GraphNode *
graph_node_new (
  Graph * graph,
  GraphNodeType type,
  void *   data)
{
  GraphNode * node =
    calloc (1, sizeof (GraphNode));
  node->id = graph->n_graph_nodes;
  node->graph = graph;
  node->type = type;
  if (type == ROUTE_NODE_TYPE_PLUGIN)
    node->pl = (Plugin *) data;
  else if (type == ROUTE_NODE_TYPE_PORT)
    node->port = (Port *) data;

  return node;
}

static inline GraphNode *
graph_add_node (
  Graph * graph,
  GraphNodeType type,
  void * data)
{
  free (graph->trigger_queue);
  graph->trigger_queue =
    (GraphNode **) malloc (
      ++graph->trigger_queue_size *
      sizeof (GraphNode *));
  graph->graph_nodes =
    (GraphNode **) realloc (
      graph->graph_nodes,
      (1 + graph->n_graph_nodes) *
      sizeof (GraphNode *));

  graph->graph_nodes[graph->n_graph_nodes] =
    graph_node_new (graph, type, data);
  return graph->graph_nodes[graph->n_graph_nodes++];
}

static inline GraphNode *
graph_add_terminal_node (
  Graph * self,
  GraphNodeType type,
  void * data)
{
  self->terminal_refcnt =
    ++self->terminal_node_count;
  GraphNode * node =
    graph_add_node (self, type, data);
  node->terminal = 1;
  /*g_message ("adding terminal node %d",*/
             /*node->id);*/
  return node;
}

static inline GraphNode *
graph_add_initial_node (
  Graph * self,
  GraphNodeType type,
  void * data)
{
  self->init_trigger_list =
    (GraphNode**)realloc (
      self->init_trigger_list,
      (1 + self->n_init_triggers) *
        sizeof (GraphNode*));

  self->init_trigger_list[
    self->n_init_triggers] =
      graph_add_node (self, type, data);
	GraphNode * node =
    self->init_trigger_list[
      self->n_init_triggers++];
  node->initial = 1;
  return node;
}

static inline void
node_free (
  GraphNode * node)
{
  free (node->childnodes);
  free (node);
}

static inline void
graph_free (
  Graph * self)
{
  int i;

  for (i = 0; i < self->n_graph_nodes; ++i)
    {
		node_free (self->graph_nodes[i]);
    }
	free (self->graph_nodes);

  pthread_mutex_destroy (&self->trigger_mutex);

  zix_sem_destroy (&self->callback_start);
  zix_sem_destroy (&self->callback_done);
  zix_sem_destroy (&self->trigger);
}

static void
graph_terminate (
  Graph * self)
{
  /*g_message ("locking to destroy");*/
  pthread_mutex_lock (&self->trigger_mutex);
  self->terminate = 1;
	self->init_trigger_list = NULL;
	self->n_init_triggers   = 0;
	self->trigger_queue     = NULL;
	self->n_trigger_queue   = 0;
  free (self->init_trigger_list);
  free (self->trigger_queue);

	int tc = self->idle_thread_cnt;
  g_warn_if_fail (tc == self->num_threads);
	pthread_mutex_unlock (&self->trigger_mutex);

	/* wake-up sleeping threads */
	for (int i = 0; i < tc; ++i) {
		zix_sem_post (&self->trigger);
	}
	/* and the main thread */
  /*g_message ("locking trigger mutex to post callback");*/
	pthread_mutex_lock (&self->trigger_mutex);
	zix_sem_post (&self->callback_start);
  pthread_mutex_unlock (&self->trigger_mutex);
}

void
graph_destroy (
  Graph * self)
{
  g_message ("destroying graph %p (router g1 %p g2 %p",
             self,
             self->router->graph1,
             self->router->graph2);
  int i;
  self->destroying = 1;

  graph_terminate (self);

  /* wait for threads to finish */
  g_usleep (1000);

  for (i = 0; i < self->num_threads; ++i) {
		/*pthread_join (workers[i], NULL);*/
  }

  graph_free (self);
  free (self);
}

void
router_cleanup (
  Router * self)
{
  graph_destroy (self->graph1);
  graph_destroy (self->graph2);
}


/* called from a terminal node (from the Graph worked-thread)
 * to indicate it has completed processing.
 *
 * The thread of the last terminal node that reaches here
 * will inform the main-thread, wait, and kick off the next process cycle.
 */
static inline void
graph_reached_terminal_node (
  Graph *  graph)
{
  /*g_message ("reached terminal node");*/
	if (g_atomic_int_dec_and_test (
        &graph->terminal_refcnt))
    {
      /*g_message ("all terminal nodes completed");*/
      /* all terminal nodes have completed,
       * we're done with this cycle.
       */
      zix_sem_post (&graph->callback_done);

      /* now wait for the next cycle to begin */
      /*g_message ("waiting callback start");*/
      zix_sem_wait (&graph->callback_start);

      if (graph->terminate)
        return;

      /* reset terminal reference count */
      g_atomic_int_set (
        &graph->terminal_refcnt,
        graph->terminal_node_count);

      /* and start the initial nodes */
      /*g_message ("locking trigger mutex to start initial nodes");*/
      /* FIXME use double buffering instead of
       * blocking (have another spare array and then
       * just switch the pointer) */
      pthread_mutex_lock (
        &graph->trigger_mutex);
      for (int i = 0;
           i < graph->n_init_triggers; ++i)
        {
          g_warn_if_fail (
            graph->n_trigger_queue <
            graph->trigger_queue_size);
          graph->trigger_queue[
            graph->n_trigger_queue++] =
              graph->init_trigger_list[i];
        }
      pthread_mutex_unlock (&graph->trigger_mutex);
      /* continue in worker-thread */
    }
  /*g_message ("terminal refcount: %d",*/
             /*g_atomic_int_get (*/
               /*&self->terminal_refcnt));*/
}

/**
 * Starts a new cycle.
 */
void
router_start_cycle (
  Router * self)
{
  /*self->n_trigger_queue = 0;*/
  /*g_message ("num trigger nodes at start: %d, num init trigger nodes at start: %d, num_max trigger nodes at start: %d",*/
             /*self->n_trigger_queue,*/
             /*self->n_init_triggers,*/
             /*self->trigger_queue_size);*/
  /*for (int i = 0; i < self->num_threads; i++)*/
  if (!self->graph2)
    return;

  self->graph1 = self->graph2;
  zix_sem_post (&self->graph1->callback_start);
  /*g_message ("waiting callback done");*/
  /*for (int i = 0; i < self->num_threads; i++)*/
  zix_sem_wait (&self->graph1->callback_done);
  /*g_message ("num trigger nodes at end: %d, num init trigger nodes at end %d, num_max trigger nodes at end: %d",*/
             /*self->n_trigger_queue,*/
             /*self->n_init_triggers,*/
             /*self->trigger_queue_size);*/
}

void
graph_print (
  Graph * graph)
{
  g_message ("==printing graph");
  /*GraphNode * node;*/
  for (int i = 0; i < graph->n_graph_nodes; i++)
    {
      print_node (graph->graph_nodes[i]);
    }
  g_message ("==finish printing graph");
}

Graph *
graph_new (
  Router * router)
{
  int i, j;
  GraphNode * node, * node2;
  Graph * self = calloc (1, sizeof (Graph));
  g_warn_if_fail (router);
  self->router = router;

  self->port_nodes =
    g_hash_table_new (
      g_int_hash,
      g_int_equal);
  self->plugin_nodes =
    g_hash_table_new (
      g_int_hash,
      g_int_equal);

  /* ========================
   * first add all the nodes
   * ======================== */

  /* add plugins */
  Plugin * plugin;
  for (i = 0; i < PROJECT->num_plugins; i++)
    {
      plugin = project_get_plugin (i);
      if (!plugin || plugin->deleting) continue;

      if (plugin->num_in_ports == 0 &&
          plugin->num_out_ports > 0)
        graph_add_initial_node (
          self, ROUTE_NODE_TYPE_PLUGIN, plugin);
      else if (plugin->num_out_ports == 0 &&
               plugin->num_in_ports > 0)
        graph_add_terminal_node (
          self, ROUTE_NODE_TYPE_PLUGIN, plugin);
      else if (plugin->num_out_ports == 0 &&
               plugin->num_in_ports == 0)
        {
        }
      else
        graph_add_node (
          self, ROUTE_NODE_TYPE_PLUGIN, plugin);
    }

  /* add ports */
  Port * port;
  for (i = 0; i < PROJECT->num_ports; i++)
    {
      /* FIXME project arrays should be hashtables
       */
      port = project_get_port (i);
      if (!port || port->deleting ||
          (port->owner_pl &&
           port->owner_pl->deleting))
        continue;

      /*g_message ("adding port node %s",*/
                 /*port->label);*/
      /*add_port_node (self, port);*/
      if (port->num_dests == 0 &&
          port->num_srcs == 0 &&
          port->owner_pl)
        {
          if (port->flow == FLOW_INPUT)
            graph_add_initial_node (
              self, ROUTE_NODE_TYPE_PORT, port);
          else if (port->flow == FLOW_OUTPUT)
            graph_add_terminal_node (
              self, ROUTE_NODE_TYPE_PORT, port);
        }
      else if (port->num_dests == 0 &&
               port->num_srcs == 0 &&
               !port->owner_pl)
        {
        }
      else if (port->num_srcs == 0 &&
          !(port->owner_pl &&
            port->flow == FLOW_OUTPUT))
        graph_add_initial_node (
          self, ROUTE_NODE_TYPE_PORT, port);
      else if (port->num_dests == 0 &&
               port->num_srcs > 0 &&
               !(port->owner_pl &&
                 port->flow == FLOW_INPUT))
        graph_add_terminal_node (
          self, ROUTE_NODE_TYPE_PORT, port);
      else
        graph_add_node (
          self, ROUTE_NODE_TYPE_PORT, port);
    }

  /* ========================
   * now connect them
   * ======================== */
  for (i = 0; i < PROJECT->num_plugins; i++)
    {
      plugin = project_get_plugin (i);
      if (!plugin || plugin->deleting)
        continue;

      node = find_node_from_plugin (self, plugin);
      for (j = 0; j < plugin->num_in_ports; j++)
        {
          port = plugin->in_ports[j];
          g_warn_if_fail (port->owner_pl != NULL);
          node2 = find_node_from_port (self, port);
          node_connect (node2, node);
        }
      for (j = 0; j < plugin->num_out_ports; j++)
        {
          port = plugin->out_ports[j];
          g_warn_if_fail (port->owner_pl != NULL);
          node2 = find_node_from_port (self, port);
          node_connect (node, node2);
        }
    }

  Port * src, * dest;
  for (i = 0; i < PROJECT->num_ports; i++)
    {
      port = project_get_port (i);
      if (!port || port->deleting ||
          (port->owner_pl &&
           port->owner_pl->deleting))
        continue;

      node = find_node_from_port (self, port);
      for (j = 0; j < port->num_srcs; j++)
        {
          src = port->srcs[j];
          node2 = find_node_from_port (self, src);
          node_connect (node2, node);
        }
      for (j = 0; j < port->num_dests; j++)
        {
          dest = port->dests[j];
          node2 = find_node_from_port (self, dest);
          node_connect (node, node2);
        }
    }

  g_message ("num trigger nodes %d",
             self->n_init_triggers);
  g_message ("num max trigger nodes %d",
             self->trigger_queue_size);
  g_message ("num terminal nodes %d",
             self->terminal_node_count);
  graph_print (self);

  zix_sem_init (&self->callback_start, 0);
  zix_sem_init (&self->callback_done, 0);
  zix_sem_init (&self->trigger, 0);

  pthread_mutex_init (&self->trigger_mutex, NULL);

  /* create worker threads */
  graph_init_threads (self);

  return self;
}

void
router_init (
  Router * self)
{
  /* create the initial graph */
  self->graph1 = graph_new (self);

  self->graph2 = NULL;
}
