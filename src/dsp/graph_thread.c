// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notices:
 *
 * ---
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
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 *
 * Copyright (C) 2001 Paul Davis
 * Copyright (C) 2004-2008 Grame
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * ---
 */

#include "zrythm-config.h"

#ifndef _WOE32
#  include <sys/resource.h>
#endif

#if !defined _WOE32 && defined __GLIBC__
#  include <dlfcn.h>
#  include <limits.h>
#endif

#ifdef __linux__
#  include <sys/syscall.h>
#endif

#include "dsp/engine.h"
#include "dsp/graph.h"
#include "dsp/graph_node.h"
#include "dsp/graph_thread.h"
#include "dsp/router.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/mpmc_queue.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#ifdef HAVE_JACK
#  include "weak_libjack.h"
#endif

#include <glib/gi18n.h>
#include "concurrentqueue_c.h"

/* uncomment to show debug messages */
/*#define DEBUG_THREADS 1*/

/**
 * See Ardour's Graph::run_one().
 */
OPTIMIZE (O3)
static void *
worker_thread (void * arg)
{
  GraphThread * thread = (GraphThread *) arg;
  Graph *       graph = thread->graph;

  /* initialize data for g_thread_self so no
   * allocation is done later on */
  g_thread_self ();

  /* create producer/consumer token */
  if (!thread->ptok)
    {
      thread->ptok = moodycamel_create_producer_token (graph->trigger_queue);
    }
  thread->ctok = moodycamel_create_consumer_token (graph->trigger_queue);
  g_return_val_if_fail (thread->ptok, NULL);
  g_return_val_if_fail (thread->ctok, NULL);

  g_message (
    "WORKER THREAD %d created (num threads %d)", thread->id,
    graph->num_threads);

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
  GraphNode *   to_run = NULL;

      if (g_atomic_int_get (&graph->terminate))
        {
          if (thread->id == -1)
            {
              g_message ("terminating main thread");
            }
          else
            {
              g_message (
                "[%d]: terminating thread", thread->id);
            }
          goto terminate_thread;
        }

      if (moodycamel_cq_try_dequeue_consumer (
            graph->trigger_queue, thread->ctok, (void *) &to_run))
        {
          g_warn_if_fail (to_run);
#ifdef DEBUG_THREADS
          g_message (
            "[%d]: dequeued node (nodes left %d)", thread->id,
            g_atomic_int_get (&graph->trigger_queue_size));
          /*graph_node_print (to_run);*/
#endif
          /* Wake up idle threads, but at most as many as
           * there's work in the trigger queue that can be
           * processed by other threads.
           * This thread has not yet decreased
           * trigger_queue_size. */
          /* this thread is not considered idle at this point
             (so it's not part of idle_thread_cnt) */
          guint idle_cnt = (guint) g_atomic_int_get (
            &graph->idle_thread_cnt);
          guint work_avail = (guint) g_atomic_int_get (
            &graph->trigger_queue_size);
          /* add 1 for current thread */
          guint wakeup = MIN (idle_cnt + 1, work_avail);
#ifdef DEBUG_THREADS
          g_message (
            "[%d]: Waking up %u idle threads (idle count %u), work available -> %u",
            thread->id, wakeup - 1, idle_cnt, work_avail);
#endif

          for (guint i = 1; i < wakeup; ++i)
            {
              zix_sem_post (&graph->trigger);
            }
        }
      else
        {
          to_run = NULL;/* it's not this - remove */
        }

      while (!to_run)
        {
          /* wait for work (a trigger graph node).
           * this thread is now idle so increase idle counter */
          g_atomic_int_inc (&graph->idle_thread_cnt);
#ifdef DEBUG_THREADS
          g_message (
            "[%d]: no node to run. just increased "
            "idle thread count "
            "and waiting for work "
            "(current idle threads %d)",
            thread->id, g_atomic_int_get (&graph->idle_thread_cnt));
#endif
          if (g_atomic_int_get (&graph->idle_thread_cnt) > graph->num_threads)
            {
              g_critical (
                "[%d]: idle thread count %d is "
                "greater than the number of threads "
                "%d. this should never occur",
                thread->id, g_atomic_int_get (&graph->idle_thread_cnt),
                graph->num_threads);
            }

          /* semaphore - wait for a trigger node to be ready */
          zix_sem_wait (&graph->trigger);

          if (g_atomic_int_get (&graph->terminate))
            {
              goto terminate_thread;
            }

          g_atomic_int_dec_and_test (&graph->idle_thread_cnt);
#ifdef DEBUG_THREADS
          g_message (
            "[%d]: work found, decremented idle "
            "thread count (current count %d) and "
            "dequeuing node to process",
            thread->id,
            g_atomic_int_get (&graph->idle_thread_cnt));
#endif

          /* try to find some work to do before another idle
           * thread does. if succeeded, this will break out of
           * the loop and process the node. if failed, loop
           * again and wait for another trigger */
          int ret = moodycamel_cq_try_dequeue_consumer (
                graph->trigger_queue, thread->ctok, (void *) &to_run);
          if (!ret)
            to_run = NULL;
        }

      /* process graph-node */
      g_atomic_int_dec_and_test (&graph->trigger_queue_size);
#ifdef DEBUG_THREADS
      g_message ("[%d]: running node", thread->id);
#endif
      graph_node_process (to_run, graph->router->time_nfo);
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
  Graph *       self = thread->graph;

  thread->ptok = moodycamel_create_producer_token (thread->graph->trigger_queue);
  g_return_val_if_fail (thread->ptok, NULL);

  /* Wait until all worker threads are active */
  while (
    g_atomic_int_get (&self->idle_thread_cnt)
    != self->num_threads)
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
  for (size_t i = 0; i < self->n_init_triggers; ++i)
    {
      /*g_message ("[main] pushing back node %d during bootstrap", i);*/
      g_atomic_int_inc (&self->trigger_queue_size);
      int ret = moodycamel_cq_try_enqueue_producer (
        self->trigger_queue, thread->ptok, (void *) self->init_trigger_list[i]);
      g_return_val_if_fail (ret, NULL);
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

#  ifdef PTHREAD_STACK_MIN
  pt_min_stack = PTHREAD_STACK_MIN;
#  endif

  void * handle = dlopen (NULL, RTLD_LAZY);

  /* This function is internal (it has a GLIBC_PRIVATE) version, but
   * available via weak symbol, or dlsym, and returns
   *
   * GLRO(dl_pagesize) + __static_tls_size + PTHREAD_STACK_MIN
   */

  size_t (*__pthread_get_minstack) (
    const pthread_attr_t * attr) =
    (size_t (*) (const pthread_attr_t *)) dlsym (
      handle, "__pthread_get_minstack");

  if (__pthread_get_minstack != NULL)
    {
      pthread_attr_t attr;
      pthread_attr_init (&attr);
      rv = __pthread_get_minstack (&attr);
      g_return_val_if_fail (rv >= pt_min_stack, 0);
      rv -= pt_min_stack;
      pthread_attr_destroy (&attr);
    }
  dlclose (handle);
#endif
  return rv;
}

#ifdef __linux__
/**
 * Returns the priority to use for the thread or
 * 0 if not enough permissions to set the priority.
 */
static int
get_absolute_rt_priority (int priority)
{
  /* POSIX requires a spread of at least 32 steps
   * between min..max */
  const int p_min =
    sched_get_priority_min (SCHED_FIFO); // Linux: 1
  const int p_max =
    sched_get_priority_max (SCHED_FIFO); // Linux: 99
  g_debug (
    "min %d max %d requested %d", p_min, p_max, priority);

  if (priority == 0)
    {
      /* use default. this should be relative to
       * audio (JACK) thread */
      priority = 7;
    }
  else if (priority > 0)
    {
      /* value relative to minimum */
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
      g_debug ("current rtprio limit: %lu", rl.rlim_cur);
      int cur_limit = (int) rl.rlim_cur;
      if (priority > cur_limit)
        {
          priority = cur_limit;
        }
    }

  g_debug ("priority after: %d", priority);

  return priority;
}
#endif /* __linux__ */

/**
 * Creates a thread.
 *
 * @param id The index of the thread.
 * @param graph The graph to set to the thread.
 * @param is_main 1 if main thread.
 */
GraphThread *
graph_thread_new (const int id, const bool is_main, Graph * graph)
{
  g_return_val_if_fail (graph, NULL);

  GraphThread * self = object_new (GraphThread);

  self->id = id;
  self->graph = graph;

  pthread_attr_t attributes;
  pthread_attr_init (&attributes);
  int res;

  res = pthread_attr_setdetachstate (
    &attributes, PTHREAD_CREATE_JOINABLE);
  if (res)
    {
      g_critical (
        "Cannot request joinable thread creation "
        "for thread res = %d",
        res);
      return NULL;
    }

  res =
    pthread_attr_setscope (&attributes, PTHREAD_SCOPE_SYSTEM);
  if (res)
    {
      g_critical (
        "Cannot set scheduling scope for thread "
        "res = %d",
        res);
      return NULL;
    }

#if defined(HAVE_JACK) || defined(__linux__)
  bool realtime = true;

  int priority = -22; /* RT priority (from ardour) */
#endif

#ifdef HAVE_JACK
  if (AUDIO_ENGINE->audio_backend == AUDIO_BACKEND_JACK)
    {
      realtime = jack_is_realtime (AUDIO_ENGINE->client);
      int jack_priority =
        jack_client_real_time_priority (AUDIO_ENGINE->client);
      g_debug ("JACK thread priority: %d", jack_priority);
      priority = jack_priority - 2;
      (void) priority;
    }
#endif

  res = pthread_attr_setinheritsched (
    &attributes, PTHREAD_EXPLICIT_SCHED);
  if (res)
    {
      g_critical (
        "Cannot request explicit scheduling "
        "res = %d",
        res);
      return NULL;
    }

#ifdef __linux__
  if (realtime)
    {
      g_debug ("creating RT thread");
      int wanted_priority = priority;
      (void) wanted_priority;
      int posix_priority = get_absolute_rt_priority (priority);

      /* if have enough permissions, use POSIX API */
      if (posix_priority > 0)
        {
          /* this throws error on windows:
           * res = 129 */
          res = pthread_attr_setschedpolicy (
            &attributes, SCHED_FIFO);
          if (res)
            {
              g_critical (
                "Cannot set RR scheduling class "
                "for RT thread res = %d",
                res);
              return NULL;
            }
          struct sched_param rt_param;
          memset (&rt_param, 0, sizeof (rt_param));
          rt_param.sched_priority = posix_priority;
          g_debug ("priority: %d", posix_priority);

          res = pthread_attr_setschedparam (
            &attributes, &rt_param);
          if (res)
            {
              g_critical (
                "Cannot set scheduling priority for RT "
                "thread res = %d",
                res);
              return NULL;
            }
        }
      /* else if not have enough permissions, try rtkit */
      else
        {
          bool dbus_fail = true;

/* FIXME doesn't work */
#  if 0
#    ifdef HAVE_DBUS
          posix_priority = - priority;
          g_message (
            "not enough permissions to make thread realtime - trying RTKit (wanted priority: %d)...", posix_priority);

          guint64 process_id = (guint64) (pid_t) getpid ();
          guint64 thread_id =
            (guint64) (pid_t) syscall (SYS_gettid);

          GError *          err = NULL;
          GDBusConnection * conn =
            g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &err);
          if (!conn)
            {
              g_error (
                "Failed to get dbus connection: %s",
                err->message);
            }

          GVariant * ret_prop = g_dbus_connection_call_sync (
            conn, "org.freedesktop.portal.Desktop",
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.DBus.Properties", "Get",
            g_variant_new (
              "(ss)", "org.freedesktop.portal.Realtime",
              "MaxRealtimePriority"),
            G_VARIANT_TYPE ("(v)"), G_DBUS_CALL_FLAGS_NONE,
            -1, NULL, &err);
          if (!ret_prop)
            {
              g_error (
                "Failed to get MaxRealtimePriority: %s",
                err->message);
            }
          GVariant * prop = NULL;
          g_variant_get (ret_prop, "(v)", &prop);
          gint32 max_rt_prio = g_variant_get_int32 (prop);
          g_message ("Max RT Priority: %d", max_rt_prio);

          g_return_if_fail (max_rt_prio >= 0);
          guint32 priority_to_set = (guint32) MIN (posix_priority, max_rt_prio);
          g_message (
            "setting priority to: %u", priority_to_set);
          g_dbus_connection_call_sync (
            conn, "org.freedesktop.portal.Desktop",
            "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.Realtime",
            "MakeThreadRealtimeWithPID",
            g_variant_new (
              "(ttu)", (guint64) process_id,
              (guint64) thread_id, (guint32) max_rt_prio),
            NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &err);
          if (err)
            {
              g_error (
                "Failed to set realtime priority: %s",
                err->message);
            }

#    endif /* HAVE_DBUS */
#  endif

          goto dbus_fail_handling;

dbus_fail_handling:
          if (
            dbus_fail && is_main && ZRYTHM_HAVE_UI
            && !ZRYTHM_TESTING
            && !zrythm_app->rt_priority_message_shown)
            {
              char * str = g_strdup_printf (
                _ (
                  "Your user does not have "
                  "enough privileges to allow %s "
                  "to set the scheduling priority "
                  "of threads. Please refer to "
                  "the 'Getting Started' "
                  "section in the "
                  "user manual for details."),
                PROGRAM_NAME);
              ZrythmAppUiMessage * ui_msg =
                zrythm_app_ui_message_new (
                  GTK_MESSAGE_WARNING, str);
              g_async_queue_push (
                zrythm_app->project_load_message_queue,
                ui_msg);
              g_free (str);
              zrythm_app->rt_priority_message_shown = true;
            }
        }
    }
#endif /* __linux__ */

#ifdef __APPLE__
#  define THREAD_STACK_SIZE 0x80000 // 512kB
#else
#  define THREAD_STACK_SIZE 0x20000 // 128kB
#endif

  res = pthread_attr_setstacksize (
    &attributes, THREAD_STACK_SIZE + get_stack_size ());
  if (res)
    {
      g_critical (
        "Cannot set thread stack size res = %d (%s)", res,
        strerror (res));
      return NULL;
    }

  res = pthread_create (
    &self->pthread, &attributes,
    is_main ? &main_thread : &worker_thread, self);
  if (res)
    {
      g_critical (
        "Cannot create thread res = %d (%s)", res,
        strerror (res));
      return NULL;
    }

  pthread_attr_destroy (&attributes);

  return self;
}
