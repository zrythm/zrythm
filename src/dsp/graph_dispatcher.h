// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph_builder.h"
#include "dsp/graph_scheduler.h"
#include "utils/rt_thread_id.h"
#include "utils/types.h"

namespace zrythm::dsp
{

/**
 * @brief The DspGraphDispatcher class manages the processing graph for the
 * audio engine.
 *
 * The DspGraphDispatcher class is responsible for maintaining the active
 * processing graph, handling graph setup and recalculation, and managing the
 * processing cycle. It provides methods for starting a new processing cycle,
 * retrieving the maximum playback latency, and checking the current processing
 * thread.
 */
class DspGraphDispatcher final
{
public:
  using RunFunctionWithEngineLock = std::function<void (std::function<void ()>)>;

  DspGraphDispatcher (
    std::unique_ptr<graph::IGraphBuilder>      graph_builder,
    std::vector<graph::IProcessable *>         terminal_processables,
    const juce::AudioDeviceManager            &device_manager,
    RunFunctionWithEngineLock                  run_function_with_engine_lock,
    graph::GraphScheduler::RunOnMainThreadFunc run_on_main_thread);

  /**
   * Recalculates the process acyclic directed graph.
   *
   * @param soft If true, only readjusts latencies.
   */
  void recalc_graph (bool soft);

  /**
   * Starts a new cycle.
   *
   * @param realtime_context Whether this is called in a realtime context.
   * @param remaining_latency_preroll Remaining number of playback latency
   * preroll samples for this cycle.
   */
  void start_cycle (
    const dsp::ITransport &current_transport_state,
    EngineProcessTimeInfo  time_nfo,
    nframes_t              remaining_latency_preroll,
    bool                   realtime_context) noexcept [[clang::nonblocking]];

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
    return process_kickoff_thread_.has_value ()
             ? current_thread_id.get () == process_kickoff_thread_.value ()
             : false;
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

  /**
   * @brief Accessor for currently active trigger nodes.
   *
   * @note This must only be called outside of start_cycle() calls (TODO:
   * enforce this).
   */
  auto &current_trigger_nodes () const
  {
    return scheduler_->get_nodes ().trigger_nodes_;
  }

private:
  /**
   * @brief Global cycle pre-processing logic that does not require rebuilding
   * the graph.
   *
   * Some examples of what could go here:
   * 1. live piano-roll keys
   * 2. transport (start/stop/seek/loop/tempo)
   * 3. global panic
   * 4. external clock/time-sig
   * 5. one-shot automation jumps
   *
   * Everything else (plugins, clips, faders) lives inside the graph.
   *
   * @param time_nfo Current cycle time info.
   */
  void
  preprocess_at_start_of_cycle (const EngineProcessTimeInfo &time_nfo) noexcept
    [[clang::nonblocking]];

private:
  std::unique_ptr<graph::IGraphBuilder> graph_builder_;
  const juce::AudioDeviceManager       &device_manager_;

  /**
   * @brief Callable to call in a context where the audio processing engine is
   * locked.
   */
  RunFunctionWithEngineLock run_function_with_engine_lock_;

  /**
   * @brief Terminal processables to prune the graph to.
   */
  std::vector<graph::IProcessable *> terminal_processables_;

  std::unique_ptr<graph::GraphScheduler> scheduler_;

  /** Stored for the currently processing cycle */
  nframes_t max_route_playback_latency_ = 0;

  /**
   * @brief Current global latency offset.
   *
   * Calculated as [max latency of all routes] minus [remaining latency from
   * engine].
   *
   * TODO: figure out if this is supposed to be used anywhere, or delete.
   */
  nframes_t global_offset_ = 0;

  /** ID of the thread that calls kicks off the cycle. */
  std::optional<unsigned int> process_kickoff_thread_;

  graph::GraphScheduler::RunOnMainThreadFunc run_on_main_thread_;
};

}
