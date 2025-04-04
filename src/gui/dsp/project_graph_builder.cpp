// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph.h"
#include "gui/backend/backend/project.h"
#include "gui/dsp/project_graph_builder.h"

using namespace zrythm;

void
ProjectGraphBuilder::build_graph_impl (dsp::Graph &graph)
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
  auto *      hw_in_processor = engine->hw_in_processor_.get ();
  auto *      transport = project.transport_;

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
  add_node_for_processable (*hw_in_processor);

  auto add_plugin = [&] (auto &graph, auto &pl) {
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

          /* add the track */
          add_node_for_processable (*tr);

          /* handle modulator track */
          if constexpr (std::is_same_v<ModulatorTrack, TrackT>)
            {
              /* add plugins */
              for (const auto &pl_var : tr->get_modulator_span ())
                {
                  std::visit (
                    [&] (auto &&pl) {
                      if (pl->deleting_)
                        return;

                      add_plugin (graph, *pl);
                    },
                    pl_var);
                }

              /* add macro processors */
              for (const auto &mp : tr->get_modulator_macro_processors ())
                {
                  add_node_for_processable (*mp);
                }
            }

          if constexpr (std::derived_from<TrackT, ChannelTrack>)
            {
              auto &channel = tr->channel_;

              /* add the fader */
              auto * fader = channel->fader_;
              add_node_for_processable (*fader);

              /* add the prefader */
              auto * prefader = channel->prefader_;
              add_node_for_processable (*prefader);

              /* add plugins */
              std::vector<zrythm::gui::old_dsp::plugins::Plugin *> plugins;
              channel->get_plugins (plugins);
              for (auto * pl : plugins)
                {
                  if (!pl || pl->deleting_)
                    continue;

                  add_plugin (graph, *pl);
                }

              /* add sends */
              if (
                tr->out_signal_type_ == dsp::PortType::Audio
                || tr->out_signal_type_ == dsp::PortType::Event)
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

  auto add_port =
    [&] (
      auto &graph, PortPtrVariant port_var, PortConnectionsManager &mgr,
      const bool drop_if_unnecessary) {
      return std::visit (
        [&] (auto &&port) -> dsp::GraphNode * {
          using PortT = base_type<decltype (port)>;
          auto owner = port->id_->owner_type_;

          /* reset port sources/dests */
          PortConnectionsManager::ConnectionsVector srcs;
          mgr.get_sources_or_dests (&srcs, port->get_uuid (), true);
          port->srcs_.clear ();
          port->src_connections_.clear ();
          for (const auto &conn : srcs)
            {
              auto src_var = project.find_port_by_id (conn->src_id_);
              if (!src_var.has_value ())
                {
                  z_return_val_if_fail (src_var.has_value (), nullptr);
                }
              std::visit (
                [&] (auto &&src) { port->srcs_.push_back (src); },
                src_var.value ());
              z_return_val_if_fail (port->srcs_.back (), nullptr);
              port->src_connections_.emplace_back (conn->clone_unique ());
            }

          PortConnectionsManager::ConnectionsVector dests;
          mgr.get_sources_or_dests (&dests, port->get_uuid (), false);
          port->dests_.clear ();
          port->dest_connections_.clear ();
          for (const auto &conn : dests)
            {
              auto dest_var = project.find_port_by_id (conn->dest_id_);
              z_return_val_if_fail (dest_var.has_value (), nullptr);
              std::visit (
                [&] (auto &&dest) { port->dests_.push_back (dest); },
                dest_var.value ());
              z_return_val_if_fail (port->dests_.back (), nullptr);
              port->dest_connections_.emplace_back (conn->clone_unique ());
            }

          /* skip unnecessary control ports */
          if constexpr (std::is_same_v<PortT, ControlPort>)
            {
              if (
                drop_if_unnecessary
                && ENUM_BITSET_TEST (
                  port->id_->flags_, dsp::PortIdentifier::Flags::Automatable))
                {
                  auto found_at = port->at_;
                  z_return_val_if_fail (found_at, nullptr);
                  if (
                    found_at->region_list_->regions_.empty ()
                    && port->srcs_.empty ())
                    {
                      return nullptr;
                    }
                }
            }

          /* drop ports without sources and dests */
          if (
            drop_if_unnecessary && port->dests_.empty () && port->srcs_.empty ()
            && owner != dsp::PortIdentifier::OwnerType::Plugin
            && owner != dsp::PortIdentifier::OwnerType::Fader
            && owner != dsp::PortIdentifier::OwnerType::TrackProcessor
            && owner != dsp::PortIdentifier::OwnerType::Track
            && owner != dsp::PortIdentifier::OwnerType::ModulatorMacroProcessor
            && owner != dsp::PortIdentifier::OwnerType::Channel
            && owner != dsp::PortIdentifier::OwnerType::ChannelSend
            && owner != dsp::PortIdentifier::OwnerType::AudioEngine
            && owner != dsp::PortIdentifier::OwnerType::HardwareProcessor
            && owner != dsp::PortIdentifier::OwnerType::Transport
            && !(ENUM_BITSET_TEST (
              port->id_->flags_, dsp::PortIdentifier::Flags::ManualPress)))
            {
              return nullptr;
            }

          /* allocate buffers to be used during DSP */
          port->allocate_bufs ();

          return add_node_for_processable (*port);
        },
        port_var);
    };

  /* add ports */
  std::vector<Port *> ports;
  project.get_all_ports (ports);
  for (auto * port : ports)
    {
      z_return_if_fail (port);
      if (port->deleting_)
        continue;

      if (port->id_->owner_type_ == dsp::PortIdentifier::OwnerType::Plugin)
        {
          auto pl_var =
            project.find_plugin_by_id (port->id_->plugin_id_.value ());
          z_return_if_fail (pl_var.has_value ());
          if (std::visit (
                [&] (auto &&pl) { return pl->deleting_; }, pl_var.value ()))
            continue;
        }

      add_port (
        graph, convert_to_variant<PortPtrVariant> (port),
        *project.port_connections_manager_, drop_unnecessary_ports);
    }

  /* ========================
   * now connect them
   * ======================== */

  auto connect_plugin =
    [&] (
      auto &graph, gui::old_dsp::plugins::Plugin &pl,
      bool drop_unnecessary_ports) {
      z_return_if_fail (!pl.deleting_);
      auto * pl_node = graph.get_nodes ().find_node_for_processable (pl);
      z_return_if_fail (pl_node);
      for (const auto port_var : pl.get_input_port_span ())
        {
          std::visit (
            [&] (auto &&port) {
              using PortT = base_type<decltype (port)>;
              // z_return_if_fail (port->get_plugin (true) != nullptr);
              auto * port_node =
                graph.get_nodes ().find_node_for_processable (*port);
              if (
                drop_unnecessary_ports && (port_node == nullptr)
                && std::is_same_v<PortT, ControlPort>)
                {
                  return;
                }
              z_return_if_fail (port_node);
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
  dsp::GraphNode * hw_processor_node =
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

  /* connect MIDI editor manual press */
  {
    auto node2 = graph.get_nodes ().find_node_for_processable (
      *engine->midi_editor_manual_press_);
    node2->connect_to (*initial_processor_node);
  }

  /* connect the transport ports */
  {
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
  }

  /* connect tracks */
  for (const auto &cur_tr : tracklist->get_track_span ())
    {
      std::visit (
        [&] (auto &tr) {
          using TrackT = base_type<decltype (tr)>;

          /* connect the track */
          auto * const track_node =
            graph.get_nodes ().find_node_for_processable (*tr);
          z_return_if_fail (track_node);
          if (tr->in_signal_type_ == dsp::PortType::Audio)
            {
              if constexpr (std::is_same_v<TrackT, AudioTrack>)
                {
                  auto node2 = graph.get_nodes ().find_node_for_processable (
                    tr->processor_->get_mono_port ());
                  node2->connect_to (*track_node);
                  initial_processor_node->connect_to (*node2);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    tr->processor_->get_input_gain_port ());
                  node2->connect_to (*track_node);
                  initial_processor_node->connect_to (*node2);
                }

              if constexpr (std::derived_from<TrackT, ProcessableTrack>)
                {
                  auto track_processor = tr->processor_.get ();
                  iterate_tuple (
                    [&] (const auto &port) {
                      auto node2 =
                        graph.get_nodes ().find_node_for_processable (port);
                      node2->connect_to (*track_node);
                      initial_processor_node->connect_to (*node2);
                    },
                    track_processor->get_stereo_in_ports ());
                  iterate_tuple (
                    [&] (const auto &port) {
                      auto node2 =
                        graph.get_nodes ().find_node_for_processable (port);
                      track_node->connect_to (*node2);
                    },
                    track_processor->get_stereo_out_ports ());
                }
            }
          else if (tr->in_signal_type_ == dsp::PortType::Event)
            {
              if constexpr (std::derived_from<TrackT, ProcessableTrack>)
                {
                  auto track_processor = tr->processor_.get ();

                  if constexpr (
                    std::derived_from<TrackT, PianoRollTrack>
                    || std::is_same_v<TrackT, ChordTrack>)
                    {
                      /* connect piano roll */
                      auto node2 = graph.get_nodes ().find_node_for_processable (
                        track_processor->get_piano_roll_port ());
                      z_return_if_fail (node2);
                      node2->connect_to (*track_node);
                    }

                  auto node2 = graph.get_nodes ().find_node_for_processable (
                    track_processor->get_midi_in_port ());
                  node2->connect_to (*track_node);
                  initial_processor_node->connect_to (*node2);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    track_processor->get_midi_out_port ());
                  track_node->connect_to (*node2);
                }
            }

          if constexpr (std::derived_from<TrackT, PianoRollTrack>)
            {
              auto track_processor = tr->processor_.get ();

              for (const auto channel_index : std::views::iota (0, 16))
                {
                  for (const auto cc_index : std::views::iota (0, 128))
                    {
                      auto node2 = graph.get_nodes ().find_node_for_processable (
                        track_processor->get_midi_cc_port (
                          channel_index, cc_index));
                      if (node2)
                        {
                          node2->connect_to (*track_node);
                        }
                    }

                  auto node2 = graph.get_nodes ().find_node_for_processable (
                    track_processor->get_pitch_bend_port (channel_index));
                  if (node2 || !drop_unnecessary_ports)
                    {
                      node2->connect_to (*track_node);
                    }

                  node2 = graph.get_nodes ().find_node_for_processable (
                    track_processor->get_poly_key_pressure_port (channel_index));
                  if (node2 || !drop_unnecessary_ports)
                    {
                      node2->connect_to (*track_node);
                    }

                  node2 = graph.get_nodes ().find_node_for_processable (
                    track_processor->get_channel_pressure_port (channel_index));
                  if (node2 || !drop_unnecessary_ports)
                    {
                      node2->connect_to (*track_node);
                    }
                }
            }

          if constexpr (std::is_same_v<TrackT, TempoTrack>)
            {
              auto node2 = graph.get_nodes ().find_node_for_processable (
                tr->get_bpm_port ());
              if (node2 || !drop_unnecessary_ports)
                {
                  graph.get_nodes ().add_special_node (*node2);
                  node2->connect_to (*track_node);
                }
              node2 = graph.get_nodes ().find_node_for_processable (
                tr->get_beats_per_bar_port ());
              if (node2 || !drop_unnecessary_ports)
                {
                  graph.get_nodes ().add_special_node (*node2);
                  node2->connect_to (*track_node);
                }
              node2 = graph.get_nodes ().find_node_for_processable (
                tr->get_beat_unit_port ());
              if (node2 || !drop_unnecessary_ports)
                {
                  graph.get_nodes ().add_special_node (*node2);
                  node2->connect_to (*track_node);
                }
              track_node->connect_to (*initial_processor_node);
            }

          if constexpr (std::is_same_v<TrackT, ModulatorTrack>)
            {
              initial_processor_node->connect_to (*track_node);

              for (auto &pl_var : tr->get_modulator_span ())
                {
                  std::visit (
                    [&] (auto &&pl) {
                      if (pl && !pl->deleting_)
                        {
                          connect_plugin (graph, *pl, drop_unnecessary_ports);
                          for (auto &pl_port_var : pl->get_input_port_span ())
                            {
                              std::visit (
                                [&] (auto &&pl_port) {
                                  // z_return_if_fail (pl_port->get_plugin (true)
                                  // != nullptr);
                                  auto port_node =
                                    graph.get_nodes ()
                                      .find_node_for_processable (*pl_port);
                                  if (
                                    drop_unnecessary_ports && !port_node
                                    && pl_port->is_control ())
                                    {
                                      return;
                                    }
                                  if (!port_node)
                                    {
                                      z_error (
                                        "failed to find node for port {}",
                                        pl_port->get_label ());
                                      return;
                                    }
                                  track_node->connect_to (*port_node);
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
                  {
                    auto * const node2 =
                      graph.get_nodes ().find_node_for_processable (
                        mmp->get_macro_port ());
                    if (node2 || !drop_unnecessary_ports)
                      {
                        node2->connect_to (*mmp_node);
                      }
                  }

                  auto * const node2 =
                    graph.get_nodes ().find_node_for_processable (
                      mmp->get_cv_out_port ());
                  mmp_node->connect_to (*node2);
                }
            }

          if constexpr (std::derived_from<TrackT, ChannelTrack>)
            {

              auto &ch = tr->channel_;
              auto &prefader = ch->prefader_;
              auto &fader = ch->fader_;

              /* connect the fader */
              auto * const fader_node =
                graph.get_nodes ().find_node_for_processable (*fader);
              z_warn_if_fail (fader_node);
              if (fader->type_ == Fader::Type::AudioChannel)
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
              else if (fader->type_ == Fader::Type::MidiChannel)
                {
                  auto node2 = graph.get_nodes ().find_node_for_processable (
                    fader->get_midi_in_port ());
                  node2->connect_to (*fader_node);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    fader->get_midi_out_port ());
                  fader_node->connect_to (*node2);
                }
              {
                auto * const node2 =
                  graph.get_nodes ().find_node_for_processable (
                    fader->get_amp_port ());
                if (node2 || !drop_unnecessary_ports)
                  {
                    node2->connect_to (*fader_node);
                  }
              }
              {
                auto * const node2 =
                  graph.get_nodes ().find_node_for_processable (
                    fader->get_balance_port ());
                if (node2 || !drop_unnecessary_ports)
                  {
                    node2->connect_to (*fader_node);
                  }
              }
              {
                auto * const node2 =
                  graph.get_nodes ().find_node_for_processable (
                    fader->get_mute_port ());
                if (node2 || !drop_unnecessary_ports)
                  {
                    node2->connect_to (*fader_node);
                  }
              }

              /* connect the prefader */
              auto * const prefader_node =
                graph.get_nodes ().find_node_for_processable (*prefader);
              z_warn_if_fail (prefader_node);
              if (prefader->type_ == Fader::Type::AudioChannel)
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
              else if (prefader->type_ == Fader::Type::MidiChannel)
                {
                  auto * node2 = graph.get_nodes ().find_node_for_processable (
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
                      connect_plugin (graph, *pl, drop_unnecessary_ports);
                    }
                }

              for (auto &send : ch->sends_)
                {
                  /* note that we do not skip empty sends because then port
                   * connection validation will not detect invalid connections
                   * properly */
                  auto * const send_node =
                    graph.get_nodes ().find_node_for_processable (*send);

                  auto * node2 = graph.get_nodes ().find_node_for_processable (
                    send->get_amount_port ());
                  if (node2)
                    node2->connect_to (*send_node);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    send->get_enabled_port ());
                  if (node2)
                    node2->connect_to (*send_node);

                  if (tr->out_signal_type_ == dsp::PortType::Event)
                    {
                      node2 = graph.get_nodes ().find_node_for_processable (
                        send->get_midi_in_port ());
                      node2->connect_to (*send_node);
                      node2 = graph.get_nodes ().find_node_for_processable (
                        send->get_midi_out_port ());
                      send_node->connect_to (*node2);
                    }
                  else if (tr->out_signal_type_ == dsp::PortType::Audio)
                    {
                      iterate_tuple (
                        [&] (const auto &port) {
                          node2 =
                            graph.get_nodes ().find_node_for_processable (port);
                          node2->connect_to (*send_node);
                        },
                        send->get_stereo_in_ports ());
                      iterate_tuple (
                        [&] (const auto &port) {
                          node2 =
                            graph.get_nodes ().find_node_for_processable (port);
                          send_node->connect_to (*node2);
                        },
                        send->get_stereo_out_ports ());
                    }
                }
            }
        },
        cur_tr);
    }

  auto connect_port = [&]<FinalPortSubclass PortT> (PortT &p) {
    auto * node = graph.get_nodes ().find_node_for_processable (p);
    for (auto &src : p.srcs_)
      {
        auto node2 = graph.get_nodes ().find_node_for_processable (*src);
        z_warn_if_fail (node);
        z_warn_if_fail (node2);
        node2->connect_to (*node);
      }
    for (auto &dest : p.dests_)
      {
        auto node2 = graph.get_nodes ().find_node_for_processable (*dest);
        z_warn_if_fail (node);
        z_warn_if_fail (node2);
        node->connect_to (*node2);
      }
  };

  for (auto &port : ports)
    {
      if (port->deleting_) [[unlikely]]
        continue;
      if (port->id_->owner_type_ == dsp::PortIdentifier::OwnerType::Plugin)
        {
          auto pl_var =
            project.find_plugin_by_id (port->id_->plugin_id_.value ());
          z_return_if_fail (pl_var.has_value ());
          if (std::visit (
                [&] (auto &&pl) { return pl->deleting_; }, pl_var.value ()))
            continue;
        }

      std::visit (
        [&] (auto &&p) { connect_port (*p); },
        convert_to_variant<PortPtrVariant> (port));
    }

  z_debug ("done building graph");
}

bool
ProjectGraphBuilder::
  can_ports_be_connected (Project &project, const Port &src, const Port &dest)
{
  AudioEngine::State state{};
  project.audio_engine_->wait_for_pause (state, false, true);

  z_debug ("validating for {} to {}", src.get_label (), dest.get_label ());

  ProjectGraphBuilder builder (project, false);
  dsp::Graph          graph;
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
    convert_to_variant<PortPtrVariant> (&src),
    convert_to_variant<PortPtrVariant> (&dest));
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
