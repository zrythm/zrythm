// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph.h"
#include "dsp/graph_export.h"
#include "engine/session/project_graph_builder.h"
#include "gui/backend/backend/project.h"

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
  auto *      sample_processor = engine->sample_processor_.get ();
  auto *      tracklist = project_.tracklist_;
  auto *      monitor_fader = engine->control_room_->monitor_fader_.get ();
  // auto *      hw_in_processor = engine->hw_in_processor_.get ();
  auto * transport = project.transport_;

  auto add_node_for_processable = [&] (auto &processable) {
    return graph.add_node_for_processable (processable, *transport);
  };

  /* add the sample processor */
  add_node_for_processable (*sample_processor);

  /* add the monitor fader */
  add_node_for_processable (*monitor_fader);

  /* add the initial processor */
  auto * initial_processor_node = graph.add_initial_processor (*transport);

  /* add the hardware input processor */
  // add_node_for_processable (*hw_in_processor);

  auto add_plugin = [&] (auto &pl) {
    z_return_if_fail (!pl.deleting_);
    std::visit (
      [&] (auto &&plugin) {
        if (!plugin->in_ports_.empty () || !plugin->out_ports_.empty ())
          {
            add_node_for_processable (*plugin);
          }
      },
      convert_to_variant<zrythm::gui::old_dsp::plugins::PluginPtrVariant> (&pl));
  };

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

                      add_plugin (*pl);
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

              /* add the fader */
              auto * fader = channel->fader ();
              add_node_for_processable (*fader);

              /* add the prefader */
              auto * prefader = channel->preFader ();
              add_node_for_processable (*prefader);

              /* add plugins */
              std::vector<zrythm::gui::old_dsp::plugins::Plugin *> plugins;
              channel->get_plugins (plugins);
              for (auto * pl : plugins)
                {
                  if (!pl || pl->deleting_)
                    continue;

                  add_plugin (*pl);
                }

              /* add sends */
              if (
                tr->get_output_signal_type () == dsp::PortType::Audio
                || tr->get_output_signal_type () == dsp::PortType::Event)
                {
                  for (auto &send : channel->sends_)
                    {
                      /* note that we add sends even if empty so that graph
                       * renders properly */
                      add_node_for_processable (*send);
                    }
                }
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

  auto connect_plugin = [&] (gui::old_dsp::plugins::Plugin &pl) {
    z_return_if_fail (!pl.deleting_);
    auto * pl_node = graph.get_nodes ().find_node_for_processable (pl);
    z_return_if_fail (pl_node);
    for (const auto port_var : pl.get_input_port_span ())
      {
        std::visit (
          [&] (auto &&port) {
            auto * port_node =
              graph.get_nodes ().find_node_for_processable (*port);
            assert (port_node);
            port_node->connect_to (*pl_node);
          },
          port_var);
      }
    for (const auto port_var : pl.get_output_port_span ())
      {
        std::visit (
          [&] (auto &&port) {
            // z_return_if_fail (port->get_plugin (true) != nullptr);
            auto * port_node =
              graph.get_nodes ().find_node_for_processable (*port);
            z_return_if_fail (port_node);
            pl_node->connect_to (*port_node);
          },
          port_var);
      }
  };

  /* connect the sample processor */
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

  /* connect the monitor fader */
  {
    auto * node = graph.get_nodes ().find_node_for_processable (*monitor_fader);
    iterate_tuple (
      [&] (const auto &port) {
        auto * node2 = graph.get_nodes ().find_node_for_processable (port);
        node2->connect_to (*node);
      },
      monitor_fader->get_stereo_in_ports ());
    iterate_tuple (
      [&] (const auto &port) {
        auto * node2 = graph.get_nodes ().find_node_for_processable (port);
        node->connect_to (*node2);
      },
      monitor_fader->get_stereo_out_ports ());
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
                                connect_plugin (*pl);
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
                  auto &ch = tr->channel_;
                  auto &prefader = ch->prefader_;
                  auto &fader = ch->fader_;

                  /* connect the fader */
                  auto * const fader_node =
                    graph.get_nodes ().find_node_for_processable (*fader);
                  z_warn_if_fail (fader_node);
                  if (
                    fader->type_ == structure::tracks::Fader::Type::AudioChannel)
                    {
                      /* connect ins */
                      iterate_tuple (
                        [&] (const auto &port) {
                          auto node2 =
                            graph.get_nodes ().find_node_for_processable (port);
                          node2->connect_to (*fader_node);
                        },
                        fader->get_stereo_in_ports ());

                      /* connect outs */
                      iterate_tuple (
                        [&] (const auto &port) {
                          auto node2 =
                            graph.get_nodes ().find_node_for_processable (port);
                          fader_node->connect_to (*node2);
                        },
                        fader->get_stereo_out_ports ());
                    }
                  else if (
                    fader->type_ == structure::tracks::Fader::Type::MidiChannel)
                    {
                      auto node2 = graph.get_nodes ().find_node_for_processable (
                        fader->get_midi_in_port ());
                      node2->connect_to (*fader_node);
                      node2 = graph.get_nodes ().find_node_for_processable (
                        fader->get_midi_out_port ());
                      fader_node->connect_to (*node2);
                    }

                  /* connect the prefader */
                  auto * const prefader_node =
                    graph.get_nodes ().find_node_for_processable (*prefader);
                  z_warn_if_fail (prefader_node);
                  if (
                    prefader->type_
                    == structure::tracks::Fader::Type::AudioChannel)
                    {
                      iterate_tuple (
                        [&] (const auto &port) {
                          auto * node2 =
                            graph.get_nodes ().find_node_for_processable (port);
                          node2->connect_to (*prefader_node);
                        },
                        prefader->get_stereo_in_ports ());
                      iterate_tuple (
                        [&] (const auto &port) {
                          auto node2 =
                            graph.get_nodes ().find_node_for_processable (port);
                          prefader_node->connect_to (*node2);
                        },
                        prefader->get_stereo_out_ports ());
                    }
                  else if (
                    prefader->type_
                    == structure::tracks::Fader::Type::MidiChannel)
                    {
                      auto * node2 =
                        graph.get_nodes ().find_node_for_processable (
                          prefader->get_midi_in_port ());
                      node2->connect_to (*prefader_node);
                      node2 = graph.get_nodes ().find_node_for_processable (
                        prefader->get_midi_out_port ());
                      prefader_node->connect_to (*node2);
                    }

                  std::vector<zrythm::gui::old_dsp::plugins::Plugin *> plugins;
                  ch->get_plugins (plugins);
                  for (auto * const pl : plugins)
                    {
                      if (pl && !pl->deleting_)
                        {
                          connect_plugin (*pl);
                        }
                    }

                  for (auto &send : ch->sends_)
                    {
                      /* note that we do not skip empty sends because then port
                       * connection validation will not detect invalid
                       * connections properly */
                      auto * const send_node =
                        graph.get_nodes ().find_node_for_processable (*send);

                      if (tr->get_output_signal_type () == dsp::PortType::Event)
                        {
                          auto * node2 =
                            graph.get_nodes ().find_node_for_processable (
                              send->get_midi_in_port ());
                          node2->connect_to (*send_node);
                          node2 = graph.get_nodes ().find_node_for_processable (
                            send->get_midi_out_port ());
                          send_node->connect_to (*node2);
                        }
                      else if (
                        tr->get_output_signal_type () == dsp::PortType::Audio)
                        {
                          iterate_tuple (
                            [&] (const auto &port) {
                              auto * node2 =
                                graph.get_nodes ().find_node_for_processable (
                                  port);
                              node2->connect_to (*send_node);
                            },
                            send->get_stereo_in_ports ());
                          iterate_tuple (
                            [&] (const auto &port) {
                              auto * node2 =
                                graph.get_nodes ().find_node_for_processable (
                                  port);
                              send_node->connect_to (*node2);
                            },
                            send->get_stereo_out_ports ());
                        }
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
