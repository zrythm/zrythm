// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---
 */

#pragma once

#include "zrythm-config.h"

#include "dsp/graph_scheduler.h"
#include "engine/device_io/engine.h"
#include "utils/rt_thread_id.h"
#include "utils/types.h"

#define ROUTER (AUDIO_ENGINE->router_)

namespace zrythm::engine::session
{

/**
 * @brief The Router class manages the processing graph for the audio engine.
 *
 * The Router class is responsible for maintaining the active processing graph,
 * handling graph setup and recalculation, and managing the processing cycle.
 * It provides methods for starting a new processing cycle, retrieving the
 * maximum playback latency, and checking the current processing thread.
 */
class Router final
{
public:
  Router (device_io::AudioEngine * engine = nullptr);

  /**
   * Recalculates the process acyclic directed graph.
   *
   * @param soft If true, only readjusts latencies.
   */
  void recalc_graph (bool soft);

  /**
   * Starts a new cycle.
   */
  void start_cycle (EngineProcessTimeInfo time_nfo);

  /**
   * Returns the max playback latency of the trigger
   * nodes.
   */
  nframes_t get_max_route_playback_latency ();

  /**
   * Returns whether this is the thread that kicks off processing (thread that
   * calls router_start_cycle()).
   */
  [[nodiscard, gnu::hot]] bool is_processing_kickoff_thread () const
  {
    return current_thread_id.get () == process_kickoff_thread_;
  }

  /**
   * Returns if the current thread is a processing thread.
   */
  [[nodiscard, gnu::hot]] bool is_processing_thread () const
  {
    /* this is called too often so use this optimization */
    static thread_local bool have_result = false;
    static thread_local bool is_processing_thread = false;

    if (have_result) [[likely]]
      {
        return is_processing_thread;
      }

    if (!scheduler_) [[unlikely]]
      {
        have_result = false;
        is_processing_thread = false;
        return false;
      }

    if (scheduler_->contains_thread (current_thread_id.get ()))
      {
        is_processing_thread = true;
        have_result = true;
        return true;
      }

    have_result = true;
    is_processing_thread = false;
    return false;
  }

public:
  std::unique_ptr<dsp::graph::GraphScheduler> scheduler_;

  /** An atomic variable to check if the graph is currently being setup (so that
   * we can avoid accessing buffers changed by this). */
  std::atomic<bool> graph_setup_in_progress_ = false;

  /** Time info for this processing cycle. */
  EngineProcessTimeInfo time_nfo_;

  /** Stored for the currently processing cycle */
  nframes_t max_route_playback_latency_ = 0;

  /** Current global latency offset (max latency
   * of all routes - remaining latency from
   * engine). */
  nframes_t global_offset_ = 0;

  /** Used when recalculating the graph. */
  std::binary_semaphore graph_access_sem_{ 1 };

  bool callback_in_progress_ = false;

  /** ID of the thread that calls kicks off the cycle. */
  unsigned int process_kickoff_thread_ = 0;

  device_io::AudioEngine * audio_engine_{};
};

}
