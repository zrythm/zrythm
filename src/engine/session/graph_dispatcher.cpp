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

#include "zrythm-config.h"

#include "dsp/graph.h"
#include "dsp/port.h"
#include "engine/device_io/engine.h"
#include "engine/session/graph_dispatcher.h"
#include "engine/session/project_graph_builder.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
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

  // fill live key-press events for the currently active piano roll
  {
    auto &midi_events = audio_engine_->midi_editor_manual_press_;
    if (time_nfo.local_offset_ == 0 && CLIP_EDITOR->has_region ())
      {
        auto clip_editor_track_var =
          CLIP_EDITOR->get_region_and_track ()->second;
        std::visit (
          [&] (auto &&track) {
            using TrackT = base_type<decltype (track)>;
            if constexpr (
              std::derived_from<TrackT, structure::tracks::PianoRollTrack>)
              {
                auto &target_port = track->processor_->get_midi_in_port ();
                /* if not set to "all channels", filter-append */
                if (track->allow_only_specific_midi_channels ())
                  {
                    target_port.midi_events_.active_events_.append_w_filter (
                      midi_events, track->midi_channels_to_allow (),
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
  }
}

void
DspGraphDispatcher::start_cycle (EngineProcessTimeInfo time_nfo)
{
  assert (scheduler_);
  assert (time_nfo.local_offset_ + time_nfo.nframes_ <= audio_engine_->nframes_);

  /* only set the kickoff thread when not called from the gtk thread (sometimes
   * this is called from the gtk thread to force some processing) */
  if (!ZRYTHM_IS_QT_THREAD)
    process_kickoff_thread_ = current_thread_id.get ();

  SemaphoreRAII<> sem (graph_access_sem_);
  if (!sem.is_acquired ())
    {
      z_info ("graph access is busy, returning...");
      return;
    }

  global_offset_ =
    max_route_playback_latency_ - audio_engine_->remaining_latency_preroll_;
  time_nfo_ = time_nfo;
  z_return_if_fail_cmp (
    time_nfo.g_start_frame_w_offset_, >=, time_nfo.g_start_frame_);

  callback_in_progress_ = true;

  preprocess_at_start_of_cycle (time_nfo_);

  scheduler_->run_cycle (time_nfo_, audio_engine_->remaining_latency_preroll_);
  callback_in_progress_ = false;
}

void
DspGraphDispatcher::recalc_graph (bool soft)
{
  z_info ("Recalculating{}...", soft ? " (soft)" : "");

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
                for (const auto &child_node : node->childnodes_)
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
    for (const auto &cur_tr : PROJECT->getTracklist ()->get_track_span ())
      {
        std::visit (
          [&] (auto &&tr) {
            z_return_if_fail (tr);
            using TrackT = base_type<decltype (tr)>;

            if constexpr (
              std::derived_from<TrackT, structure::tracks::RecordableTrack>)
              {
                tr->processor_->set_handle_recording_callback (
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
    ProjectGraphBuilder builder (*PROJECT);
    dsp::graph::Graph   graph;
    builder.build_graph (graph);

    set_caches_on_graph_nodes (graph.get_nodes ().graph_nodes_);

    // TODO
    // PROJECT->clip_editor_->set_caches ();
    TRACKLIST->get_track_span ().set_caches (ALL_CACHE_TYPES);
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
      graph_access_sem_.acquire ();
      scheduler_->get_nodes ().update_latencies ();
      graph_access_sem_.release ();
    }
  else
    {
      bool running = AUDIO_ENGINE->run_.load ();
      AUDIO_ENGINE->run_.store (false);
      while (AUDIO_ENGINE->cycle_running_.load ())
        {
          std::this_thread::sleep_for (std::chrono::milliseconds (10));
        }
      rebuild_graph ();
      AUDIO_ENGINE->run_.store (running);
    }

  z_info ("done");
}
}
