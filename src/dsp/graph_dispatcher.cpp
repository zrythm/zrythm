// SPDX-FileCopyrightText: © 2019-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "dsp/graph.h"
#include "dsp/graph_dispatcher.h"
#include "dsp/graph_export.h"
#include "dsp/graph_pruner.h"
#include "utils/logger.h"
#include "utils/tracy.h"

namespace zrythm::dsp
{
DspGraphDispatcher::DspGraphDispatcher (
  std::unique_ptr<graph::IGraphBuilder>      graph_builder,
  TerminalProcessablesProvider               terminal_processables_provider,
  const IHardwareAudioInterface             &hw_interface,
  RunFunctionWithEngineLock                  run_function_with_engine_lock,
  graph::GraphScheduler::RunOnMainThreadFunc run_on_main_thread,
  std::optional<juce::AudioWorkgroup>        workgroup)
    : graph_builder_ (std::move (graph_builder)), hw_interface_ (hw_interface),
      workgroup_ (std::move (workgroup)),
      run_function_with_engine_lock_ (std::move (run_function_with_engine_lock)),
      terminal_processables_provider_ (std::move (terminal_processables_provider)),
      run_on_main_thread_ (std::move (run_on_main_thread))
{
}

units::sample_u32_t
DspGraphDispatcher::get_max_route_playback_latency ()
{
  if (scheduler_ == nullptr)
    return {};

  max_route_playback_latency_ =
    scheduler_->get_nodes ().get_max_route_playback_latency ();

  return max_route_playback_latency_;
}

void
DspGraphDispatcher::preprocess_at_start_of_cycle (
  const dsp::graph::ProcessBlockInfo &time_nfo) noexcept
{

  // TODO: fill live key-press events for the currently active piano roll
  {
#if 0
    [[maybe_unused]] auto &midi_events = piano_roll_events_;
    if (time_nfo.buffer_offset_ == 0 && CLIP_EDITOR->has_region ())
      {
// FIXME!!!! threading bug here. clip editor region & track may be
// changed in the main (UI) thread while this is attempted
        auto clip_editor_track_var =
          CLIP_EDITOR->get_region_and_track ()->second;
        std::visit (
          [&] (auto &&track) {
            using TrackT = utils::base_type<decltype (track)>;
            if constexpr (structure::tracks::PianoRollTrack<TrackT>)
              {
                auto &target_port =
                  track->get_track_processor ()->get_midi_in_port ();
                /* if not set to "all channels", filter-append */
                if (track->pianoRollTrackMixin ()
                      ->allow_only_specific_midi_channels ())
                  {
                    {
                      const auto channels =
                        track->pianoRollTrackMixin ()->midi_channels_to_allow ();
                      for (const auto &src_ev : midi_events)
                        {
                          if (
                            src_ev.time_ < time_nfo.buffer_offset_
                            || src_ev.time_
                                 >= time_nfo.buffer_offset_ + time_nfo.nframes_)
                            continue;
                          const auto d = src_ev.data ();
                          const midi_byte_t channel = d[0] & 0x0f;
                          if (!channels[channel])
                            continue;
                          target_port.buffer_.push_back (src_ev);
                        }
                    }
                  }
                /* otherwise append normally */
                else
                  {
                    midi_event::append_in_range (
                      target_port.buffer_, midi_events,
                      std::make_pair (
                        time_nfo.buffer_offset_,
                        time_nfo.buffer_offset_ + time_nfo.nframes_));
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
  const ITransport            &current_transport_state,
  dsp::graph::ProcessBlockInfo time_nfo,
  units::sample_u64_t          remaining_latency_preroll,
  bool                         realtime_context,
  const dsp::TempoMap         &tempo_map) noexcept
{
  ZoneScopedN ("Graph cycle");

  if (scheduler_ == nullptr)
    {
      return;
    }

  /* only set the kickoff thread when called from a realtime context during
   * audio processing (sometimes this method is called from the UI thread to
   * force some processing) */
  if (realtime_context)
    process_kickoff_thread_ = current_thread_id.get ();

  global_offset_ = max_route_playback_latency_ - remaining_latency_preroll;

  preprocess_at_start_of_cycle (time_nfo);

  scheduler_->run_cycle (
    time_nfo, remaining_latency_preroll, current_transport_state, tempo_map);
}

void
DspGraphDispatcher::recalc_graph (bool soft)
{
  z_info ("Recalculating processing graph{}...", soft ? " (soft)" : "");

  const auto device_info = hw_interface_.get_device_info ();
  const auto sample_rate = device_info.sample_rate;
  const auto buffer_size = device_info.block_length;

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
      for (const auto &processable : terminal_processables_provider_ ())
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
        run_on_main_thread_, sample_rate, buffer_size, true, workgroup_);
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

void
DspGraphDispatcher::clear_graph ()
{
  if (!scheduler_)
    return;

  const auto device_info = hw_interface_.get_device_info ();

  run_function_with_engine_lock_ ([&] () {
    scheduler_->rechain_from_node_collection (
      graph::GraphNodeCollection{}, device_info.sample_rate,
      device_info.block_length);
  });
}
}
