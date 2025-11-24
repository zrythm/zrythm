// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
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
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * ---
 */

#include "dsp/graph.h"
#include "dsp/graph_pruner.h"
#include "dsp/port.h"
#include "engine/device_io/engine.h"
#include "engine/session/graph_dispatcher.h"
#include "engine/session/project_graph_builder.h"
#include "gui/backend/backend/project.h"
#include "structure/tracks/track_processor.h"
#include "structure/tracks/tracklist.h"
#include "utils/debug.h"

namespace zrythm::engine::session
{
DspGraphDispatcher::DspGraphDispatcher (device_io::AudioEngine * engine)
    : audio_engine_ (engine)
{
}

nframes_t
DspGraphDispatcher::get_max_route_playback_latency ()
{
  z_return_val_if_fail (scheduler_, 0);
  max_route_playback_latency_ =
    scheduler_->get_nodes ().get_max_route_playback_latency ();

  return max_route_playback_latency_;
}

void
DspGraphDispatcher::preprocess_at_start_of_cycle (
  const EngineProcessTimeInfo &time_nfo)
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
  const dsp::ITransport &current_transport_state,
  EngineProcessTimeInfo  time_nfo,
  nframes_t              remaining_latency_preroll,
  bool                   realtime_context) noexcept
{
  assert (scheduler_);

  /* only set the kickoff thread when called from a realtime context during
   * audio processing (sometimes this method is called from the UI thread to
   * force some processing) */
  if (realtime_context)
    process_kickoff_thread_ = current_thread_id.get ();

  if (!try_acquire_graph_access ())
    {
      z_info ("graph access is busy, returning...");
      return;
    }

  global_offset_ = max_route_playback_latency_ - remaining_latency_preroll;
  time_nfo_ = time_nfo;
  z_return_if_fail_cmp (
    time_nfo.g_start_frame_w_offset_, >=, time_nfo.g_start_frame_);

  callback_in_progress_ = true;

  preprocess_at_start_of_cycle (time_nfo_);

  scheduler_->run_cycle (
    time_nfo_, remaining_latency_preroll, current_transport_state);
  callback_in_progress_ = false;

  release_graph_access ();
}

void
DspGraphDispatcher::recalc_graph (bool soft)
{
  z_info ("Recalculating processing graph{}...", soft ? " (soft)" : "");

  auto       device_mgr = audio_engine_->get_device_manager ();
  const auto current_device = device_mgr->getCurrentAudioDevice ();

  // set various caches on processors and ports inside the graph
  const auto set_caches_on_graph_nodes = [&] (const auto &graph_nodes) {
    // collect all ports while going through each node
    std::vector<std::pair<dsp::PortPtrVariant, dsp::PortPtrVariant>>
      port_connections_in_graph;
    for (auto &node : graph_nodes)
      {
        if (
          auto * port_base =
            dynamic_cast<dsp::Port *> (&node->get_processable ());
          port_base != nullptr)
          {
            std::visit (
              [&] (auto &&port) {
                using PortT = base_type<decltype (port)>;
                port->port_sources_.clear ();
                for (const auto &child_node : node->feeds ())
                  {
                    auto * dest_port = dynamic_cast<PortT *> (
                      &child_node.get ().get_processable ());
                    if (dest_port != nullptr)
                      {
                        port_connections_in_graph.emplace_back (port, dest_port);
                      }
                  }
              },
              convert_to_variant<dsp::PortPtrVariant> (port_base));
          }
      }

    // set up caches for ports
    for (const auto &[src_port_var, dest_port_var] : port_connections_in_graph)
      {
        std::visit (
          [&] (auto &&src_port) {
            using PortT = base_type<decltype (src_port)>;
            auto * dest_port = std::get<PortT *> (dest_port_var);
            dest_port->port_sources_.push_back (
              std::make_pair (
                src_port,
                std::make_unique<dsp::PortConnection> (
                  src_port->get_uuid (), dest_port->get_uuid (), 1.f, true,
                  true)));
          },
          src_port_var);
      }

    // set appropriate callbacks
    for (
      const auto &cur_tr :
      PROJECT->tracklist ()->collection ()->get_track_span ())
      {
        std::visit (
          [&] (auto &&tr) {
            z_return_if_fail (tr);
            using TrackT = base_type<decltype (tr)>;

            if constexpr (structure::tracks::RecordableTrack<TrackT>)
              {
                tr->get_track_processor ()->set_handle_recording_callback (
                  [tr] (
                    const EngineProcessTimeInfo &time_nfo,
                    const dsp::MidiEventVector * midi_events,
                    std::optional<
                      structure::tracks::TrackProcessor::ConstStereoPortPair>
                      stereo_ports) {
                    RECORDING_MANAGER->handle_recording (
                      tr, time_nfo, midi_events, stereo_ports);
                  });
              }
          },
          cur_tr);
      }
  };

  const auto sample_rate =
    static_cast<sample_rate_t> (current_device->getCurrentSampleRate ());
  const auto buffer_size = current_device->getCurrentBufferSizeSamples ();

  const auto rebuild_graph = [&] () {
    graph_setup_in_progress_.store (true);
    dsp::graph::Graph graph;

    // Build graph
    {
      ProjectGraphBuilder builder (*PROJECT);
      builder.build_graph (graph);
      z_debug (
        "Built graph (before pruning): {}",
        dsp::graph::GraphExport::export_to_dot (graph, true));
    }

    // Prune graph
    {
      std::vector<std::reference_wrapper<dsp::graph::GraphNode>> terminals;
      auto  &monitor_out = audio_engine_->get_monitor_out_port ();
      auto * monitor_out_node =
        graph.get_nodes ().find_node_for_processable (monitor_out);
      terminals.emplace_back (*monitor_out_node);
      dsp::graph::GraphPruner::prune_graph_to_terminals (graph, terminals);
      z_debug (
        "Built graph (pruned): {}",
        dsp::graph::GraphExport::export_to_dot (graph, true));
    }

    set_caches_on_graph_nodes (graph.get_nodes ().graph_nodes_);

    // TODO
    // PROJECT->clip_editor_->set_caches ();
    TRACKLIST->collection ()->get_track_span ().set_caches (ALL_CACHE_TYPES);
    scheduler_->rechain_from_node_collection (
      graph.steal_nodes (), sample_rate, buffer_size);

    graph_setup_in_progress_.store (false);
  };

  if (!scheduler_ && !soft)
    {
      scheduler_ = std::make_unique<dsp::graph::GraphScheduler> (
        sample_rate, buffer_size, std::nullopt,
        device_mgr->getDeviceAudioWorkgroup ());
      rebuild_graph ();
      scheduler_->start_threads ();
      return;
    }

  if (soft)
    {
      wait_for_graph_access ();
      scheduler_->get_nodes ().update_latencies ();
      release_graph_access ();
    }
  else
    {
      bool running = audio_engine_->running ();
      audio_engine_->set_running (false);
      auto lock = audio_engine_->get_processing_lock ();
      rebuild_graph ();
      audio_engine_->set_running (running);
    }

  z_info ("Processing graph ready");
}
}
