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

#include "zrythm-config.h"

#include "audio/engine.h"
#include "audio/graph.h"
#include "audio/graph_node.h"
#include "audio/graph_thread.h"
#include "audio/router.h"
#include "project.h"
#include "utils/mpmc_queue.h"
#include "utils/objects.h"

/* uncomment to show debug messages */
/*#define DEBUG_THREADS 1*/

static void *
worker_thread (void * arg)
{
  GraphThread * thread = (GraphThread *) arg;
  Graph * graph = thread->graph;
  GraphNode* to_run = NULL;

  g_message (
    "WORKER THREAD %d created (num threads %d)",
    thread->id, graph->num_threads);

  /* wait for all threads to get created */
  if (thread->id < graph->num_threads - 1)
    {
      sched_yield ();
    }

  for (;;)
    {
      to_run = NULL;

      if (g_atomic_int_get (&graph->terminate))
        {
          g_message (
            "[%d]: terminating thread",
            thread->id);
          return 0;
        }

      if (mpmc_queue_dequeue_node (
            graph->trigger_queue, &to_run))
        {
          g_warn_if_fail (to_run);
#ifdef DEBUG_THREADS
          g_message (
            "[%d]: dequeued node (nodes left %d)",
            thread->id,
            g_atomic_int_get (
              &graph->trigger_queue_size));
          graph_node_print (to_run);
#endif
          /* Wake up idle threads, but at most as
           * many as there's work in the trigger
           * queue that can be processed by other
           * threads.
           * This thread as not yet decreased
           * _trigger_queue_size. */
          guint idle_cnt =
            (guint)
            g_atomic_int_get (
              &graph->idle_thread_cnt);
          guint work_avail =
            (guint)
            g_atomic_int_get (
              &graph->trigger_queue_size);
          guint wakeup =
            MIN (idle_cnt + 1, work_avail);
#ifdef DEBUG_THREADS
          g_message (
            "[%d]: Waking up %u idle threads (idle count %u), work available -> %u",
            thread->id, wakeup - 1,
            idle_cnt, work_avail);
#endif

          for (guint i = 1; i < wakeup; ++i)
            {
              zix_sem_post (&graph->trigger);
            }
        }

      while (!to_run)
        {
          /* wait for work, fall asleep */
          g_atomic_int_inc (
            &graph->idle_thread_cnt);
          int idle_thread_cnt =
            g_atomic_int_get (
              &graph->idle_thread_cnt);
#ifdef DEBUG_THREADS
          g_message (
            "[%d]: no node to run. just increased "
            "idle thread count "
            "and waiting for work "
            "(current idle threads %d)",
            thread->id, idle_thread_cnt);
#endif
          if (idle_thread_cnt > graph->num_threads)
            {
              g_critical (
                "[%d]: idle thread count %d is "
                "greater than the number of threads "
                "%d. this should never occur",
                thread->id, idle_thread_cnt,
                graph->num_threads);
            }

          zix_sem_wait (&graph->trigger);

          if (g_atomic_int_get (&graph->terminate))
            return 0;

          g_atomic_int_dec_and_test (
            &graph->idle_thread_cnt);
#ifdef DEBUG_THREADS
          g_message (
            "[%d]: work found, decremented idle "
            "thread count (current count %d) and "
            "dequeing node to process",
            thread->id,
            g_atomic_int_get (
              &graph->idle_thread_cnt));
#endif

          /* try to find some work to do */
          mpmc_queue_dequeue_node (
            graph->trigger_queue,
            &to_run);
        }

      /* process graph-node */
      g_atomic_int_dec_and_test (
        &graph->trigger_queue_size);
#ifdef DEBUG_THREADS
      g_message ("[%d]: running node", thread->id);
#endif
      graph_node_process (
        to_run, graph->router->nsamples);

    }
  return 0;
}

REALTIME
static void *
main_thread (void * arg)
{
  GraphThread * thread = (GraphThread *) arg;
  Graph * self = thread->graph;

  /* Wait until all worker threads are active */
  while (
    g_atomic_int_get (&self->idle_thread_cnt) !=
      self->num_threads)
    {
      sched_yield ();
    }

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
  return worker_thread (thread);
}

/**
 * Creates a thread.
 *
 * @param id The index of the thread.
 * @param graph The graph to set to the thread.
 * @param is_main 1 if main thread.
 */
GraphThread *
graph_thread_new (
  const int  id,
  const bool is_main,
  Graph *    graph)
{
  g_return_val_if_fail (graph, NULL);

  GraphThread * self = object_new (GraphThread);

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
          main_thread :
          worker_thread,
        self);

      g_return_val_if_fail (
        (int) self->jthread != -1, NULL);
    }
  else
    {
#endif
      if (pthread_create (
          &self->pthread, NULL,
          is_main ?
            &main_thread :
            &worker_thread,
          self))
        {
          g_return_val_if_reached (NULL);
        }
#ifdef HAVE_JACK
    }
#endif

  return self;
}
