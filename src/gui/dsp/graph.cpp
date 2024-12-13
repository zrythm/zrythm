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

#include "gui/dsp/graph.h"
#include "gui/dsp/graph_thread.h"
#include "utils/audio.h"
#include "utils/env.h"

using namespace zrythm;

Graph::Graph () = default;

void
Graph::trigger_node (GraphNode &node)
{
  /* check if we can run */
  if (node.refcount_.fetch_sub (1) == 1)
    {
      /* reset reference count for next cycle */
      node.refcount_.store (node.init_refcount_);

      // FIXME: is the code below correct? seems like it would cause data races
      // since we are increasing the size but the pointer might not be pushed in
      // the queue yet?

      /* all nodes that feed this node have completed, so this node be
       * processed now. */
      trigger_queue_size_.fetch_add (1);
      /*z_info ("triggering node, pushing back");*/
      trigger_queue_.push_back (&node);
    }
}

bool
Graph::is_valid () const
{
  std::vector<GraphNode *> triggers;
  for (auto trigger : setup_init_trigger_list_)
    {
      triggers.push_back (trigger);
    }

  while (!triggers.empty ())
    {
      auto trigger = triggers.back ();
      triggers.pop_back ();

      for (auto child : trigger->childnodes_)
        {
          trigger->childnodes_.pop_back ();
          child->init_refcount_--;
          if (child->init_refcount_ == 0)
            {
              triggers.push_back (child);
            }
        }
    }

  for (auto &node : setup_graph_nodes_vector_)
    {
      if (!node->childnodes_.empty () || node->init_refcount_ > 0)
        return false;
    }

  return true;
}

void
Graph::clear_setup ()
{
  setup_graph_nodes_vector_.clear ();
  setup_init_trigger_list_.clear ();
  setup_terminal_nodes_.clear ();
}

void
Graph::rechain ()
{
  z_return_if_fail (trigger_queue_size_.load () == 0);

  /* --- swap setup nodes with graph nodes --- */

  std::swap (graph_nodes_vector_, setup_graph_nodes_vector_);

  std::swap (init_trigger_list_, setup_init_trigger_list_);
  std::swap (terminal_nodes_, setup_terminal_nodes_);

  terminal_refcnt_.store (terminal_nodes_.size ());

  trigger_queue_.reserve (graph_nodes_vector_.size ());

  clear_setup ();
}

void
Graph::print () const
{
  z_info ("==printing graph");

  for (const auto &node : setup_graph_nodes_vector_)
    {
      node->print_node ();
    }

  z_info (
    "num trigger nodes {} | num terminal nodes {}",
    setup_init_trigger_list_.size (), setup_terminal_nodes_.size ());
  z_info ("==finish printing graph");
}

Graph::~Graph ()
{
  if (main_thread_ || !threads_.empty ())
    {
      z_info ("terminating graph");
      terminate ();
    }
  else
    {
      z_info ("graph already terminated");
    }
}

nframes_t
Graph::get_max_route_playback_latency (bool use_setup_nodes)
{
  nframes_t   max = 0;
  const auto &nodes =
    use_setup_nodes ? setup_init_trigger_list_ : init_trigger_list_;
  for (const auto &node : nodes)
    {
      if (node->route_playback_latency_ > max)
        max = node->route_playback_latency_;
    }

  return max;
}

void
Graph::update_latencies (bool use_setup_nodes)
{
  z_info ("updating graph latencies...");

  /* reset latencies */
  auto &nodes =
    use_setup_nodes ? setup_graph_nodes_vector_ : graph_nodes_vector_;
  z_debug ("setting all latencies to 0");
  for (auto &node : nodes)
    {
      node->playback_latency_ = 0;
      node->route_playback_latency_ = 0;
    }
  z_debug ("done setting all latencies to 0");

  z_debug ("iterating over {} nodes...", nodes.size ());
  for (auto &node : nodes)
    {
      node->playback_latency_ = node->get_single_playback_latency ();
      if (node->playback_latency_ > 0)
        {
          node->set_route_playback_latency (node->playback_latency_);
        }
    }
  z_debug ("iterating done...");

  z_info (
    "Total latencies:\n"
    "Playback: {}\n"
    "Recording: {}\n",
    get_max_route_playback_latency (use_setup_nodes), 0);
}

void
Graph::finish_adding_nodes ()
{

  /* ========================
   * set initial and terminal nodes
   * ======================== */

  set_initial_and_terminal_nodes ();

  /* ========================
   * calculate latencies of each port and each processor
   * ======================== */

  update_latencies (true);
}

void
Graph::set_initial_and_terminal_nodes ()
{
  setup_terminal_nodes_.clear ();
  setup_init_trigger_list_.clear ();
  for (const auto &node : setup_graph_nodes_vector_)
    {
      if (node->childnodes_.empty ())
        {
          /* terminal node */
          node->terminal_ = true;
          setup_terminal_nodes_.push_back (node.get ());
        }
      if (node->init_refcount_ == 0)
        {
          /* initial node */
          node->initial_ = true;
          setup_init_trigger_list_.push_back (node.get ());
        }
    }
}

void
Graph::start (ThreadCreateFunc thread_create_func, std::optional<int> num_threads)
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
          threads_.emplace_back (thread_create_func (false, i, *this));
        }

      /* and the main thread */
      main_thread_ = thread_create_func (true, -1, *this);
    }
  catch (const std::exception &e)
    {
      terminate ();
      throw ZrythmException (
        "failed to create thread: " + std::string (e.what ()));
    }

  auto start_thread = [&] (auto &thread) {
    try
      {
        thread->startRealtimeThread (
          juce::Thread::RealtimeOptions ().withPriority (9));
      }
    catch (const ZrythmException &e)
      {
        terminate ();
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
      z_info ("waiting for threads to go idle after creation...");
      std::this_thread::sleep_for (std::chrono::milliseconds (1));
    }
}

void
Graph::terminate ()
{
  z_info ("terminating graph...");

  /* Flag threads to terminate */
  for (auto &thread : threads_)
    {
      thread->signalThreadShouldExit ();
    }
  main_thread_->signalThreadShouldExit ();

  /* wait for all threads to go idle */
  while (idle_thread_cnt_.load () != static_cast<int> (threads_.size ()))
    {
      z_info ("waiting for threads to go idle...");
      std::this_thread::sleep_for (std::chrono::milliseconds (1));
    }

  /* wake-up sleeping threads */
  int tc = idle_thread_cnt_.load ();
  if (tc != static_cast<int> (threads_.size ()))
    {
      z_warning ("expected {} idle threads, found {}", threads_.size (), tc);
    }
  for (int i = 0; i < tc; ++i)
    {
      trigger_sem_.release ();
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

Graph::GraphNode *
Graph::find_node_for_processable (const dsp::IProcessable &processable) const
{
  auto it =
    std::ranges::find_if (setup_graph_nodes_vector_, [&] (const auto &node) {
      return &node->get_processable () == &processable;
    });
  if (it != setup_graph_nodes_vector_.end ())
    {
      return it->get ();
    }
  return nullptr;
}

Graph::GraphNode *
Graph::add_node_for_processable (
  dsp::IProcessable     &node,
  const dsp::ITransport &transport)
{
  setup_graph_nodes_vector_.emplace_back (std::make_unique<GraphNode> (
    setup_graph_nodes_vector_.size (), transport, node));
  return setup_graph_nodes_vector_.back ().get ();
}

#if 0
GraphThread *
Graph::get_current_thread () const
{
  for (auto &thread : threads_)
    {
      if (thread->rt_thread_id_ == current_thread_id.get ())
        {
          return thread.get ();
        }
    }
  if (main_thread_->rt_thread_id_ == current_thread_id.get ())
    {
      return main_thread_.get ();
    }
  return nullptr;
}
#endif
