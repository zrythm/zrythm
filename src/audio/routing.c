/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "audio/track_processor.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/env.h"
#include "utils/mpmc_queue.h"
#include "utils/objects.h"
#include "utils/stoat.h"

#ifdef HAVE_JACK
#include <jack/thread.h>
#endif

#define MAGIC_NUMBER 76322

static inline void
graph_reached_terminal_node (Graph* self);

/* called from a node when all its incoming edges
 * have completed processing and the node can run.
 * It is added to the "work queue" */
static inline void
graph_trigger (
  Graph *     self,
  GraphNode * n)
{
  g_atomic_int_inc (&self->trigger_queue_size);
  /*g_message ("triggering node, pushing back");*/
  mpmc_queue_push_back_node (
    self->trigger_queue, n);
}

/**
 * Called by an upstream node when it has completed
 * processing.
 */
static inline void
node_trigger (
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
      return
        g_strdup_printf (
          "%s Fader",
          node->fader->channel->track->name);
    case ROUTE_NODE_TYPE_TRACK:
      return
        g_strdup (
          node->track->name);
    case ROUTE_NODE_TYPE_PREFADER:
      return
        g_strdup_printf (
          "%s Pre-Fader",
          node->prefader->channel->track->name);
    case ROUTE_NODE_TYPE_MONITOR_FADER:
      return
        g_strdup ("Monitor Fader");
    case ROUTE_NODE_TYPE_SAMPLE_PROCESSOR:
      return
        g_strdup ("Sample Processor");
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
  /*g_message ("processing %s",*/
             /*get_node_name (node));*/
  int noroll = 0;

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
  else if (node->type ==
           ROUTE_NODE_TYPE_TRACK)
    {
      track_processor_process (
        &node->track->processor,
        g_start_frames, local_offset,
        nframes);
    }
  else if (node->type == ROUTE_NODE_TYPE_PORT)
    {
      /* decide what to do based on what port it
       * is */
      Port * port = node->port;

      /* if midi editor manual press */
      if (port == AUDIO_ENGINE->
            midi_editor_manual_press)
        {
          midi_events_dequeue (
            AUDIO_ENGINE->
              midi_editor_manual_press->
                midi_events);
        }

      /* if channel stereo out port */
      else if (
        port->identifier.owner_type ==
          PORT_OWNER_TYPE_TRACK &&
        port->identifier.flow == FLOW_OUTPUT)
        {
          Track * track = port_get_track (port, 1);

          /* if muted clear it */
          if (track->mute ||
                (tracklist_has_soloed (
                  TRACKLIST) &&
                   !track->solo &&
                   track != P_MASTER_TRACK))
            {
              port_clear_buffer (port);
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
  g_return_if_fail (
    self && self->graph && self->graph->router);

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
graph_worker_thread (void * arg)
{
  GraphThread * thread = (GraphThread *) arg;
  Graph * self = thread->graph;
  GraphNode* to_run = NULL;

  g_message ("WORKER THREAD %d created",
             thread->id);

  /* wait for all threads to get created */
  if (thread->id < self->num_threads - 1)
    {
      sched_yield ();
    }

  for (;;)
    {
      to_run = NULL;

      if (g_atomic_int_get (&self->terminate))
        {
          return 0;
        }

      if (mpmc_queue_dequeue_node (
            self->trigger_queue, &to_run))
        {
          g_warn_if_fail (to_run);
          /*g_message (*/
            /*"[%d]: dequeued node (nodes left %d)",*/
            /*thread->id,*/
            /*g_atomic_int_get (*/
              /*&self->trigger_queue_size));*/
          /*print_node (to_run);*/
          /* Wake up idle threads, but at most as
           * many as there's work in the trigger
           * queue that can be processed by other
           * threads.
           * This thread as not yet decreased
           * _trigger_queue_size. */
          guint idle_cnt =
            (guint)
            g_atomic_int_get (
              &self->idle_thread_cnt);
          guint work_avail =
            (guint)
            g_atomic_int_get (
              &self->trigger_queue_size);
          guint wakeup =
            MIN (idle_cnt + 1, work_avail);
          /*g_message (*/
            /*"[%d]: idle threads %u, work avail %u",*/
            /*thread->id,*/
            /*idle_cnt, work_avail);*/

          for (guint i = 1; i < wakeup; ++i)
            zix_sem_post (&self->trigger);
        }

      while (!to_run)
        {
          /* wait for work, fall asleep */
          g_atomic_int_inc (
            &self->idle_thread_cnt);
          /*g_message (*/
            /*"[%d]: just increased idle thread count and waiting for work (idle threads %d)",*/
            /*thread->id,*/
            /*g_atomic_int_get (*/
              /*&self->idle_thread_cnt));*/
          g_warn_if_fail (
            g_atomic_int_get (
              &self->idle_thread_cnt) <=
            self->num_threads);

          zix_sem_wait (&self->trigger);

          if (g_atomic_int_get (
                &self->terminate))
            return 0;

          g_atomic_int_dec_and_test (
            &self->idle_thread_cnt);
          /*g_message (*/
            /*"[%d]: work found, decremented idle thread count back to %d and dequeing node",*/
            /*thread->id,*/
            /*g_atomic_int_get (*/
              /*&self->idle_thread_cnt));*/

          /* try to find some work to do */
          mpmc_queue_dequeue_node (
            self->trigger_queue,
            &to_run);
        }

      /* process graph-node */
      g_atomic_int_dec_and_test (
        &self->trigger_queue_size);
      /*g_message ("[%d]: running node",*/
                 /*thread->id);*/
      node_run (to_run);

    }
  return 0;
}

REALTIME
static void *
graph_main_thread (void * arg)
{
  GraphThread * thread = (GraphThread *) arg;
  Graph * self = thread->graph;

  /* Wait until all worker threads are active */
  while (
    g_atomic_int_get (&self->idle_thread_cnt) !=
      self->num_threads)
    sched_yield ();

  /* wait for initial process callback */
  zix_sem_wait (&self->callback_start);

  /* first time setup */

  if (!self->destroying)
    {
      /* Can't run without a graph */
      g_warn_if_fail (self->n_graph_nodes > 0);
      g_warn_if_fail (self->n_init_triggers > 0);
      g_warn_if_fail (self->n_terminal_nodes > 0);
    }

  /* bootstrap trigger-list.
   * (later this is done by
   * Graph_reached_terminal_node)*/
  for (size_t i = 0;
       i < self->n_init_triggers; ++i)
    {
      g_atomic_int_inc (&self->trigger_queue_size);
      /*g_message ("[main] pushing back node %d during bootstrap", i);*/
      mpmc_queue_push_back_node (
        self->trigger_queue,
        self->init_trigger_list[i]);
    }

  /* after setup, the main-thread just becomes
   * a normal worker */
  return graph_worker_thread (thread);
}

/**
 * Tell all threads to terminate.
 */
static void
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
  for (int i = 0; i < self->num_threads; i++)
    {
#ifdef HAVE_JACK_CLIENT_STOP_THREAD
      jack_client_stop_thread (
        AUDIO_ENGINE->client,
        self->threads[i].jthread);
#else
      pthread_join (
        self->threads[i].jthread, NULL);
#endif
    }
#ifdef HAVE_JACK_CLIENT_STOP_THREAD
  jack_client_stop_thread (
    AUDIO_ENGINE->client,
    self->main_thread.jthread);
#else
  pthread_join (
    self->main_thread.jthread, NULL);
#endif
#else
  for (int i = 0; i < self->num_threads; i++)
    {
      pthread_join (
        self->threads[i].pthread, NULL);
    }
  pthread_join (self->main_thread.pthread, NULL);
#endif

  /*self->init_trigger_list = NULL;*/
  /*self->n_init_triggers   = 0;*/
  /*self->trigger_queue     = NULL;*/
  /*free (self->init_trigger_list);*/
  /*free (self->trigger_queue);*/
}

/**
 * Creates a thread.
 *
 * @param self The GraphThread to fill in.
 * @param id The index of the thread.
 * @param graph The graph to set to the thread.
 * @param is_main 1 if main thread.
 *
 * @return 1 if created, 0 if error.
 */
static int
create_thread (
  GraphThread * self,
  const int     id,
  const int     is_main,
  Graph *       graph)
{
  self->id = id;
  self->graph = graph;

#ifdef HAVE_JACK
  if (AUDIO_ENGINE->audio_backend ==
        AUDIO_BACKEND_JACK)
    {
      jack_client_create_thread (
        AUDIO_ENGINE->client,
        &self->jthread,
        jack_client_real_time_priority (
          AUDIO_ENGINE->client),
        jack_is_realtime (
          AUDIO_ENGINE->client),
        is_main ?
          graph_main_thread :
          graph_worker_thread,
        self);

      g_return_val_if_fail (
        (int) self->jthread != -1, 0);
    }
  else
    {
#endif
      if (pthread_create (
          &self->pthread, NULL,
          is_main ?
            &graph_main_thread :
            &graph_worker_thread,
          self))
        {
          g_return_val_if_reached (0);
        }
#ifdef HAVE_JACK
    }
#endif

  return 1;
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
  int num_cores = audio_get_num_cores ();
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
  int ret;
  for (int i = 0; i < graph->num_threads; i++)
    {
      ret =
        create_thread (
          &graph->threads[i], i, 0, graph);
      if (!ret)
        {
          graph_terminate (graph);
          return 0;
        }
    }

  /* and the main thread */
  ret =
    create_thread (
      &graph->main_thread, -1, 1, graph);
  if (!ret)
    {
      graph_terminate (graph);
      return 0;
    }

  /* breathe */
  sched_yield ();
  return 1;
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
  for (size_t i = 0;
       i < graph->n_init_triggers; i++)
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
      router->graph);

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

static GraphNode *
find_node_from_plugin (
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

static GraphNode *
find_node_from_track (
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

static GraphNode *
find_node_from_fader (
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

static GraphNode *
find_node_from_prefader (
  Graph * graph,
  PassthroughProcessor * prefader)
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

static GraphNode *
find_node_from_sample_processor (
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

static GraphNode *
find_node_from_monitor_fader (
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

static inline GraphNode *
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
      node->prefader =
        (PassthroughProcessor *) data;
      break;
    case ROUTE_NODE_TYPE_SAMPLE_PROCESSOR:
      node->sample_processor =
        (SampleProcessor *) data;
      break;
    case ROUTE_NODE_TYPE_TRACK:
      node->track = (Track *) data;
      break;
    }

  return node;
}

static inline GraphNode *
graph_add_node (
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

static inline GraphNode *
graph_add_terminal_node (
  Graph * self,
  GraphNodeType type,
  void * data)
{
  GraphNode * node =
    graph_add_node (self, type, data);
  node->terminal = 1;

  /* used for calculating latencies */
  self->setup_terminal_nodes =
    (GraphNode **) realloc (
      self->setup_terminal_nodes,
      (size_t)
      (1 + self->num_setup_terminal_nodes) *
        sizeof (GraphNode *));
  self->setup_terminal_nodes[
    self->num_setup_terminal_nodes++] = node;

  return node;
}

static inline GraphNode *
graph_add_initial_node (
  Graph * self,
  GraphNodeType type,
  void * data)
{
  self->setup_init_trigger_list =
    (GraphNode**)realloc (
      self->setup_init_trigger_list,
      (size_t) (1 + self->num_setup_init_triggers) *
        sizeof (GraphNode*));

  GraphNode * node =
    graph_add_node (self, type, data);
  node->initial = 1;

  self->setup_init_trigger_list[
    self->num_setup_init_triggers++] = node;

  return node;
}

static inline void
node_free (
  GraphNode * node)
{
  free (node->childnodes);
  free (node->parentnodes);
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
  free (self->init_trigger_list);
  free (self->setup_graph_nodes);
  free (self->setup_init_trigger_list);

  zix_sem_destroy (&self->callback_start);
  zix_sem_destroy (&self->callback_done);
  zix_sem_destroy (&self->trigger);
}

void
graph_destroy (
  Graph * self)
{
  g_message (
    "destroying graph %p (router g1 %p g2 %p)",
    self, self->router->graph,
    self->router->graph);
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

/* called from a terminal node (from the Graph worked-thread)
 * to indicate it has completed processing.
 *
 * The thread of the last terminal node that reaches here
 * will inform the main-thread, wait, and kick off the next process cycle.
 */
static inline void
graph_reached_terminal_node (
  Graph *  self)
{
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
  if (!zix_sem_try_wait (&self->graph_access))
    return;

  self->nsamples = nsamples;
  self->global_offset =
    self->max_playback_latency -
    AUDIO_ENGINE->remaining_latency_preroll;
  self->local_offset = local_offset;
  zix_sem_post (&self->graph->callback_start);
  zix_sem_wait (&self->graph->callback_done);

  zix_sem_post (&self->graph_access);
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
      print_node (self->setup_graph_nodes[i]);
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

/**
 * Add the port to the nodes.
 *
 * @param drop_if_unnecessary Drops the port
 *   if it doesn't connect anywhere.
 */
static inline void
add_port (
  Graph *      self,
  Port * port,
  const int    drop_if_unnecessary)
{
  PortOwnerType owner =
    port->identifier.owner_type;

  /*add_port_node (self, port);*/
  if (port->num_dests == 0 &&
      port->num_srcs == 0 &&
      (owner == PORT_OWNER_TYPE_PLUGIN ||
       owner == PORT_OWNER_TYPE_FADER))
    {
      if (port->identifier.flow == FLOW_INPUT)
        {
          graph_add_initial_node (
            self, ROUTE_NODE_TYPE_PORT, port);
        }
      else if (port->identifier.flow == FLOW_OUTPUT)
        {
          graph_add_terminal_node (
            self, ROUTE_NODE_TYPE_PORT, port);
        }
    }
  /* drop ports without sources and dests */
  else if (
    drop_if_unnecessary &&
    port->num_dests == 0 &&
    port->num_srcs == 0 &&
    owner != PORT_OWNER_TYPE_PLUGIN &&
    owner != PORT_OWNER_TYPE_FADER &&
    owner != PORT_OWNER_TYPE_MONITOR_FADER &&
    owner != PORT_OWNER_TYPE_PREFADER &&
    owner != PORT_OWNER_TYPE_TRACK_PROCESSOR)
    {
    }
  else if (port->num_srcs == 0 &&
      !(owner == PORT_OWNER_TYPE_PLUGIN &&
        port->identifier.flow == FLOW_OUTPUT) &&
      !(owner == PORT_OWNER_TYPE_TRACK_PROCESSOR &&
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
    {
      graph_add_initial_node (
        self, ROUTE_NODE_TYPE_PORT, port);
    }
  else if (port->num_dests == 0 &&
           /*port->num_srcs > 0 &&*/
           !(owner == PORT_OWNER_TYPE_PLUGIN &&
             port->identifier.flow ==
               FLOW_INPUT) &&
           !(owner == PORT_OWNER_TYPE_TRACK_PROCESSOR &&
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
    {
      graph_add_terminal_node (
        self, ROUTE_NODE_TYPE_PORT, port);
    }
  else
    {
      graph_add_node (
        self, ROUTE_NODE_TYPE_PORT, port);
    }
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
graph_clear_setup (
  Graph * self)
{
  for (size_t i = 0;
       i < self->num_setup_graph_nodes; i++)
    {
      free_later (
        self->setup_graph_nodes[i], free);
    }
  self->num_setup_graph_nodes = 0;
  self->num_setup_init_triggers = 0;
  self->num_setup_terminal_nodes = 0;
}

static void
graph_rechain (
  Graph * self)
{
  g_warn_if_fail (
    g_atomic_int_get (
      &self->terminal_refcnt) == 0);
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

  graph_clear_setup (self);
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
  int i, j, k;
  GraphNode * node, * node2;

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
      graph_add_node ( \
        self, ROUTE_NODE_TYPE_TRACK,
        tr);

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
      if (port->deleting)
        continue;
      if (port->identifier.owner_type ==
            PORT_OWNER_TYPE_PLUGIN)
        {
          Plugin * port_pl =
            port_get_plugin (port, 1);
          if (port_pl->deleting)
            continue;
        }

      add_port (
        self, port, drop_unnecessary_ports);
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

  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      tr = TRACKLIST->tracks[i];
      if (!tr->channel)
        continue;

      /* connect the track */
      node =
        find_node_from_track (
          self, tr);
      g_warn_if_fail (node);
      if (tr->in_signal_type == TYPE_AUDIO)
        {
          port = tr->processor.stereo_in->l;
          node2 =
            find_node_from_port (self, port);
          node_connect (node2, node);
          port = tr->processor.stereo_in->r;
          node2 =
            find_node_from_port (self, port);
          node_connect (node2, node);
          port = tr->processor.stereo_out->l;
          node2 =
            find_node_from_port (self, port);
          node_connect (node, node2);
          port = tr->processor.stereo_out->r;
          node2 =
            find_node_from_port (self, port);
          node_connect (node, node2);
        }
      else if (tr->in_signal_type == TYPE_EVENT)
        {
          port = tr->processor.midi_in;
          node2 =
            find_node_from_port (self, port);
          node_connect (node2, node);
          port = tr->processor.midi_out;
          node2 =
            find_node_from_port (self, port);
          node_connect (node, node2);
        }

      Fader * fader;
      PassthroughProcessor * prefader;
      fader = &tr->channel->fader;
      prefader = &tr->channel->prefader;

      /* connect the fader */
      node =
        find_node_from_fader (
          self, fader);
      g_warn_if_fail (node);
      if (fader->type == FADER_TYPE_AUDIO_CHANNEL)
        {
          /* connect ins */
          port = fader->stereo_in->l;
          node2 =
            find_node_from_port (self, port);
          node_connect (node2, node);
          port = fader->stereo_in->r;
          node2 =
            find_node_from_port (self, port);
          node_connect (node2, node);

          /* connect outs */
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
      port = fader->amp;
      node2 =
        find_node_from_port (self, port);
      node_connect (node2, node);
      port = fader->pan;
      node2 =
        find_node_from_port (self, port);
      node_connect (node2, node);

      /* connect the prefader */
      node =
        find_node_from_prefader (
          self, prefader);
      g_warn_if_fail (node);
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
          g_warn_if_fail (node); \
          for (k = 0; k < pl->num_in_ports; k++) \
            { \
              port = pl->in_ports[k]; \
              g_warn_if_fail ( \
                port_get_plugin (port, 1) != NULL); \
              node2 = \
                find_node_from_port (self, port); \
              node_connect (node2, node); \
            } \
          for (k = 0; k < pl->num_out_ports; k++) \
            { \
              port = pl->out_ports[k]; \
              g_warn_if_fail ( \
                port_get_plugin (port, 1) != NULL); \
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
      if (port->deleting)
        continue;
      if (port->identifier.owner_type ==
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
   * calculate latencies of each port and each
   * processor
   * ======================== */

  for (size_t ii = 0;
       ii < self->num_setup_terminal_nodes; ii++)
    {
      set_node_playback_latency (
        self->setup_terminal_nodes[ii], 0);
    }

  graph_print (self);

  if (rechain)
    graph_rechain (self);
}

/**
 * Adds a new connection for the given
 * src and dest ports and validates the graph.
 *
 * @return 1 for ok, 0 for invalid.
 */
int
graph_validate (
  Router *     router,
  const Port * src,
  const Port * dest)
{
  g_return_val_if_fail (src && dest, 0);
  Graph * self = graph_new (router);

  graph_setup (self, 0, 0);

  /* connect the src/dest if not NULL */
  /* this code is only for creating graphs to test
   * if the connection between src->dest is valid */
  GraphNode * node, * node2;
  node =
    find_node_from_port (self, src);
  node2 =
    find_node_from_port (self, dest);
  node_connect (node, node2);

  int valid = graph_is_valid (self);

  graph_free (self);

  return valid;
}

/**
 * Returns a new graph.
 */
Graph *
graph_new (
  Router * router)
{
  Graph * self = calloc (1, sizeof (Graph));
  g_warn_if_fail (router);
  self->router = router;
  self->trigger_queue =
    mpmc_queue_new ();
  self->init_trigger_list =
    calloc (1, sizeof (GraphNode *));
  self->graph_nodes =
    calloc (1, sizeof (GraphNode *));

  zix_sem_init (&self->callback_start, 0);
  zix_sem_init (&self->callback_done, 0);
  zix_sem_init (&self->trigger, 0);

  g_atomic_int_set (&self->terminal_refcnt, 0);
  g_atomic_int_set (&self->terminate, 0);
  g_atomic_int_set (&self->idle_thread_cnt, 0);
  g_atomic_int_set (&self->trigger_queue_size, 0);

  return self;
}

void
router_init (
  Router * self)
{
  zix_sem_init (&self->graph_access, 1);

  self->graph = NULL;
}

/**
 * Returns if the current thread is a
 * processing thread.
 */
int
router_is_processing_thread (
  Router * self)
{
  for (int j = 0;
       j < self->graph->num_threads; j++)
    {
#ifdef HAVE_JACK
      if (AUDIO_ENGINE->audio_backend ==
            AUDIO_BACKEND_JACK)
        {
          if (pthread_equal (
                pthread_self (),
                self->graph->threads[j].jthread))
            return 1;
        }
#endif
#ifndef HAVE_JACK
      if (pthread_equal (
            pthread_self (),
            self->graph->threads[j].pthread))
        return 1;
#endif
    }

#ifdef HAVE_JACK
  if (AUDIO_ENGINE->audio_backend ==
        AUDIO_BACKEND_JACK)
    {
      if (pthread_equal (
            pthread_self (),
            self->graph->main_thread.jthread))
        return 1;
    }
#endif
#ifndef HAVE_JACK
  if (pthread_equal (
        pthread_self (),
        self->graph->main_thread.pthread))
    return 1;
#endif

  return 0;
}
