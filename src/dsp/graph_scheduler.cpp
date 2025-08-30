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
  sample_rate_t                                sample_rate,
  nframes_t                                    max_block_length,
  std::optional<juce::Thread::RealtimeOptions> rt_options,
  std::optional<juce::AudioWorkgroup>          thread_workgroup)
    : thread_workgroup_ (thread_workgroup), sample_rate_ (sample_rate),
      max_block_length_ (max_block_length)
{
  if (rt_options)
    {
      realtime_thread_options_ = rt_options.value ();
    }
  else
    {
      realtime_thread_options_ =
        juce::Thread::RealtimeOptions ().withPriority (9);
    }
  realtime_thread_options_ =
    realtime_thread_options_.withApproximateAudioProcessingTime (
      max_block_length, sample_rate);
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
GraphScheduler::rechain_from_node_collection (
  GraphNodeCollection &&nodes,
  sample_rate_t         sample_rate,
  nframes_t             max_block_length)
{
  z_debug ("rechaining graph...");

  // cleanup previous graph nodes
  release_node_resources ();

  /* --- swap setup nodes with graph nodes --- */

  graph_nodes_ = std::move (nodes);

  terminal_refcnt_.store (
    static_cast<int> (graph_nodes_.terminal_nodes_.size ()));

  trigger_queue_.reserve (graph_nodes_.graph_nodes_.size ());

  sample_rate_ = sample_rate;
  max_block_length_ = max_block_length;
  prepare_nodes_for_processing ();

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
          threads_.insert (
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
        auto success = thread->startRealtimeThread (realtime_thread_options_);
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

  // start them sequentially
  threads_.cvisit_all ([&] (auto &thread) {
    start_thread (thread);

    // wait for this thread to initialize before starting the next one
    // this fixes an internal JUCE race condition on first thread construction
    while (idle_thread_cnt_.load () < 1)
      {
        std::this_thread::sleep_for (std::chrono::microseconds (100));
      }
  });
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
  threads_.cvisit_all ([&] (auto &thread) {
    thread->signalThreadShouldExit ();
  });
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
  callback_start_sem_.signal ();

  /* join threads - wait indefinitely to ensure complete termination */
  threads_.cvisit_all ([&] (auto &thread) {
    thread->waitForThreadToExit (-1);
  });
  main_thread_->waitForThreadToExit (-1);

  /* Clear threads only after all threads have been joined */
  threads_.clear ();
  main_thread_.reset ();

  z_info ("graph terminated");
}

bool
GraphScheduler::contains_thread (RTThreadId::IdType thread_id)
{
  // this is not ideal performance-wise, but this method is called rarely and
  // the result is cached in DspGraphDispatcher
  bool found{};
  threads_.cvisit_while ([&] (auto &thread) {
    if (thread_id == thread->rt_thread_id_.load ())
      {
        found = true;
        return false; // finish
      }
    return true; // keep on visiting
  });
  if (found)
    {
      return true;
    }

  return main_thread_ && thread_id == main_thread_->rt_thread_id_.load ();
}

void
GraphScheduler::run_cycle (
  const EngineProcessTimeInfo time_nfo,
  const nframes_t             remaining_preroll_frames)
{
  time_nfo_ = time_nfo;
  remaining_preroll_frames_ = remaining_preroll_frames;

  callback_start_sem_.signal ();
  callback_done_sem_.wait ();
}

void
GraphScheduler::prepare_nodes_for_processing ()
{
  z_debug (
    "preparing nodes for processing with sample rate {} and max block length {}",
    sample_rate_, max_block_length_);
  for (auto &node : graph_nodes_.graph_nodes_)
    {
      node->get_processable ().prepare_for_processing (
        sample_rate_, max_block_length_);
    }

  graph_nodes_.update_latencies ();
}

void
GraphScheduler::release_node_resources ()
{
  for (auto &node : graph_nodes_.graph_nodes_)
    {
      node->get_processable ().release_resources ();
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

  release_node_resources ();
}

} // namespace zrythm::dsp::graph
