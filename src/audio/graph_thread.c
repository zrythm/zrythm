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
 * permission notices:
 *
 * Copyright (C) 2017, 2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2002-2015 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2007-2009 David Robillard <d@drobilla.net>
 * Copyright (C) 2015-2018 Robin Gareus <robin@gareus.org>
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
Copyright (C) 2001 Paul Davis
Copyright (C) 2004-2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "zrythm-config.h"

#ifndef _WOE32
#include <sys/resource.h>
#endif

#if !defined _WOE32 && defined __GLIBC__
#include <limits.h>
#include <dlfcn.h>
#endif

#include "audio/engine.h"
#include "audio/graph.h"
#include "audio/graph_node.h"
#include "audio/graph_thread.h"
#include "audio/router.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/mpmc_queue.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#ifdef HAVE_JACK
#include "weak_libjack.h"
#endif

/* uncomment to show debug messages */
/*#define DEBUG_THREADS 1*/

OPTIMIZE (O3)
static void *
worker_thread (void * arg)
{
  GraphThread * thread = (GraphThread *) arg;
  Graph * graph = thread->graph;
  GraphNode* to_run = NULL;

  /* initialize data for g_thread_self so no
   * allocation is done later on */
  g_thread_self ();

  g_message (
    "WORKER THREAD %d created (num threads %d)",
    thread->id, graph->num_threads);

  /* wait for all threads to get created */
  if (thread->id < graph->num_threads - 1)
    {
      sched_yield ();
    }

#ifdef HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp_dsp_start (&thread->lsp_ctx);
    }
#endif

  for (;;)
    {
      to_run = NULL;

      if (g_atomic_int_get (&graph->terminate))
        {
          if (thread->id == -1)
            {
              g_message ("terminating main thread");
            }
          else
            {
              g_message (
                "[%d]: terminating thread",
                thread->id);
            }
          goto terminate_thread;
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
            {
              goto terminate_thread;
            }

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
        to_run, graph->router->time_nfo);
    }

terminate_thread:

#ifdef HAVE_LSP_DSP
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp_dsp_finish (&thread->lsp_ctx);
    }
#endif

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
      g_warn_if_fail (
        g_hash_table_size (self->graph_nodes) > 0);
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

static size_t
get_stack_size (void)
{
  size_t rv = 0;
#if !defined _WOE32 && defined __GLIBC__

  size_t pt_min_stack = 16384;

#ifdef PTHREAD_STACK_MIN
  pt_min_stack = PTHREAD_STACK_MIN;
#endif

  void* handle = dlopen (NULL, RTLD_LAZY);

  /* This function is internal (it has a GLIBC_PRIVATE) version, but
   * available via weak symbol, or dlsym, and returns
   *
   * GLRO(dl_pagesize) + __static_tls_size + PTHREAD_STACK_MIN
   */

  size_t (*__pthread_get_minstack) (const pthread_attr_t* attr) =
      (size_t (*) (const pthread_attr_t*))dlsym (handle, "__pthread_get_minstack");

  if (__pthread_get_minstack != NULL) {
    pthread_attr_t attr;
    pthread_attr_init (&attr);
    rv = __pthread_get_minstack (&attr);
    assert (rv >= pt_min_stack);
    rv -= pt_min_stack;
    pthread_attr_destroy (&attr);
  }
  dlclose (handle);
#endif
  return rv;
}

#ifndef _WOE32
/**
 * Returns the priority to use for the thread or
 * 0 if not enough permissions to set the priority.
 */
static int
get_absolute_rt_priority (
  int priority)
{
  /* POSIX requires a spread of at least 32 steps
   * between min..max */
  const int p_min =
    sched_get_priority_min (SCHED_FIFO); // Linux: 1
  const int p_max =
    sched_get_priority_max (SCHED_FIFO); // Linux: 99
  g_debug ("min %d max %d requested %d",
    p_min, p_max, priority);

  if (priority == 0)
    {
      /* use default. this should be relative to
       * audio (JACK) thread */
      priority = 7;
    }
  else if (priority > 0)
    {
      /* value relative to minium */
      priority += p_min;
    }
  else
    {
      /* value relative maximum */
      priority += p_max;
    }

  if (priority > p_max)
    {
      priority = p_max;
    }
  if (priority < p_min)
    {
      priority = p_min;
    }

  g_debug ("priority before: %d", priority);

  /* clamp to allowed limits */
  struct rlimit rl;
  if (getrlimit (RLIMIT_RTPRIO, &rl) == 0)
    {
      g_debug (
        "current rtprio limit: %lu", rl.rlim_cur);
      int cur_limit = (int) rl.rlim_cur;
      if (priority > cur_limit)
        {
          priority = cur_limit;
        }
    }

  g_debug ("priority after: %d", priority);

  return priority;
}
#endif

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

  pthread_attr_t attributes;
  pthread_attr_init (&attributes);
  int res;

  res =
    pthread_attr_setdetachstate (
      &attributes, PTHREAD_CREATE_JOINABLE);
  if (res)
    {
      g_critical (
        "Cannot request joinable thread creation "
        "for thread res = %d", res);
      return NULL;
    }

  res =
    pthread_attr_setscope (
      &attributes, PTHREAD_SCOPE_SYSTEM);
  if (res)
    {
      g_critical (
        "Cannot set scheduling scope for thread "
        "res = %d", res);
      return NULL;
    }

  bool realtime = true;

  int priority = -22; /* RT priority (from ardour) */
#ifdef HAVE_JACK
  if (AUDIO_ENGINE->audio_backend ==
        AUDIO_BACKEND_JACK)
    {
      realtime =
        jack_is_realtime (AUDIO_ENGINE->client);
      int jack_priority =
        jack_client_real_time_priority (
          AUDIO_ENGINE->client);
      g_debug ("JACK thread priority: %d",
        jack_priority);
      priority = jack_priority - 2;
      (void) priority;
    }
#endif

  if (realtime)
    {
      g_debug ("creating RT thread");
      res =
        pthread_attr_setinheritsched (
          &attributes, PTHREAD_EXPLICIT_SCHED);
      if (res)
        {
          g_critical (
            "Cannot request explicit scheduling "
            "for RT thread res = %d", res);
          return NULL;
        }

#ifndef _WOE32
      priority = get_absolute_rt_priority (priority);

      if (priority > 0)
        {
          /* this throws error on windows:
           * res = 129 */
          res =
            pthread_attr_setschedpolicy (
              &attributes, SCHED_FIFO);
          if (res)
            {
              g_critical (
                "Cannot set RR scheduling class "
                "for RT thread res = %d", res);
              return NULL;
            }
          struct sched_param rt_param;
          memset (&rt_param, 0, sizeof (rt_param));
          rt_param.sched_priority = priority;
          g_debug ("priority: %d", priority);

          res =
            pthread_attr_setschedparam (
              &attributes, &rt_param);
          if (res)
            {
              g_critical (
                "Cannot set scheduling priority for RT "
                "thread res = %d", res);
              return NULL;
            }
        }
      else if (
        is_main && ZRYTHM_HAVE_UI && !ZRYTHM_TESTING)
        {
          ui_show_message_printf (
            MAIN_WINDOW, GTK_MESSAGE_WARNING, false,
            "Your user does not have enough "
            "privileges to allow %s to set "
            "the scheduling priority of threads. "
            "Please refer to the 'Getting Started' "
            "section in the "
            "user manual for details.",
            PROGRAM_NAME);
        }
#endif
    }
  /* else if not RT thread */
  else
    {
      g_debug ("creating non RT thread");
      res =
        pthread_attr_setinheritsched (
          &attributes, PTHREAD_EXPLICIT_SCHED);
      if (res)
        {
          g_critical (
            "Cannot request explicit scheduling for "
            "non RT thread res = %d", res);
          return NULL;
        }
    }

#ifdef __APPLE__
#define THREAD_STACK_SIZE 0x80000 // 512kB
#else
#define THREAD_STACK_SIZE 0x20000 // 128kB
#endif

  res =
    pthread_attr_setstacksize (
      &attributes,
      THREAD_STACK_SIZE + get_stack_size ());
  if (res)
    {
      g_critical (
        "Cannot set thread stack size res = %d (%s)",
        res, strerror (res));
      return NULL;
    }

  res =
    pthread_create (
      &self->pthread, &attributes,
      is_main ? &main_thread : &worker_thread,
      self);
  if (res)
    {
      g_critical (
        "Cannot create thread res = %d (%s)",
        res, strerror (res));
      return NULL;
    }

  pthread_attr_destroy (&attributes);

  return self;
}
