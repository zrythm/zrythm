// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "dsp/graph.h"
#include "dsp/graph_dispatcher.h"
#include "dsp/graph_pruner.h"

namespace zrythm::dsp
{
DspGraphDispatcher::DspGraphDispatcher (
  std::unique_ptr<graph::IGraphBuilder>      graph_builder,
  std::vector<graph::IProcessable *>         terminal_processables,
  const IHardwareAudioInterface             &hw_interface,
  RunFunctionWithEngineLock                  run_function_with_engine_lock,
  graph::GraphScheduler::RunOnMainThreadFunc run_on_main_thread)
    : graph_builder_ (std::move (graph_builder)), hw_interface_ (hw_interface),
      run_function_with_engine_lock_ (std::move (run_function_with_engine_lock)),
      terminal_processables_ (std::move (terminal_processables)),
      run_on_main_thread_ (std::move (run_on_main_thread))
{
}

nframes_t
DspGraphDispatcher::get_max_route_playback_latency ()
{
  if (scheduler_ == nullptr)
    return 0;

  max_route_playback_latency_ =
    scheduler_->get_nodes ().get_max_route_playback_latency ();

  return max_route_playback_latency_;
}

void
DspGraphDispatcher::preprocess_at_start_of_cycle (
  const EngineProcessTimeInfo &time_nfo) noexcept
{

  // TODO: fill live key-press events for the currently active piano roll
  {
#if 0
    [[maybe_unused]] auto &midi_events = piano_roll_events_;
    if (time_nfo.local_offset_ == 0 && CLIP_EDITOR->has_region ())
      {
// FIXME!!!! threading bug here. clip editor region & track may be
// changed in the main (UI) thread while this is attempted
        auto clip_editor_track_var =
          CLIP_EDITOR->get_region_and_track ()->second;
        std::visit (
          [&] (auto &&track) {
            using TrackT = base_type<decltype (track)>;
            if constexpr (structure::tracks::PianoRollTrack<TrackT>)
              {
                auto &target_port =
                  track->get_track_processor ()->get_midi_in_port ();
                /* if not set to "all channels", filter-append */
                if (track->pianoRollTrackMixin ()
                      ->allow_only_specific_midi_channels ())
                  {
                    target_port.midi_events_.active_events_.append_w_filter (
                      midi_events,
                      track->pianoRollTrackMixin ()->midi_channels_to_allow (),
                      time_nfo.local_offset_, time_nfo.nframes_);
                  }
                /* otherwise append normally */
                else
                  {
                    target_port.midi_events_.active_events_.append (
                      midi_events, time_nfo.local_offset_, time_nfo.nframes_);
                  }
                midi_events.clear ();
              }
          },
          clip_editor_track_var);
        }
#endif
  }
}

void
DspGraphDispatcher::start_cycle (
  const ITransport     &current_transport_state,
  EngineProcessTimeInfo time_nfo,
  nframes_t             remaining_latency_preroll,
  bool                  realtime_context) noexcept
{
  if (scheduler_ == nullptr)
    {
      return;
    }
  assert (time_nfo.g_start_frame_w_offset_ >= time_nfo.g_start_frame_);

  /* only set the kickoff thread when called from a realtime context during
   * audio processing (sometimes this method is called from the UI thread to
   * force some processing) */
  if (realtime_context)
    process_kickoff_thread_ = current_thread_id.get ();

  global_offset_ = max_route_playback_latency_ - remaining_latency_preroll;

  preprocess_at_start_of_cycle (time_nfo);

  scheduler_->run_cycle (
    time_nfo, remaining_latency_preroll, current_transport_state);
}

void
DspGraphDispatcher::recalc_graph (bool soft)
{
  z_info ("Recalculating processing graph{}...", soft ? " (soft)" : "");

  const auto sample_rate = hw_interface_.get_sample_rate ();
  const auto buffer_size = hw_interface_.get_block_length ();

  const auto rebuild_graph = [&] () {
    graph::Graph graph;

    // Build graph
    graph_builder_->build_graph (graph);
    z_debug (
      "Built graph (before pruning): {}",
      graph::GraphExport::export_to_dot (graph, true));

    // Prune graph
    {
      std::vector<std::reference_wrapper<graph::GraphNode>> terminals;
      for (const auto &processable : terminal_processables_)
        {
          auto * node =
            graph.get_nodes ().find_node_for_processable (*processable);
          terminals.emplace_back (*node);
        }
      graph::GraphPruner::prune_graph_to_terminals (graph, terminals);
      z_debug (
        "Built graph (pruned): {}",
        graph::GraphExport::export_to_dot (graph, true));
    }

    scheduler_->rechain_from_node_collection (
      graph.steal_nodes (), sample_rate, buffer_size);
  };

  if (!scheduler_ && !soft)
    {
      scheduler_ = std::make_unique<graph::GraphScheduler> (
        run_on_main_thread_, sample_rate, buffer_size, true,
        hw_interface_.get_device_audio_workgroup ());
      rebuild_graph ();
      scheduler_->start_threads ();
      return;
    }

  run_function_with_engine_lock_ ([&] () {
    if (soft)
      {
        scheduler_->get_nodes ().update_latencies ();
      }
    else
      {
        rebuild_graph ();
      }
  });

  z_info ("Processing graph ready");
}
}
