// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
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
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#include "dsp/graph_scheduler.h"
#include "dsp/graph_thread.h"
#include "utils/audio.h"
#include "utils/env.h"

namespace zrythm::dsp::graph
{

GraphScheduler::GraphScheduler (
  std::optional<juce::Thread::RealtimeOptions> rt_options,
  std::optional<juce::AudioWorkgroup>          thread_workgroup)
    : realtime_thread_options_ (rt_options), thread_workgroup_ (thread_workgroup)
{
}

void
GraphScheduler::trigger_node (GraphNode &node)
{
  /* check if we can run */
  if (node.refcount_.fetch_sub (1) == 1)
    {
      /* reset reference count for next cycle */
      node.refcount_.store (node.init_refcount_);

      // FIXME: is the code below correct? seems like it would cause data
      // races since we are increasing the size but the pointer might not be
      // pushed in the queue yet?

      /* all nodes that feed this node have completed, so this node be
       * processed now. */
      /*z_info ("triggering node, pushing back");*/
      trigger_queue_.push_back (&node);
    }
}

void
GraphScheduler::rechain_from_node_collection (GraphNodeCollection &&nodes)
{
  z_debug ("rechaining graph...");

  /* --- swap setup nodes with graph nodes --- */

  graph_nodes_ = std::move (nodes);

  terminal_refcnt_.store (
    static_cast<int> (graph_nodes_.terminal_nodes_.size ()));

  trigger_queue_.reserve (graph_nodes_.graph_nodes_.size ());

  z_debug ("rechaining done");
}

void
GraphScheduler::start_threads (std::optional<int> num_threads)
{
  if (num_threads)
    {
      num_threads.emplace (
        std::clamp (num_threads.value (), 1, MAX_GRAPH_THREADS));
    }
  else
    {
      int num_cores =
        std::min (MAX_GRAPH_THREADS, utils::audio::get_num_cores ());

      /* we reserve 1 core for the OS and other tasks and 1 core for the main
       * thread
       */
      auto num_threads_int = env_get_int ("ZRYTHM_DSP_THREADS", num_cores - 2);

      if (num_threads_int < 0)
        {
          throw ZrythmException ("number of threads must be >= 0");
        }

      num_threads.emplace (num_threads_int);
    }

  try
    {
      /* create worker threads */
      z_debug ("creating {} threads", num_threads.value ());
      for (int i = 0; i < num_threads.value (); ++i)
        {
          threads_.emplace_back (
            std::make_unique<GraphThread> (i, false, *this, thread_workgroup_));
        }

      /* and the main thread */
      main_thread_ =
        std::make_unique<GraphThread> (-1, true, *this, thread_workgroup_);
    }
  catch (const std::exception &e)
    {
      terminate_threads ();
      throw ZrythmException (
        "failed to create thread: " + std::string (e.what ()));
    }

  auto start_thread = [&] (auto &thread) {
    try
      {
        auto success = thread->startRealtimeThread (
          realtime_thread_options_
            ? realtime_thread_options_.value ()
            : juce::Thread::RealtimeOptions ().withPriority (9));
        if (!success)
          {
            z_warning ("failed to start realtime thread, trying normal thread");
            // fallback to non-realtime thread
            success = thread->startThread ();
            if (!success)
              {
                throw ZrythmException ("startThread failed");
              }
          }
      }
    catch (const ZrythmException &e)
      {
        terminate_threads ();
        throw ZrythmException (
          "failed to start thread: " + std::string (e.what ()));
      }
  };

  /* start them */
  for (auto &thread : threads_)
    {
      start_thread (thread);
    }
  start_thread (main_thread_);

  /* wait for all threads to go idle */
  while (idle_thread_cnt_.load () != static_cast<int> (threads_.size ()))
    {
      /* wait for all threads to go idle */
      z_info (
        "waiting for all {} threads to go idle after creation (current idle {})...",
        threads_.size (), idle_thread_cnt_.load ());
      std::this_thread::sleep_for (std::chrono::milliseconds (1));
    }
}

void
GraphScheduler::terminate_threads ()
{
  z_info ("terminating graph...");

  /* Flag threads to terminate */
  for (auto &thread : threads_)
    {
      thread->signalThreadShouldExit ();
    }
  main_thread_->signalThreadShouldExit ();

  /* wait for all threads to go idle */
  constexpr auto max_tries = 100;
  auto           tries = 0;
  while (idle_thread_cnt_.load () != static_cast<int> (threads_.size ()))
    {
      z_info ("waiting for threads to go idle...");
      std::this_thread::sleep_for (std::chrono::milliseconds (1));
      if (++tries > max_tries)
        break;
    }

  /* wake-up sleeping threads */
  int tc = idle_thread_cnt_.load ();
  assert (tc >= 0);
  assert (tc <= static_cast<int> (threads_.size ()));
  if (tc != static_cast<int> (threads_.size ()))
    {
      z_warning ("expected {} idle threads, found {}", threads_.size (), tc);
    }
  for (const auto _ : std::views::iota (0, tc))
    {
      trigger_sem_.signal ();
    }

  /* and the main thread */
  callback_start_sem_.release ();

  /* join threads */
  for (auto &thread : threads_)
    {
      thread->waitForThreadToExit (5);
    }
  main_thread_->waitForThreadToExit (5);
  threads_.clear ();
  main_thread_.reset ();

  z_info ("graph terminated");
}

bool
GraphScheduler::contains_thread (RTThreadId::IdType thread_id)
{
  for (const auto &thread : threads_)
    {
      if (thread_id == thread->rt_thread_id_)
        {
          return true;
        }
    }

  return main_thread_ && thread_id == main_thread_->rt_thread_id_;
}

void
GraphScheduler::run_cycle (
  const EngineProcessTimeInfo time_nfo,
  const nframes_t             remaining_preroll_frames)
{
  time_nfo_ = time_nfo;
  remaining_preroll_frames_ = remaining_preroll_frames;

  /* process special nodes first */
  for (const auto node : graph_nodes_.special_nodes_)
    {
      node.get ().process (time_nfo_, remaining_preroll_frames);
      node.get ().set_skip_processing (true);
    }

  callback_start_sem_.release ();
  callback_done_sem_.acquire ();

  /* reset bypass state of special nodes */
  for (const auto node : graph_nodes_.special_nodes_)
    {
      node.get ().set_skip_processing (false);
    }
}

GraphScheduler::~GraphScheduler ()
{
  if (main_thread_ || !threads_.empty ())
    {
      z_info ("terminating graph");
      terminate_threads ();
    }
  else
    {
      z_info ("graph already terminated");
    }
}

} // namespace zrythm::dsp::graph
