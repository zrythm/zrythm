// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
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

#include "common/utils/dsp.h"
#include "common/utils/logger.h"

#include <fmt/format.h>

#ifndef _WIN32
#  include <sys/resource.h>
#endif

#if !defined _WIN32 && defined __GLIBC__
#  include <climits>

#  include <dlfcn.h>
#  include <pthread.h>
#endif

#include "common/dsp/engine.h"
#include "common/dsp/graph.h"
#include "common/dsp/graph_node.h"
#include "common/dsp/graph_thread.h"
#include "common/dsp/router.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

/* uncomment to show debug messages */
// #define DEBUG_THREADS 1

#ifdef __APPLE__
constexpr auto THREAD_STACK_SIZE = 0x80000; // 512kB
#else
constexpr auto THREAD_STACK_SIZE = 0x20000; // 128kB
#endif

void
GraphThread::on_reached_terminal_node ()
{
  z_return_if_fail (graph_.terminal_refcnt_.load () >= 0);
  if (graph_.terminal_refcnt_.fetch_sub (1) == 1)
    {
      /* all terminal nodes have completed, we're done with this cycle. */
      z_warn_if_fail (graph_.trigger_queue_size_.load () == 0);

      /* Notify caller */
      graph_.callback_done_sem_.release ();

      /* Ensure that all background threads are idle.
       * When freewheeling there may be an immediate restart:
       * If there are more threads than CPU cores, some worker- threads may only
       * be "on the way" to become idle. */
      while (
        graph_.idle_thread_cnt_.load ()
        != static_cast<int> (graph_.threads_.size ()))
        yield ();

      if (threadShouldExit ())
        return;

      /* now wait for the next cycle to begin */
      graph_.callback_start_sem_.acquire ();

      if (threadShouldExit ())
        return;

      /* reset terminal reference count */
      graph_.terminal_refcnt_.store (graph_.terminal_nodes_.size ());

      /* and start the initial nodes */
      for (auto &node : graph_.init_trigger_list_)
        {
          graph_.trigger_queue_size_.fetch_add (1);
          graph_.trigger_queue_.push_back (node);
        }
      /* continue in worker-thread */
    }
}

void
GraphThread::run_worker ()
{
  auto * graph = &graph_;

  std::optional<LspDspContextRAII> lsp_dsp_context_raii;
  if (ZRYTHM_USE_OPTIMIZED_DSP)
    {
      lsp_dsp_context_raii.emplace ();
    }

  z_info (
    "Worker thread {} created (num threads {})", id_, graph->threads_.size ());

  /* wait for all threads to get created */
  if (id_ < static_cast<int> (graph->threads_.size ()) - 1)
    {
      yield ();
    }

  for (;;)
    {
      GraphNode * to_run = nullptr;

      if (threadShouldExit ())
        {
          if (id_ == -1)
            {
              z_info ("terminating main thread");
            }
          else
            {
              z_info ("[{}]: terminating thread", id_);
            }
          return;
        }

      if (graph->trigger_queue_.pop_front (to_run))
        {
          if (to_run == nullptr) [[unlikely]]
            {
              z_error ("[{}]: null node in queue, terminating thread...", id_);
              return;
            }
#ifdef DEBUG_THREADS
          z_debug (
            "[{}]: dequeued node below (nodes left {})\n{}", id_,
            graph->trigger_queue_size_.load (), to_run->print_to_str ());
#endif
          /* Wake up idle threads, but at most as many as there's work in the
           * trigger queue that can be processed by other threads. This thread
           * as not yet decreased _trigger_queue_size. */
          int idle_cnt = graph->idle_thread_cnt_.load ();
          int work_avail = graph->trigger_queue_size_.load ();
          int wakeup = std::min (idle_cnt + 1, work_avail);
#ifdef DEBUG_THREADS
          z_debug (
            "[{}]: Waking up {} idle threads (idle count {}), work available -> {}",
            id_, wakeup - 1, idle_cnt, work_avail);
#endif

          for (int i = 1; i < wakeup; ++i)
            {
              graph->trigger_sem_.release ();
            }
        }

      while (to_run == nullptr)
        {
          /* wait for work, fall asleep */
          int idle_thread_cnt = graph->idle_thread_cnt_.fetch_add (1) + 1;
#ifdef DEBUG_THREADS
          z_debug (
            "[{}]: no node to run. just increased idle thread count and waiting for work "
            "(current idle threads {})",
            id_, idle_thread_cnt);
#endif
          if (idle_thread_cnt > static_cast<int> (graph->threads_.size ()))
            [[unlikely]]
            {
              z_error (
                "[{}]: idle thread count {} is greater than the number of threads "
                "{}. this should never occur",
                id_, idle_thread_cnt, graph->threads_.size ());
            }

          graph->trigger_sem_.acquire ();

          if (threadShouldExit ())
            {
              return;
            }

          /* not idle anymore - decrease idle thread count */
          graph->idle_thread_cnt_.fetch_sub (1);
#ifdef DEBUG_THREADS
          z_info (
            "[{}]: work found, decremented idle thread count (current count %d) and "
            "dequeuing node to process",
            id_, graph->idle_thread_cnt_.load ());
#endif

          /* try to find some work to do */
          graph->trigger_queue_.pop_front (to_run);
        }

      /* this thread has now claimed the graph node for processing - process it */
      graph->trigger_queue_size_.fetch_sub (1);
#ifdef DEBUG_THREADS
      z_info ("[{}]: running node", id_);
#endif
      to_run->process (graph->router_->time_nfo_, *this);
    }
}

ATTR_REALTIME
void
GraphThread::run ()
{
  /* pre-create the unique identifier of this thread */
  rt_thread_id_ = current_thread_id.get ();

  if (is_main_)
    {
      auto graph = &graph_;

      /* Wait until all worker threads are active */
      while (
        graph->idle_thread_cnt_.load ()
        != static_cast<int> (graph->threads_.size ()))
        {
          yield ();
        }

      /* wait for initial process callback */
      graph->callback_start_sem_.acquire ();

      /* first time setup */

      /* Can't run without a graph */
      z_warn_if_fail (!graph->graph_nodes_map_.empty ());
      z_warn_if_fail (!graph->init_trigger_list_.empty ());
      z_warn_if_fail (!graph->terminal_nodes_.empty ());

      /* bootstrap trigger-list.
       * (later this is done by Graph.reached_terminal_node())*/
      for (auto &node : graph->init_trigger_list_)
        {
          graph->trigger_queue_size_.fetch_add (1);
          /*z_info ("[main] pushing back node {} during bootstrap", i);*/
          graph->trigger_queue_.push_back (node);
        }
    }

  /* after setup, the main-thread just becomes a normal worker */
  run_worker ();
}

static size_t
get_stack_size ()
{
  size_t rv = 0;
#if !defined _WIN32 && defined __GLIBC__

  size_t pt_min_stack = 16384;

#  ifdef PTHREAD_STACK_MIN
  pt_min_stack = PTHREAD_STACK_MIN;
#  endif

  void * handle = dlopen (nullptr, RTLD_LAZY);

  /* This function is internal (it has a GLIBC_PRIVATE) version, but
   * available via weak symbol, or dlsym, and returns
   *
   * GLRO(dl_pagesize) + __static_tls_size + PTHREAD_STACK_MIN
   */

  auto * __pthread_get_minstack = (size_t (*) (const pthread_attr_t *)) dlsym (
    handle, "__pthread_get_minstack");

  if (__pthread_get_minstack != nullptr)
    {
      pthread_attr_t attr;
      pthread_attr_init (&attr);
      rv = __pthread_get_minstack (&attr);
      z_return_val_if_fail (rv >= pt_min_stack, 0);
      rv -= pt_min_stack;
      pthread_attr_destroy (&attr);
    }
  dlclose (handle);
#endif
  return rv;
}

GraphThread::GraphThread (const int id, const bool is_main, Graph &graph)
    : juce::Thread (
        is_main ? "GraphWorkerMain" : fmt::format ("GraphWorker{}", id),
        THREAD_STACK_SIZE + get_stack_size ()),
      id_ (id), is_main_ (is_main), graph_ (graph)
{
}