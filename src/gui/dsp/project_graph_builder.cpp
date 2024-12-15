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
  for (const auto &cur_tr : tracklist->tracks_)
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
              for (auto &pl : tr->modulators_)
                {
                  if (!pl || pl->deleting_)
                    continue;

                  add_plugin (graph, *pl);
                }

              /* add macro processors */
              for (auto &mp : tr->modulator_macro_processors_)
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

          if (owner == dsp::PortIdentifier::OwnerType::Plugin)
            {
              port->plugin_ = port->get_plugin (true);
              z_return_val_if_fail (port->plugin_, nullptr);
            }

          if (port->id_->track_name_hash_ != 0)
            {
              port->track_ = port->get_track (true);
              z_return_val_if_fail (port->track_, nullptr);
            }

          /* reset port sources/dests */
          PortConnectionsManager::ConnectionsVector srcs;
          mgr.get_sources_or_dests (&srcs, *port->id_, true);
          port->srcs_.clear ();
          port->src_connections_.clear ();
          for (const auto &conn : srcs)
            {
              auto src_var = project.find_port_by_id (*conn->src_id_);
              z_return_val_if_fail (src_var.has_value (), nullptr);
              std::visit (
                [&] (auto &&src) { port->srcs_.push_back (src); },
                src_var.value ());
              z_return_val_if_fail (port->srcs_.back (), nullptr);
              port->src_connections_.emplace_back (conn->clone_unique ());
            }

          PortConnectionsManager::ConnectionsVector dests;
          mgr.get_sources_or_dests (&dests, *port->id_, false);
          port->dests_.clear ();
          port->dest_connections_.clear ();
          for (const auto &conn : dests)
            {
              auto dest_var = project.find_port_by_id (*conn->dest_id_);
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
                  dsp::PortIdentifier::Flags, port->id_->flags_,
                  dsp::PortIdentifier::Flags::Automatable))
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
              dsp::PortIdentifier::Flags, port->id_->flags_,
              dsp::PortIdentifier::Flags::ManualPress)))
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
      if (port->deleting_ || (port->id_->owner_type_ == dsp::PortIdentifier::OwnerType::Plugin && port->get_plugin(true)->deleting_))
        continue;

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
      for (auto &port : pl.in_ports_)
        {
          z_return_if_fail (port->get_plugin (true) != nullptr);
          auto * port_node =
            graph.get_nodes ().find_node_for_processable (*port);
          if (
            drop_unnecessary_ports && (port_node == nullptr)
            && port->is_control ())
            {
              continue;
            }
          z_return_if_fail (port_node);
          port_node->connect_to (*pl_node);
        }
      for (auto &port : pl.out_ports_)
        {
          z_return_if_fail (port->get_plugin (true) != nullptr);
          auto * port_node =
            graph.get_nodes ().find_node_for_processable (*port);
          z_return_if_fail (port_node);
          pl_node->connect_to (*port_node);
        }
    };

  /* connect the sample processor */
  {
    auto * node =
      graph.get_nodes ().find_node_for_processable (*sample_processor);
    z_return_if_fail (node);
    auto * port = &sample_processor->fader_->stereo_out_->get_l ();
    auto * node2 = graph.get_nodes ().find_node_for_processable (*port);
    node->connect_to (*node2);
    port = &sample_processor->fader_->stereo_out_->get_r ();
    node2 = graph.get_nodes ().find_node_for_processable (*port);
    node->connect_to (*node2);
  }

  /* connect the monitor fader */
  {
    auto * node = graph.get_nodes ().find_node_for_processable (*monitor_fader);
    auto * port = &monitor_fader->stereo_in_->get_l ();
    auto * node2 = graph.get_nodes ().find_node_for_processable (*port);
    node2->connect_to (*node);
    port = &monitor_fader->stereo_in_->get_r ();
    node2 = graph.get_nodes ().find_node_for_processable (*port);
    node2->connect_to (*node);
    port = &monitor_fader->stereo_out_->get_l ();
    node2 = graph.get_nodes ().find_node_for_processable (*port);
    node->connect_to (*node2);
    port = &monitor_fader->stereo_out_->get_r ();
    node2 = graph.get_nodes ().find_node_for_processable (*port);
    node->connect_to (*node2);
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
  for (auto &cur_tr : tracklist->tracks_)
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
                    *tr->processor_->mono_);
                  node2->connect_to (*track_node);
                  initial_processor_node->connect_to (*node2);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    *tr->processor_->input_gain_);
                  node2->connect_to (*track_node);
                  initial_processor_node->connect_to (*node2);
                }

              if constexpr (std::derived_from<TrackT, ProcessableTrack>)
                {
                  auto track_processor = tr->processor_.get ();
                  auto node2 = graph.get_nodes ().find_node_for_processable (
                    track_processor->stereo_in_->get_l ());
                  node2->connect_to (*track_node);
                  initial_processor_node->connect_to (*node2);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    track_processor->stereo_in_->get_r ());
                  node2->connect_to (*track_node);
                  initial_processor_node->connect_to (*node2);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    track_processor->stereo_out_->get_l ());
                  track_node->connect_to (*node2);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    track_processor->stereo_out_->get_r ());
                  track_node->connect_to (*node2);
                }
            }
          else if (tr->in_signal_type_ == dsp::PortType::Event)
            {
              if constexpr (std::derived_from<TrackT, ProcessableTrack>)
                {
                  auto track_processor = tr->processor_.get ();

                  if (tr->has_piano_roll () || tr->is_chord ())
                    {
                      /* connect piano roll */
                      auto node2 = graph.get_nodes ().find_node_for_processable (
                        *track_processor->piano_roll_);
                      z_return_if_fail (node2);
                      node2->connect_to (*track_node);
                    }

                  auto node2 = graph.get_nodes ().find_node_for_processable (
                    *track_processor->midi_in_);
                  node2->connect_to (*track_node);
                  initial_processor_node->connect_to (*node2);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    *track_processor->midi_out_);
                  track_node->connect_to (*node2);
                }
            }

          if constexpr (std::derived_from<TrackT, PianoRollTrack>)
            {
              auto track_processor = tr->processor_.get ();

              for (int j = 0; j < 16; j++)
                {
                  for (int k = 0; k < 128; k++)
                    {
                      auto node2 = graph.get_nodes ().find_node_for_processable (
                        *track_processor->midi_cc_[j * 128 + k]);
                      if (node2)
                        {
                          node2->connect_to (*track_node);
                        }
                    }

                  auto node2 = graph.get_nodes ().find_node_for_processable (
                    *track_processor->pitch_bend_[j]);
                  if (node2 || !drop_unnecessary_ports)
                    {
                      node2->connect_to (*track_node);
                    }

                  node2 = graph.get_nodes ().find_node_for_processable (
                    *track_processor->poly_key_pressure_[j]);
                  if (node2 || !drop_unnecessary_ports)
                    {
                      node2->connect_to (*track_node);
                    }

                  node2 = graph.get_nodes ().find_node_for_processable (
                    *track_processor->channel_pressure_[j]);
                  if (node2 || !drop_unnecessary_ports)
                    {
                      node2->connect_to (*track_node);
                    }
                }
            }

          if constexpr (std::is_same_v<TrackT, TempoTrack>)
            {
              auto node2 =
                graph.get_nodes ().find_node_for_processable (*tr->bpm_port_);
              if (node2 || !drop_unnecessary_ports)
                {
                  graph.get_nodes ().add_special_node (*node2);
                  node2->connect_to (*track_node);
                }
              node2 = graph.get_nodes ().find_node_for_processable (
                *tr->beats_per_bar_port_);
              if (node2 || !drop_unnecessary_ports)
                {
                  graph.get_nodes ().add_special_node (*node2);
                  node2->connect_to (*track_node);
                }
              node2 = graph.get_nodes ().find_node_for_processable (
                *tr->beat_unit_port_);
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

              for (auto &pl : tr->modulators_)
                {
                  if (pl && !pl->deleting_)
                    {
                      connect_plugin (graph, *pl, drop_unnecessary_ports);
                      for (auto &pl_port : pl->in_ports_)
                        {
                          z_return_if_fail (
                            pl_port->get_plugin (true) != nullptr);
                          auto port_node =
                            graph.get_nodes ().find_node_for_processable (
                              *pl_port);
                          if (
                            drop_unnecessary_ports && !port_node
                            && pl_port->is_control ())
                            {
                              continue;
                            }
                          if (!port_node)
                            {
                              z_error (
                                "failed to find node for port {}",
                                pl_port->get_label ());
                              return;
                            }
                          track_node->connect_to (*port_node);
                        }
                    }
                }

              /* connect the modulator macro processors */
              for (auto &mmp : tr->modulator_macro_processors_)
                {
                  auto mmp_node =
                    graph.get_nodes ().find_node_for_processable (*mmp);
                  z_return_if_fail (mmp_node);

                  {
                    auto * const node2 =
                      graph.get_nodes ().find_node_for_processable (*mmp->cv_in_);
                    node2->connect_to (*mmp_node);
                  }
                  {
                    auto * const node2 =
                      graph.get_nodes ().find_node_for_processable (*mmp->macro_);
                    if (node2 || !drop_unnecessary_ports)
                      {
                        node2->connect_to (*mmp_node);
                      }
                  }

                  auto * const node2 =
                    graph.get_nodes ().find_node_for_processable (*mmp->cv_out_);
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
                  auto node2 = graph.get_nodes ().find_node_for_processable (
                    fader->stereo_in_->get_l ());
                  node2->connect_to (*fader_node);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    fader->stereo_in_->get_r ());
                  node2->connect_to (*fader_node);

                  /* connect outs */
                  node2 = graph.get_nodes ().find_node_for_processable (
                    fader->stereo_out_->get_l ());
                  fader_node->connect_to (*node2);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    fader->stereo_out_->get_r ());
                  fader_node->connect_to (*node2);
                }
              else if (fader->type_ == Fader::Type::MidiChannel)
                {
                  auto node2 = graph.get_nodes ().find_node_for_processable (
                    *fader->midi_in_);
                  node2->connect_to (*fader_node);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    *fader->midi_out_);
                  fader_node->connect_to (*node2);
                }
              {
                auto * const node2 =
                  graph.get_nodes ().find_node_for_processable (*fader->amp_);
                if (node2 || !drop_unnecessary_ports)
                  {
                    node2->connect_to (*fader_node);
                  }
              }
              {
                auto * const node2 =
                  graph.get_nodes ().find_node_for_processable (*fader->balance_);
                if (node2 || !drop_unnecessary_ports)
                  {
                    node2->connect_to (*fader_node);
                  }
              }
              {
                auto * const node2 =
                  graph.get_nodes ().find_node_for_processable (*fader->mute_);
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
                  auto * node2 = graph.get_nodes ().find_node_for_processable (
                    prefader->stereo_in_->get_l ());
                  node2->connect_to (*prefader_node);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    prefader->stereo_in_->get_r ());
                  node2->connect_to (*prefader_node);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    prefader->stereo_out_->get_l ());
                  prefader_node->connect_to (*node2);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    prefader->stereo_out_->get_r ());
                  prefader_node->connect_to (*node2);
                }
              else if (prefader->type_ == Fader::Type::MidiChannel)
                {
                  auto * node2 = graph.get_nodes ().find_node_for_processable (
                    *prefader->midi_in_);
                  node2->connect_to (*prefader_node);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    *prefader->midi_out_);
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
                    *send->amount_);
                  if (node2)
                    node2->connect_to (*send_node);
                  node2 = graph.get_nodes ().find_node_for_processable (
                    *send->enabled_);
                  if (node2)
                    node2->connect_to (*send_node);

                  if (tr->out_signal_type_ == dsp::PortType::Event)
                    {
                      node2 = graph.get_nodes ().find_node_for_processable (
                        *send->midi_in_);
                      node2->connect_to (*send_node);
                      node2 = graph.get_nodes ().find_node_for_processable (
                        *send->midi_out_);
                      send_node->connect_to (*node2);
                    }
                  else if (tr->out_signal_type_ == dsp::PortType::Audio)
                    {
                      node2 = graph.get_nodes ().find_node_for_processable (
                        send->stereo_in_->get_l ());
                      node2->connect_to (*send_node);
                      node2 = graph.get_nodes ().find_node_for_processable (
                        send->stereo_in_->get_r ());
                      node2->connect_to (*send_node);
                      node2 = graph.get_nodes ().find_node_for_processable (
                        send->stereo_out_->get_l ());
                      send_node->connect_to (*node2);
                      node2 = graph.get_nodes ().find_node_for_processable (
                        send->stereo_out_->get_r ());
                      send_node->connect_to (*node2);
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
          auto port_pl = port->get_plugin (true);
          if (port_pl->deleting_)
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
