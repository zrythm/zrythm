// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph.h"
#include "dsp/graph_export.h"
#include "engine/session/project_graph_builder.h"
#include "gui/backend/backend/project.h"
#include "structure/tracks/channel_subgraph_builder.h"

using namespace zrythm;

void
ProjectGraphBuilder::build_graph_impl (dsp::graph::Graph &graph)
{
  z_debug ("building graph...");

  const bool drop_unnecessary_ports = drop_unnecessary_ports_;

  /* ========================
   * first add all the nodes
   * ======================== */

  const auto &project = project_;
  const auto &engine = project_.audio_engine_;
  // auto *      sample_processor = engine->sample_processor_.get ();
  auto * tracklist = project_.tracklist_;
  auto * monitor_fader = engine->control_room_->monitor_fader_.get ();
  // auto *      hw_in_processor = engine->hw_in_processor_.get ();
  auto * transport = project.transport_;

  const auto add_node_for_processable = [&] (auto &processable) {
    return graph.add_node_for_processable (processable, *transport);
  };

  const auto connect_ports =
    [&] (const dsp::Port::Uuid src_id, const dsp::Port::Uuid &dest_id) {
      dsp::add_connection_for_ports (
        graph, { src_id, project.get_port_registry () },
        { dest_id, project.get_port_registry () });
    };

  /* add the sample processor */
// TODO
#if 0
  add_node_for_processable (*sample_processor);
  for (const auto &fader_out : sample_processor->fader_->get_output_ports ())
    {
      add_node_for_processable (*fader_out.get_object_as<dsp::AudioPort> ());
    }
#endif

  /* add the monitor fader */
  dsp::ProcessorGraphBuilder::add_nodes (graph, *transport, *monitor_fader);

  /* add the initial processor */
  auto * initial_processor_node = graph.add_initial_processor (*transport);

  /* add the hardware input processor */
  // add_node_for_processable (*hw_in_processor);

  /* add plugins */
  for (const auto &cur_tr : tracklist->get_track_span ())
    {
      std::visit (
        [&] (auto &&tr) {
          z_return_if_fail (tr);
          using TrackT = base_type<decltype (tr)>;

          if constexpr (
            std::derived_from<TrackT, structure::tracks::ProcessableTrack>)
            {
              /* add the track processor */
              add_node_for_processable (*tr->processor_);
            }

          /* handle modulator track */
          if constexpr (
            std::is_same_v<structure::tracks::ModulatorTrack, TrackT>)
            {
              /* add plugins */
              for (const auto &pl_var : tr->get_modulator_span ())
                {
                  std::visit (
                    [&] (auto &&pl) {
                      if (pl->deleting_)
                        return;

                      dsp::ProcessorGraphBuilder::add_nodes (
                        graph, *transport, *pl);
                    },
                    pl_var);
                }

              /* add macro processors */
              for (const auto &mp : tr->get_modulator_macro_processors ())
                {
                  add_node_for_processable (*mp);
                }
            }

          if constexpr (
            std::derived_from<TrackT, structure::tracks::ChannelTrack>)
            {
              auto &channel = tr->channel_;
              structure::tracks::ChannelSubgraphBuilder::add_nodes (
                graph, *transport, *channel, drop_unnecessary_ports);
            }
        },
        cur_tr);
    }

  auto add_port = [&] (
                    dsp::PortPtrVariant port_var, dsp::PortConnectionsManager &mgr,
                    const bool drop_if_unnecessary) {
    return std::visit (
      [&] (auto &&port) -> dsp::graph::GraphNode * {
        using PortT = base_type<decltype (port)>;

        /* reset port sources/dests */
        dsp::PortConnectionsManager::ConnectionsVector srcs;
        mgr.get_sources (&srcs, port->get_uuid ());
        port->port_sources_.clear ();
        for (const auto &conn : srcs)
          {
            auto src_var = project.find_port_by_id (conn->src_id_);
            assert (src_var.has_value ());
            std::visit (
              [&] (auto &&src) {
                using SourcePortT = base_type<decltype (src)>;
                if constexpr (
                  (std::is_same_v<PortT, dsp::CVPort>
                   && std::is_same_v<SourcePortT, dsp::CVPort>)
                  || (std::is_same_v<PortT, dsp::MidiPort> && std::is_same_v<SourcePortT, dsp::MidiPort>)
                  || (std::is_same_v<PortT, dsp::AudioPort> && std::is_same_v<SourcePortT, dsp::AudioPort>) )
                  {
                    port->port_sources_.push_back (
                      std::make_pair (src, utils::clone_unique (*conn)));
                  }
              },
              src_var.value ());
          }

        dsp::PortConnectionsManager::ConnectionsVector dests;
        mgr.get_dests (&dests, port->get_uuid ());

      // TODO
#if 0
        /* drop ports without sources and dests */
        if (drop_if_unnecessary && dests.empty () && srcs.empty ())
          {
            return nullptr;
          }
#endif

        return add_node_for_processable (*port);
      },
      port_var);
  };

  /* add ports */
  std::vector<dsp::Port *> ports;
  project.get_all_ports (ports);
  for (auto * port : ports)
    {
      assert (port);
      add_port (
        convert_to_variant<dsp::PortPtrVariant> (port),
        *project.port_connections_manager_, drop_unnecessary_ports);
    }

    /* ========================
     * now connect them
     * ======================== */

// TODO: connect the sample processor
#if 0
  {
    auto * node =
      graph.get_nodes ().find_node_for_processable (*sample_processor);
    z_return_if_fail (node);
    iterate_tuple (
      [&] (const auto &port) {
        auto * node2 = graph.get_nodes ().find_node_for_processable (port);
        node->connect_to (*node2);
      },
      sample_processor->fader_->get_stereo_out_ports ());
  }
#endif

  /* connect the monitor fader */
  dsp::ProcessorGraphBuilder::add_connections (graph, *monitor_fader);

// TODO: connect the sample processor output to the monitor fader output so we
// hear the samples
#if 0
  {
    const auto &sp_fader_outs = sample_processor->fader_->get_output_ports ();
    const auto &monitor_fader_outs = monitor_fader->get_output_ports ();
    for (
      const auto &[sp_out, mf_out] :
      std::views::zip (sp_fader_outs, monitor_fader_outs))
      {
        auto * sp_out_node = graph.get_nodes ().find_node_for_processable (
          *sp_out.get_object_as<dsp::AudioPort> ());
        auto * mf_out_node = graph.get_nodes ().find_node_for_processable (
          *mf_out.get_object_as<dsp::AudioPort> ());
        sp_out_node->connect_to (*mf_out_node);
      }
  }
#endif

  // connect monitor fader output to engine monitor output
  {
    const auto &mf_outs = monitor_fader->get_output_ports ();
    const std::array<dsp::PortUuidReference, 2> monitor_outs = {
      engine->monitor_out_left_.value (), engine->monitor_out_right_.value ()
    };
    for (
      const auto &[mf_out, monitor_out] :
      std::views::zip (mf_outs, monitor_outs))
      {
        auto * mf_out_node = graph.get_nodes ().find_node_for_processable (
          *mf_out.get_object_as<dsp::AudioPort> ());
        auto * monitor_out_node = graph.get_nodes ().find_node_for_processable (
          *monitor_out.get_object_as<dsp::AudioPort> ());
        mf_out_node->connect_to (*monitor_out_node);
      }
  }

  /* connect the HW input processor */
#if 0
  dsp::graph::GraphNode * hw_processor_node =
    graph.get_nodes ().find_node_for_processable (*hw_in_processor);
  for (auto &port : hw_in_processor->audio_ports_)
    {
      auto node2 = graph.get_nodes ().find_node_for_processable (*port);
      z_warn_if_fail (node2);
      hw_processor_node->connect_to (*node2);
    }
  for (auto &port : hw_in_processor->midi_ports_)
    {
      auto node2 = graph.get_nodes ().find_node_for_processable (*port);
      hw_processor_node->connect_to (*node2);
    }
#endif

  /* connect MIDI editor manual press */
  {
// TODO
#if 0
    auto node2 = graph.get_nodes ().find_node_for_processable (
      *engine->midi_editor_manual_press_);
    node2->connect_to (*initial_processor_node);
#endif
  }

  /* connect the transport ports */
  {
// TODO
#if 0
    auto node = graph.get_nodes ().find_node_for_processable (*transport->roll_);
    node->connect_to (*initial_processor_node);
    node = graph.get_nodes ().find_node_for_processable (*transport->stop_);
    node->connect_to (*initial_processor_node);
    node = graph.get_nodes ().find_node_for_processable (*transport->backward_);
    node->connect_to (*initial_processor_node);
    node = graph.get_nodes ().find_node_for_processable (*transport->forward_);
    node->connect_to (*initial_processor_node);
    node =
      graph.get_nodes ().find_node_for_processable (*transport->loop_toggle_);
    node->connect_to (*initial_processor_node);
    node =
      graph.get_nodes ().find_node_for_processable (*transport->rec_toggle_);
    node->connect_to (*initial_processor_node);
#endif
  }

  /* connect tracks */
  for (const auto &cur_tr : tracklist->get_track_span ())
    {
      std::visit (
        [&] (auto &tr) {
          using TrackT = base_type<decltype (tr)>;

          // only processable tracks are part of the graph (essentially every
          // track besides MarkerTrack)
          if constexpr (
            std::derived_from<TrackT, structure::tracks::ProcessableTrack>)
            {
              {
                /* connect the track processor */
                auto * const track_processor_node =
                  graph.get_nodes ().find_node_for_processable (*tr->processor_);
                assert (track_processor_node);
                if (tr->get_input_signal_type () == dsp::PortType::Audio)
                  {

                    auto track_processor = tr->processor_.get ();
                    iterate_tuple (
                      [&] (const auto &port) {
                        auto node2 =
                          graph.get_nodes ().find_node_for_processable (port);
                        node2->connect_to (*track_processor_node);
                        initial_processor_node->connect_to (*node2);
                      },
                      track_processor->get_stereo_in_ports ());
                    iterate_tuple (
                      [&] (const auto &port) {
                        auto node2 =
                          graph.get_nodes ().find_node_for_processable (port);
                        track_processor_node->connect_to (*node2);
                      },
                      track_processor->get_stereo_out_ports ());
                  }
                else if (tr->get_input_signal_type () == dsp::PortType::Event)
                  {
                    auto track_processor = tr->processor_.get ();

                    if constexpr (
                      std::derived_from<TrackT, structure::tracks::PianoRollTrack>
                      || std::is_same_v<TrackT, structure::tracks::ChordTrack>)
                      {
                        // connect piano roll
                        auto node2 =
                          graph.get_nodes ().find_node_for_processable (
                            track_processor->get_piano_roll_port ());
                        z_return_if_fail (node2);
                        node2->connect_to (*track_processor_node);
                      }

                    auto node2 = graph.get_nodes ().find_node_for_processable (
                      track_processor->get_midi_in_port ());
                    node2->connect_to (*track_processor_node);
                    initial_processor_node->connect_to (*node2);
                    node2 = graph.get_nodes ().find_node_for_processable (
                      track_processor->get_midi_out_port ());
                    track_processor_node->connect_to (*node2);
                  }

                if constexpr (
                  std::is_same_v<TrackT, structure::tracks::ModulatorTrack>)
                  {
                    initial_processor_node->connect_to (*track_processor_node);

                    for (const auto &pl_var : tr->get_modulator_span ())
                      {
                        std::visit (
                          [&] (auto &&pl) {
                            if (pl && !pl->deleting_)
                              {
                                dsp::ProcessorGraphBuilder::add_connections (
                                  graph, *pl);
                                for (
                                  const auto &pl_port_var :
                                  pl->get_input_port_span ())
                                  {
                                    std::visit (
                                      [&] (auto &&pl_port) {
                                        // z_return_if_fail (pl_port->get_plugin
                                        // (true)
                                        // != nullptr);
                                        auto port_node =
                                          graph.get_nodes ()
                                            .find_node_for_processable (*pl_port);
                                        if (!port_node)
                                          {
                                            z_error (
                                              "failed to find node for port {}",
                                              pl_port->get_label ());
                                            return;
                                          }
                                        track_processor_node->connect_to (
                                          *port_node);
                                      },
                                      pl_port_var);
                                  }
                              }
                          },
                          pl_var);
                      }

                    /* connect the modulator macro processors */
                    for (const auto &mmp : tr->get_modulator_macro_processors ())
                      {
                        auto mmp_node =
                          graph.get_nodes ().find_node_for_processable (*mmp);
                        z_return_if_fail (mmp_node);

                        {
                          auto * const node2 =
                            graph.get_nodes ().find_node_for_processable (
                              mmp->get_cv_in_port ());
                          node2->connect_to (*mmp_node);
                        }

                        auto * const node2 =
                          graph.get_nodes ().find_node_for_processable (
                            mmp->get_cv_out_port ());
                        mmp_node->connect_to (*node2);
                      }
                  }
              }

              // connect the channel
              if constexpr (
                std::derived_from<TrackT, structure::tracks::ChannelTrack>)
                {
                  std::vector<dsp::PortUuidReference> track_processor_outs;
                  if (tr->get_input_signal_type () == dsp::PortType::Event)
                    {
                      track_processor_outs.push_back (
                        *tr->processor_->midi_out_id_);
                    }
                  else if (tr->get_input_signal_type () == dsp::PortType::Audio)
                    {
                      track_processor_outs.push_back (
                        *tr->processor_->stereo_out_left_id_);
                      track_processor_outs.push_back (
                        *tr->processor_->stereo_out_right_id_);
                    }
                  auto &ch = tr->channel_;
                  structure::tracks::ChannelSubgraphBuilder::add_connections (
                    graph, *ch, track_processor_outs, drop_unnecessary_ports);

                  // Connect master track output to monitor fader input
                  if constexpr (
                    std::is_same_v<TrackT, structure::tracks::MasterTrack>)
                    {
                      const auto &ch_stereo_outs = ch->get_stereo_out_ports ();
                      const auto &monitor_fader_ins =
                        monitor_fader->get_stereo_in_ports ();
                      connect_ports (
                        ch_stereo_outs.first.get_uuid (),
                        monitor_fader_ins.first.get_uuid ());
                      connect_ports (
                        ch_stereo_outs.second.get_uuid (),
                        monitor_fader_ins.second.get_uuid ());
                    }
                }
            }
        },
        cur_tr);
    }

  auto connect_port = [&]<typename PortT> (PortT &p)
    requires std::derived_from<PortT, dsp::Port>
  {
    const auto &mgr = *project.port_connections_manager_;
    dsp::PortConnectionsManager::ConnectionsVector srcs;
    mgr.get_sources (&srcs, p.get_uuid ());
    dsp::PortConnectionsManager::ConnectionsVector dests;
    mgr.get_dests (&dests, p.get_uuid ());

    auto * node = graph.get_nodes ().find_node_for_processable (p);
    for (const auto &conn : srcs)
      {
        auto src_var = project.find_port_by_id (conn->src_id_);
        assert (src_var.has_value ());
        std::visit (
          [&] (auto &&src) {
            auto node2 = graph.get_nodes ().find_node_for_processable (*src);
            assert (node);
            assert (node2);
            node2->connect_to (*node);
          },
          src_var.value ());
      }
    for (const auto &conn : dests)
      {
        auto dest_var = project.find_port_by_id (conn->dest_id_);
        assert (dest_var.has_value ());
        std::visit (
          [&] (auto &&dest) {
            auto node2 = graph.get_nodes ().find_node_for_processable (*dest);
            assert (node);
            assert (node2);
            node->connect_to (*node2);
          },
          dest_var.value ());
      }
  };

  for (auto &port : ports)
    {
      std::visit (
        [&] (auto &&p) { connect_port (*p); },
        convert_to_variant<dsp::PortPtrVariant> (port));
    }

  z_debug (
    "done building graph: {}",
    dsp::graph::GraphExport::export_to_dot (graph, true));
}

bool
ProjectGraphBuilder::can_ports_be_connected (
  Project         &project,
  const dsp::Port &src,
  const dsp::Port &dest)
{
  engine::device_io::AudioEngine::State state{};
  project.audio_engine_->wait_for_pause (state, false, true);

  z_debug ("validating for {} to {}", src.get_label (), dest.get_label ());

  ProjectGraphBuilder builder (project, false);
  dsp::graph::Graph   graph;
  builder.build_graph (graph);

  /* connect the src/dest if not NULL */
  /* this code is only for creating graphs to test if the connection between
   * src->dest is valid */
  bool success = std::visit (
    [&] (auto &&srcp, auto &&destp) {
      auto * node = graph.get_nodes ().find_node_for_processable (*srcp);
      z_return_val_if_fail (node, false);
      auto * node2 = graph.get_nodes ().find_node_for_processable (*destp);
      z_return_val_if_fail (node2, false);
      node->connect_to (*node2);
      z_return_val_if_fail (!node->terminal_, false);
      z_return_val_if_fail (!node2->initial_, false);
      return true;
    },
    convert_to_variant<dsp::PortPtrVariant> (&src),
    convert_to_variant<dsp::PortPtrVariant> (&dest));
  if (!success)
    {
      z_info ("failed to connect {} to {}", src.get_label (), dest.get_label ());
      return false;
    }

  bool valid = graph.is_valid ();

  z_debug ("valid {}", valid);

  project.audio_engine_->resume (state);

  return valid;
}
