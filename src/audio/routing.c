/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 * Copyright (C) 2017, 2019 Robin Gareus <robin@gareus.org>
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

#include "config.h"

#include "audio/audio_track.h"
#include "audio/engine.h"
#include "audio/engine_alsa.h"
#ifdef HAVE_JACK
#include "audio/engine_jack.h"
#endif
#ifdef HAVE_PORT_AUDIO
#include "audio/engine_pa.h"
#endif
#include "audio/master_track.h"
#include "audio/midi.h"
#include "audio/midi_track.h"
#include "audio/modulator.h"
#include "audio/pan.h"
#include "audio/passthrough_processor.h"
#include "audio/port.h"
#include "audio/routing.h"
#include "audio/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/stoat.h"

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
      g_atomic_int_set (
        &self->refcount,
        (unsigned int) self->init_refcount);
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

      /* set the largest playback latency of this
       * route to the child as well */
      self->childnodes[i]->route_playback_latency =
        self->playback_latency;
        /*MAX (*/
          /*self->playback_latency,*/
          /*self->childnodes[i]->*/
            /*route_playback_latency);*/
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

/**
 * Returns a human friendly name of the node.
 *
 * Must be free'd.
 */
static char *
get_node_name (
  GraphNode * node)
{
  switch (node->type)
    {
    case ROUTE_NODE_TYPE_PLUGIN:
      return
        g_strdup (node->pl->descr->name);
      break;
    case ROUTE_NODE_TYPE_PORT:
      return
        port_get_full_designation (node->port);
      break;
    case ROUTE_NODE_TYPE_FADER:
      return
        g_strdup_printf (
          "%s Fader",
          node->fader->channel->track->name);
      break;
    case ROUTE_NODE_TYPE_PREFADER:
      return
        g_strdup_printf (
          "%s Pre-Fader",
          node->prefader->channel->track->name);
      break;
    case ROUTE_NODE_TYPE_MONITOR_FADER:
      return
        g_strdup ("Monitor Fader");
      break;
    case ROUTE_NODE_TYPE_SAMPLE_PROCESSOR:
      return
        g_strdup ("Sample Processor");
      break;
    }
  g_return_val_if_reached (NULL);
}

static void
print_node (
  GraphNode * node)
{
  GraphNode * dest;
  if (!node)
    {
      g_message ("(null) node");
      return;
    }

  char * name = get_node_name (node);
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
      name = get_node_name (dest);
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

/**
 * Processes the GraphNode and returns a new trigger
 * node.
 */
static void
node_process (
  GraphNode * node,
  const nframes_t   nframes)
{
  int noroll = 0;
  Channel * chan;

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
      noroll = 1;

      /* if no-roll, only process terminal nodes
       * to set their buffers to 0 */
      if (!node->terminal)
        return;
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
      g_start_frames =
        transport_frames_add_frames (
          TRANSPORT, PLAYHEAD->frames,
          node->route_playback_latency -
          AUDIO_ENGINE->remaining_latency_preroll);
    }
  else
    {
      g_start_frames = PLAYHEAD->frames;
    }

  if (node->type == ROUTE_NODE_TYPE_PLUGIN)
    {
      plugin_process (
        node->pl, g_start_frames, nframes);
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
      passthrough_processor_process (
        node->prefader, local_offset, nframes);
    }
  else if (node->type ==
           ROUTE_NODE_TYPE_SAMPLE_PROCESSOR)
    {
      sample_processor_process (
        node->sample_processor, local_offset,
        nframes);
    }
  else if (node->type == ROUTE_NODE_TYPE_PORT)
    {
      /* decide what to do based on what port it
       * is */
      Port * port = node->port;

      /* if piano roll */
      if (port->identifier.flags &
            PORT_FLAG_PIANO_ROLL)
        {
          chan = port->track->channel;

          if (chan->track->type ==
                TRACK_TYPE_INSTRUMENT ||
              chan->track->type ==
                TRACK_TYPE_MIDI)
            {
              /* panic MIDI if necessary */
              if (g_atomic_int_get (
                    &AUDIO_ENGINE->panic))
                {
                  midi_events_panic (
                    port->midi_events, 1);
                }
              /* get events from track if playing */
              else if (TRANSPORT->play_state ==
                       PLAYSTATE_ROLLING)
                {
                  if (TRANSPORT->recording &&
                        chan->track->recording)
                    {
                      channel_handle_recording (
                        chan, g_start_frames,
                        nframes);
                    }

                  /* fill midi events to pass to
                   * ins plugin */
                  midi_track_fill_midi_events (
                    chan->track,
                    g_start_frames,
                    local_offset,
                    nframes,
                    port->midi_events);
                }
              midi_events_dequeue (
                port->midi_events);
              if (port->midi_events->num_events > 0)
                g_message (
                  "piano roll port %s has %d events",
                  port->identifier.label,
                  port->midi_events->num_events);
            }
        }

      /* if midi editor manual press */
      else if (port == AUDIO_ENGINE->
            midi_editor_manual_press)
        {
          midi_events_dequeue (
            AUDIO_ENGINE->
              midi_editor_manual_press->
                midi_events);
        }

      /* if channel stereo in */
      else if (
          port->identifier.type == TYPE_AUDIO &&
          port->identifier.flow == FLOW_INPUT &&
          port->identifier.owner_type ==
            PORT_OWNER_TYPE_TRACK)
        {
          chan = port->track->channel;

          /* fill stereo in buffers with info from
           * the current clip */
          /*int ret;*/
          switch (chan->track->type)
            {
            case TRACK_TYPE_AUDIO:
              audio_track_fill_stereo_in_from_clip (
                chan->track,
                port,
                g_start_frames,
                local_offset,
                nframes);
              break;
            case TRACK_TYPE_MASTER:
            case TRACK_TYPE_AUDIO_BUS:
            case TRACK_TYPE_AUDIO_GROUP:
            case TRACK_TYPE_INSTRUMENT:
            case TRACK_TYPE_CHORD:
            case TRACK_TYPE_MARKER:
            case TRACK_TYPE_MIDI:
            default:
              port_sum_signal_from_inputs (
                port, local_offset,
                nframes, noroll);
              break;
            }
        }

      /* if channel stereo out port */
      else if (port->track &&
          port->identifier.owner_type ==
            PORT_OWNER_TYPE_TRACK &&
          port->identifier.type == TYPE_AUDIO &&
          port->identifier.flow == FLOW_OUTPUT)
        {
          /* if muted clear it */
          if (port->track->mute ||
                (tracklist_has_soloed (
                  TRACKLIST) &&
                   !port->track->solo &&
                   port->track != P_MASTER_TRACK))
            {
              /* TODO */
              /*port_clear_buffer (port);*/
            }
          /* if not muted/soloed process it */
          else
            {
              port_sum_signal_from_inputs (
                port, local_offset,
                nframes, noroll);
            }
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
            port, local_offset,
            nframes, noroll);
        }
    }
}

static inline void
node_run (GraphNode * self)
{
  /*graph_print (self->graph);*/
  node_process (
    self, self->graph->router->nsamples);
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
      (size_t) (1 + self->n_childnodes) *
        sizeof (GraphNode *));
  self->childnodes[self->n_childnodes++] = dest;
}

static inline void
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
          /*g_message ("unlocking");*/
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
      /*g_message ("unlocking");*/
      pthread_mutex_unlock (&graph->trigger_mutex);

      /* process graph-node */
      node_run (to_run);

    } while (!graph->terminate);
	return 0;
}

REALTIME
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
static void
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

/**
 * Returns the max playback latency of the trigger
 * nodes.
 */
static nframes_t
graph_get_max_playback_latency (
  Graph * graph)
{
  nframes_t max = 0;
  GraphNode * node;
  for (int i = 0; i < graph->n_init_triggers; i++)
    {
      node =  graph->init_trigger_list[i];
      if (node->playback_latency > max)
        max = node->playback_latency;
    }

  return max;
}

/**
 * Returns the max playback latency of the trigger
 * nodes.
 */
nframes_t
router_get_max_playback_latency (
  Router * router)
{
  router->max_playback_latency =
    graph_get_max_playback_latency (
      router->graph2);

  return router->max_playback_latency;
}

static void
node_connect (
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
}

static GraphNode *
find_node_from_port (
  Graph * graph,
  const Port * port)
{
  GraphNode * node;
  for (int i = 0; i < graph->n_graph_nodes; i++)
    {
      node = graph->graph_nodes[i];
      if (node->type == ROUTE_NODE_TYPE_PORT &&
          node->port == port)
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
          node->pl == pl)
        return node;
    }
  return NULL;
}

static GraphNode *
find_node_from_fader (
  Graph * graph,
  Fader * fader)
{
  GraphNode * node;
  for (int i = 0; i < graph->n_graph_nodes; i++)
    {
      node = graph->graph_nodes[i];
      if (node->type == ROUTE_NODE_TYPE_FADER &&
          node->fader == fader)
        return node;
    }
  return NULL;
}

static GraphNode *
find_node_from_prefader (
  Graph * graph,
  PassthroughProcessor * prefader)
{
  GraphNode * node;
  for (int i = 0; i < graph->n_graph_nodes; i++)
    {
      node = graph->graph_nodes[i];
      if (node->type == ROUTE_NODE_TYPE_PREFADER &&
          node->prefader == prefader)
        return node;
    }
  return NULL;
}

static GraphNode *
find_node_from_sample_processor (
  Graph * graph,
  SampleProcessor * sample_processor)
{
  GraphNode * node;
  for (int i = 0; i < graph->n_graph_nodes; i++)
    {
      node = graph->graph_nodes[i];
      if (node->type ==
            ROUTE_NODE_TYPE_SAMPLE_PROCESSOR &&
          node->sample_processor ==
            sample_processor)
        return node;
    }
  return NULL;
}

static GraphNode *
find_node_from_monitor_fader (
  Graph * graph,
  Fader * fader)
{
  GraphNode * node;
  for (int i = 0; i < graph->n_graph_nodes; i++)
    {
      node = graph->graph_nodes[i];
      if (node->type ==
            ROUTE_NODE_TYPE_MONITOR_FADER &&
          node->fader == fader)
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
  else if (type == ROUTE_NODE_TYPE_FADER)
    node->fader = (Fader *) data;
  else if (type == ROUTE_NODE_TYPE_MONITOR_FADER)
    node->fader = (Fader *) data;
  else if (type == ROUTE_NODE_TYPE_PREFADER)
    node->prefader = (PassthroughProcessor *) data;
  else if (type ==
             ROUTE_NODE_TYPE_SAMPLE_PROCESSOR)
    node->sample_processor =
      (SampleProcessor *) data;

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
      (size_t) ++graph->trigger_queue_size *
      sizeof (GraphNode *));
  graph->graph_nodes =
    (GraphNode **) realloc (
      graph->graph_nodes,
      (size_t) (1 + graph->n_graph_nodes) *
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
  self->terminal_nodes[
    self->terminal_node_count - 1] = node;
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
      (size_t) (1 + self->n_init_triggers) *
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

/**
 * Frees the graph and its members.
 */
void
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
	for (int i = 0; i < tc; ++i)
    {
      /*g_message ("posting trigger to wake up "*/
                 /*"sleeping threads");*/
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

/*void*/
/*router_cleanup (*/
  /*Router * self)*/
/*{*/
  /*graph_destroy (self->graph1);*/
  /*graph_destroy (self->graph2);*/
/*}*/


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
      /*g_message ("waiting for next cycle to begin");*/
      zix_sem_wait (&graph->callback_start);

      if (graph->terminate)
        return;

      /* reset terminal reference count */
      g_atomic_int_set (
        &graph->terminal_refcnt,
        (unsigned int) graph->terminal_node_count);

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
 *
 * @param local_offset The local offset to start
 *   playing from in this cycle:
 *   (0 - <engine buffer size>)
 */
void
router_start_cycle (
  Router *         self,
  const nframes_t  nsamples,
  const nframes_t  local_offset,
  const Position * pos)
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
  self->nsamples = nsamples;
  self->global_offset =
    self->max_playback_latency -
    AUDIO_ENGINE->remaining_latency_preroll;
  self->local_offset = local_offset;
  zix_sem_post (&self->graph1->callback_start);
  /*g_message ("waiting for cycle to finish");*/
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

/**
 * Add the port to the nodes.
 */
static inline void
add_port (
  Graph * self,
  Port *   port)
{
  PortOwnerType owner =
    port->identifier.owner_type;

  /*add_port_node (self, port);*/
  if (port->num_dests == 0 &&
      port->num_srcs == 0 &&
      owner == PORT_OWNER_TYPE_PLUGIN)
    {
      if (port->identifier.flow == FLOW_INPUT)
        graph_add_initial_node (
          self, ROUTE_NODE_TYPE_PORT, port);
      else if (port->identifier.flow == FLOW_OUTPUT)
        graph_add_terminal_node (
          self, ROUTE_NODE_TYPE_PORT, port);
    }
  else if (port->num_dests == 0 &&
           port->num_srcs == 0 &&
           owner != PORT_OWNER_TYPE_PLUGIN &&
           owner != PORT_OWNER_TYPE_FADER &&
           owner != PORT_OWNER_TYPE_MONITOR_FADER &&
           owner != PORT_OWNER_TYPE_PREFADER)
    {
    }
  else if (port->num_srcs == 0 &&
      !(owner == PORT_OWNER_TYPE_PLUGIN &&
        port->identifier.flow == FLOW_OUTPUT) &&
      !(owner == PORT_OWNER_TYPE_FADER &&
        port->identifier.flow == FLOW_OUTPUT) &&
      !(owner == PORT_OWNER_TYPE_MONITOR_FADER &&
        port->identifier.flow == FLOW_OUTPUT) &&
      !(owner == PORT_OWNER_TYPE_PREFADER &&
        port->identifier.flow == FLOW_OUTPUT) &&
      !(owner ==
          PORT_OWNER_TYPE_SAMPLE_PROCESSOR &&
        port->identifier.flow == FLOW_OUTPUT))
    graph_add_initial_node (
      self, ROUTE_NODE_TYPE_PORT, port);
  else if (port->num_dests == 0 &&
           port->num_srcs > 0 &&
           !(owner == PORT_OWNER_TYPE_PLUGIN &&
             port->identifier.flow ==
               FLOW_INPUT) &&
           !(owner == PORT_OWNER_TYPE_FADER &&
             port->identifier.flow ==
               FLOW_INPUT) &&
           !(owner == PORT_OWNER_TYPE_MONITOR_FADER &&
             port->identifier.flow ==
               FLOW_INPUT) &&
           !(owner == PORT_OWNER_TYPE_PREFADER &&
             port->identifier.flow ==
               FLOW_INPUT) &&
           !(owner ==
               PORT_OWNER_TYPE_SAMPLE_PROCESSOR &&
             port->identifier.flow == FLOW_INPUT))
    graph_add_terminal_node (
      self, ROUTE_NODE_TYPE_PORT, port);
  else
    graph_add_node (
      self, ROUTE_NODE_TYPE_PORT, port);
}

/**
 * Connect the port as a node.
 */
static inline void
connect_port (
  Graph * self,
  Port * port)
{
  GraphNode * node =
    find_node_from_port (self, port);
  GraphNode * node2;
  Port * src, * dest;
  for (int j = 0; j < port->num_srcs; j++)
    {
      src = port->srcs[j];
      node2 = find_node_from_port (self, src);
      g_warn_if_fail (node);
      g_warn_if_fail (node2);
      node_connect (node2, node);
    }
  for (int j = 0; j < port->num_dests; j++)
    {
      dest = port->dests[j];
      node2 = find_node_from_port (self, dest);
      g_warn_if_fail (node);
      g_warn_if_fail (node2);
      node_connect (node, node2);
    }
}

/**
 * Returns the latency of only the given port,
 * without adding the previous/next latencies.
 *
 * It returns the plugin's latency if plugin,
 * otherwise 0.
 */
static nframes_t
get_node_single_playback_latency (
  GraphNode * node)
{
  if (node->type == ROUTE_NODE_TYPE_PLUGIN)
    {
      /* latency is already set at this point */
      return node->pl->latency;
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
static void
set_node_playback_latency (
  GraphNode * node,
  nframes_t   dest_latency)
{
  /*long max_latency = 0, parent_latency;*/

  /* set this node's latency */
  node->playback_latency =
    dest_latency +
    get_node_single_playback_latency (node);

  /* set route playback latency if trigger node */
  if (node->initial)
    node->route_playback_latency =
      node->playback_latency;

  GraphNode * parent;
  for (int i = 0; i < node->init_refcount; i++)
    {
      parent = node->parentnodes[i];
      set_node_playback_latency (
        parent, node->playback_latency);
    }
}

/**
 * Checks for cycles in the graph.
 *
 * @return 1 if valid, 0 if invalid.
 */
static int
graph_is_valid (
  Graph * self)
{
  int i;

  /* empty list to put sorted elements */
  /*GraphNode * sorted_nodes[self->n_graph_nodes];*/
  /*int num_sorted_nodes = 0;*/

  /* set of all nodes without incoming edges */
  /*GraphNode * trigger_nodes[self->n_init_triggers];*/
  /*int num_trigger_nodes = 0;*/
  /*for (i = 0; i < self->n_init_triggers; i++)*/
    /*{*/
      /*trigger_nodes[num_trigger_nodes++] =*/
        /*self->init_trigger_list[i];*/
    /*}*/

  GraphNode * n, * m;
  while (self->n_init_triggers > 0)
    {
      n =
        self->init_trigger_list[
          --self->n_init_triggers];
      /*g_message ("init trigger %d %s",*/
                 /*self->n_init_triggers,*/
                 /*get_node_name (n));*/
      /*sorted_nodes[num_sorted_nodes++] = n;*/
      for (i = n->n_childnodes - 1;
           i >= 0; i--)
        {
          m = n->childnodes[i];
          /*g_message ("child node %d %s",*/
                     /*i,*/
                     /*get_node_name (m));*/
          n->n_childnodes--;
          m->init_refcount--;
          if (m->init_refcount == 0)
            {
              self->init_trigger_list[
                self->n_init_triggers++] = m;
            }
        }
    }

  for (i = 0; i < self->n_graph_nodes; i++)
    {
      n = self->graph_nodes[i];
      if (n->n_childnodes > 0 ||
          n->init_refcount > 0)
        {
          graph_print (self);
          /*g_warning ("graph invalid");*/
          return 0;
        }
    }

  return 1;
}

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
  const Port *   src,
  const Port *   dest)
{
  int i, j, k;
  GraphNode * node, * node2;
  Graph * self = calloc (1, sizeof (Graph));
  g_warn_if_fail (router);
  self->router = router;

  /* ========================
   * first add all the nodes
   * ======================== */

  /* add the sample processor */
  graph_add_initial_node ( \
    self, ROUTE_NODE_TYPE_SAMPLE_PROCESSOR,
    SAMPLE_PROCESSOR);

  /* add the monitor fader */
  graph_add_node ( \
    self, ROUTE_NODE_TYPE_MONITOR_FADER,
    MONITOR_FADER);

  /* add plugins */
  Track * tr;
  Plugin * pl;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      tr = TRACKLIST->tracks[i];
      g_warn_if_fail (tr);
      if (!tr->channel)
        continue;

      /* add the track */
      /*graph_add_node ( \*/
        /*self, ROUTE_NODE_TYPE_TRACK,*/
        /*&tr);*/

      /* add the fader */
      graph_add_node ( \
        self, ROUTE_NODE_TYPE_FADER,
        &tr->channel->fader);

      /* add the prefader */
      graph_add_node ( \
        self, ROUTE_NODE_TYPE_PREFADER,
        &tr->channel->prefader);

#define ADD_PLUGIN \
          if (!pl || pl->deleting) \
            continue; \
 \
          if (pl->num_in_ports == 0 && \
              pl->num_out_ports > 0) \
            graph_add_initial_node ( \
              self, ROUTE_NODE_TYPE_PLUGIN, pl); \
          else if (pl->num_out_ports == 0 && \
                   pl->num_in_ports > 0) \
            graph_add_terminal_node ( \
              self, ROUTE_NODE_TYPE_PLUGIN, pl); \
          else if (pl->num_out_ports == 0 && \
                   pl->num_in_ports == 0) \
            { \
            } \
          else \
            graph_add_node ( \
              self, ROUTE_NODE_TYPE_PLUGIN, pl)

      for (j = 0; j < STRIP_SIZE; j++)
        {
          pl = tr->channel->plugins[j];

          ADD_PLUGIN;

          plugin_update_latency (pl);
        }
      for (j = 0; j < tr->num_modulators; j++)
        {
          pl = tr->modulators[j]->plugin;

          ADD_PLUGIN;

          plugin_update_latency (pl);
        }
    }
#undef ADD_PLUGIN

  /* add ports */
  Port * ports[10000];
  int num_ports;
  Port * port;
  port_get_all (ports, &num_ports);
  for (i = 0; i < num_ports; i++)
    {
      port = ports[i];
      g_warn_if_fail (port);
      if (port->deleting ||
          (port->plugin &&
           port->plugin->deleting))
        continue;

      add_port (
        self, port);
    }

  /* ========================
   * now connect them
   * ======================== */

  /* connect the sample processor */
  node =
    find_node_from_sample_processor (
      self, SAMPLE_PROCESSOR);
  port = SAMPLE_PROCESSOR->stereo_out->l;
  node2 =
    find_node_from_port (self, port);
  node_connect (node, node2);
  port = SAMPLE_PROCESSOR->stereo_out->r;
  node2 =
    find_node_from_port (self, port);
  node_connect (node, node2);

  /* connect the monitor fader */
  node =
    find_node_from_monitor_fader (
      self, MONITOR_FADER);
  port = MONITOR_FADER->stereo_in->l;
  node2 =
    find_node_from_port (self, port);
  node_connect (node2, node);
  port = MONITOR_FADER->stereo_in->r;
  node2 =
    find_node_from_port (self, port);
  node_connect (node2, node);
  port = MONITOR_FADER->stereo_out->l;
  node2 =
    find_node_from_port (self, port);
  node_connect (node, node2);
  port = MONITOR_FADER->stereo_out->r;
  node2 =
    find_node_from_port (self, port);
  node_connect (node, node2);

  Fader * fader;
  PassthroughProcessor * prefader;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      tr = TRACKLIST->tracks[i];
      if (!tr->channel)
        continue;

      fader = &tr->channel->fader;
      prefader = &tr->channel->prefader;

      /* connect the fader */
      node =
        find_node_from_fader (
          self, fader);
      if (fader->type == FADER_TYPE_AUDIO_CHANNEL)
        {
          port = fader->stereo_in->l;
          node2 =
            find_node_from_port (self, port);
          node_connect (node2, node);
          port = fader->stereo_in->r;
          node2 =
            find_node_from_port (self, port);
          node_connect (node2, node);
          port = fader->stereo_out->l;
          node2 =
            find_node_from_port (self, port);
          node_connect (node, node2);
          port = fader->stereo_out->r;
          node2 =
            find_node_from_port (self, port);
          node_connect (node, node2);
        }
      else if (fader->type ==
                 FADER_TYPE_MIDI_CHANNEL)
        {
          port = fader->midi_in;
          node2 =
            find_node_from_port (self, port);
          node_connect (node2, node);
          port = fader->midi_out;
          node2 =
            find_node_from_port (self, port);
          node_connect (node, node2);
        }

      /* connect the prefader */
      node =
        find_node_from_prefader (
          self, prefader);
      if (prefader->type ==
            PP_TYPE_AUDIO_CHANNEL)
        {
          port = prefader->stereo_in->l;
          node2 =
            find_node_from_port (self, port);
          node_connect (node2, node);
          port = prefader->stereo_in->r;
          node2 =
            find_node_from_port (self, port);
          node_connect (node2, node);
          port = prefader->stereo_out->l;
          node2 =
            find_node_from_port (self, port);
          node_connect (node, node2);
          port = prefader->stereo_out->r;
          node2 =
            find_node_from_port (self, port);
          node_connect (node, node2);
        }
      else if (prefader->type ==
                 PP_TYPE_MIDI_CHANNEL)
        {
          port = prefader->midi_in;
          node2 =
            find_node_from_port (self, port);
          node_connect (node2, node);
          port = prefader->midi_out;
          node2 =
            find_node_from_port (self, port);
          node_connect (node, node2);
        }

#define CONNECT_PLUGIN \
          if (!pl || pl->deleting) \
            continue; \
 \
          node = \
            find_node_from_plugin (self, pl); \
          for (k = 0; k < pl->num_in_ports; k++) \
            { \
              port = pl->in_ports[k]; \
              g_warn_if_fail ( \
                port->plugin != NULL); \
              node2 = \
                find_node_from_port (self, port); \
              node_connect (node2, node); \
            } \
          for (k = 0; k < pl->num_out_ports; k++) \
            { \
              port = pl->out_ports[k]; \
              g_warn_if_fail ( \
                port->plugin != NULL); \
              node2 = \
                find_node_from_port (self, port); \
              node_connect (node, node2); \
            }

      for (j = 0; j < STRIP_SIZE; j++)
        {
          pl = tr->channel->plugins[j];

          CONNECT_PLUGIN;
        }

      for (j = 0; j < tr->num_modulators; j++)
        {
          pl = tr->modulators[j]->plugin;

          CONNECT_PLUGIN;
        }
    }
#undef CONNECT_PLUGIN

  for (i = 0; i < num_ports; i++)
    {
      port = ports[i];
      g_warn_if_fail (port);
      if (port->deleting ||
          (port->plugin &&
           port->plugin->deleting))
        continue;

      connect_port (
        self, port);
    }

  /* connect the src/dest if not NULL */
  /* this code is only for creating graphs to test
   * if the connection between src->dest is valid */
  if (src && dest)
    {
      node =
        find_node_from_port (self, src);
      node2 =
        find_node_from_port (self, dest);
      node_connect (node, node2);

      if (graph_is_valid (self))
        return self;
      else
        {
          graph_free (self);
          return NULL;
        }
    }

  /* ========================
   * calculate latencies of each port and each
   * processor
   * ======================== */

  for (i = 0; i < self->terminal_node_count; i++)
    {
      set_node_playback_latency (
        self->terminal_nodes[i], 0);
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
  self->graph1 = graph_new (self, NULL, NULL);

  self->graph2 = NULL;
}
