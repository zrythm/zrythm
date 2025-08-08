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
  auto  &metronome = *transport->metronome_;

  const auto add_node_for_processable = [&] (auto &processable) {
    return graph.add_node_for_processable (processable, *transport);
  };

  const auto connect_ports =
    [&] (const dsp::Port::Uuid src_id, const dsp::Port::Uuid &dest_id) {
      dsp::add_connection_for_ports (
        graph, { src_id, project.get_port_registry () },
        { dest_id, project.get_port_registry () });
    };

  // add engine monitor output
  {
    const auto &monitor_outs = engine->get_monitor_out_ports ();
    add_node_for_processable (monitor_outs.first);
    add_node_for_processable (monitor_outs.second);
  }

  /* add the sample processor */
// TODO
#if 0
  add_node_for_processable (*sample_processor);
  for (const auto &fader_out : sample_processor->fader_->get_output_ports ())
    {
      add_node_for_processable (*fader_out.get_object_as<dsp::AudioPort> ());
    }
#endif

  // add metronome processor
  dsp::ProcessorGraphBuilder::add_nodes (graph, *transport, metronome);

  /* add the monitor fader */
  dsp::ProcessorGraphBuilder::add_nodes (graph, *transport, *monitor_fader);

  /* add the initial processor */
  auto * initial_processor_node = graph.add_initial_processor (*transport);

  // add midi panic processor
  dsp::ProcessorGraphBuilder::add_nodes (
    graph, *transport, *engine->midi_panic_processor_);

  /* add the hardware input processor */
  // add_node_for_processable (*hw_in_processor);

  /* add each track */
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
              dsp::ProcessorGraphBuilder::add_nodes (
                graph, *transport, *tr->processor_);
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
                      dsp::ProcessorGraphBuilder::add_nodes (
                        graph, *transport, *pl);
                    },
                    pl_var);
                }

              /* add macro processors */
              for (const auto &mp : tr->get_modulator_macro_processors ())
                {
                  dsp::ProcessorGraphBuilder::add_nodes (graph, *transport, *mp);
                }
            }

          if constexpr (
            std::derived_from<TrackT, structure::tracks::ChannelTrack>)
            {
              auto * channel = tr->channel ();
              structure::tracks::ChannelSubgraphBuilder::add_nodes (
                graph, *transport, *channel);
            }
        },
        cur_tr);
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

  // connect metronome processor
  {
    dsp::ProcessorGraphBuilder::add_connections (graph, metronome);
    const auto &[monitor_l_in, monitor_r_in] =
      monitor_fader->get_stereo_in_ports ();
    auto * monitor_l_in_node =
      graph.get_nodes ().find_node_for_processable (monitor_l_in);
    auto * monitor_r_in_node =
      graph.get_nodes ().find_node_for_processable (monitor_r_in);
    auto [metronome_l_out, metronome_r_out] =
      metronome.get_output_audio_ports ();
    auto * metronome_l_out_node =
      graph.get_nodes ().find_node_for_processable (*metronome_l_out);
    auto * metronome_r_out_node =
      graph.get_nodes ().find_node_for_processable (*metronome_r_out);
    metronome_l_out_node->connect_to (*monitor_l_in_node);
    metronome_r_out_node->connect_to (*monitor_r_in_node);
  }

  // connect midi panic processor
  {
    dsp::ProcessorGraphBuilder::add_connections (
      graph, *engine->midi_panic_processor_);
    auto * midi_panic_processor_node =
      graph.get_nodes ().find_node_for_processable (
        *engine->midi_panic_processor_);
    initial_processor_node->connect_to (*midi_panic_processor_node);
  }

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
                auto         track_processor = tr->processor_.get ();
                auto * const track_processor_node =
                  graph.get_nodes ().find_node_for_processable (*track_processor);
                assert (track_processor_node);
                dsp::ProcessorGraphBuilder::add_connections (
                  graph, *track_processor);

                // connect initial processor to track processor inputs (or track
                // processor itself if no inputs)
                if (track_processor->get_input_ports ().empty ())
                  {
                    initial_processor_node->connect_to (*track_processor_node);
                  }
                else
                  {
                    for (
                      const auto &tp_in_port_ref :
                      track_processor->get_input_ports ())
                      {
                        std::visit (
                          [&] (auto &&tp_in_port) {
                            using PortT = base_type<decltype (tp_in_port)>;
                            // MIDI ports go via MIDI panic
                            if constexpr (std::is_same_v<PortT, dsp::MidiPort>)
                              {
                                graph.get_nodes ()
                                  .find_node_for_processable (
                                    *engine->midi_panic_processor_
                                       ->get_output_ports ()
                                       .front ()
                                       .get_object_as<dsp::MidiPort> ())
                                  ->connect_to (
                                    *graph.get_nodes ()
                                       .find_node_for_processable (*tp_in_port));
                              }
                            // other ports go directly via initial processor
                            else
                              {
                                initial_processor_node->connect_to (
                                  *graph.get_nodes ().find_node_for_processable (
                                    *tp_in_port));
                              }
                          },
                          tp_in_port_ref.get_object ());
                      }
                  }

                if constexpr (
                  std::is_same_v<TrackT, structure::tracks::ModulatorTrack>)
                  {
                    for (const auto &pl_var : tr->get_modulator_span ())
                      {
                        std::visit (
                          [&] (auto &&pl) {
                            if (pl)
                              {
                                dsp::ProcessorGraphBuilder::add_connections (
                                  graph, *pl);
                                for (
                                  const auto &pl_port_ref :
                                  pl->get_input_ports ())
                                  {
                                    std::visit (
                                      [&] (auto &&pl_port) {
                                        // z_return_if_fail (pl_port->get_plugin
                                        // (true)
                                        // != nullptr);
                                        auto port_node =
                                          graph.get_nodes ()
                                            .find_node_for_processable (*pl_port);
                                        assert (port_node != nullptr);
                                        track_processor_node->connect_to (
                                          *port_node);
                                      },
                                      pl_port_ref.get_object ());
                                  }
                              }
                          },
                          pl_var);
                      }

                    /* connect the modulator macro processors */
                    for (const auto &mmp : tr->get_modulator_macro_processors ())
                      {
                        dsp::ProcessorGraphBuilder::add_connections (
                          graph, *mmp);

                        // special case - track processor connected directly to
                        // the mmp input(s)
                        auto * const cv_in_node =
                          graph.get_nodes ().find_node_for_processable (
                            mmp->get_cv_in_port ());
                        track_processor_node->connect_to (*cv_in_node);
                      }
                  } // if modulator track
              }

              // connect the channel
              if constexpr (
                std::derived_from<TrackT, structure::tracks::ChannelTrack>)
                {
                  auto * ch = tr->channel ();
                  structure::tracks::ChannelSubgraphBuilder::add_connections (
                    graph, tr->get_port_registry (), *ch,
                    std::span (tr->processor_->get_output_ports ()));

                  // connect to target track
                  if (tr->has_output ())
                    {
                      auto &output_track = tr->output_track_as_group_target ();
                      if (
                        output_track.get_input_signal_type ()
                        == dsp::PortType::Audio)
                        {
                          connect_ports (
                            ch->get_audio_post_fader ()
                              .get_audio_out_port (0)
                              .get_uuid (),
                            output_track.processor_->get_stereo_in_ports ()
                              .first.get_uuid ());
                          connect_ports (
                            ch->get_audio_post_fader ()
                              .get_audio_out_port (1)
                              .get_uuid (),
                            output_track.processor_->get_stereo_in_ports ()
                              .second.get_uuid ());
                        }
                      else if (
                        output_track.get_input_signal_type ()
                        == dsp::PortType::Midi)
                        {
                          connect_ports (
                            ch->get_midi_post_fader ()
                              .get_midi_out_port (0)
                              .get_uuid (),
                            output_track.processor_->get_midi_in_port ()
                              .get_uuid ());
                        }
                    }

                  // Connect master track output to monitor fader input
                  if constexpr (
                    std::is_same_v<TrackT, structure::tracks::MasterTrack>)
                    {
                      const auto &monitor_fader_ins =
                        monitor_fader->get_stereo_in_ports ();
                      connect_ports (
                        ch->get_audio_post_fader ()
                          .get_audio_out_port (0)
                          .get_uuid (),
                        monitor_fader_ins.first.get_uuid ());
                      connect_ports (
                        ch->get_audio_post_fader ()
                          .get_audio_out_port (1)
                          .get_uuid (),
                        monitor_fader_ins.second.get_uuid ());
                    }
                }
            }
        },
        cur_tr);
    }

  // add additional custom connections from the PortConnectionsManager
  {
    const auto &mgr = *project.port_connections_manager_;
    for (const auto &conn : mgr.get_connections ())
      {
        const auto src_port_var = *project.find_port_by_id (conn->src_id_);
        const auto dest_port_var = *project.find_port_by_id (conn->dest_id_);
        std::visit (
          [&] (auto &&src_port, auto &&dest_port) {
            using SourcePortT = base_type<decltype (src_port)>;
            using DestPortT = base_type<decltype (dest_port)>;
            if constexpr (std::is_same_v<SourcePortT, DestPortT>)
              {
                auto * src_node =
                  graph.get_nodes ().find_node_for_processable (*src_port);
                auto * dest_node =
                  graph.get_nodes ().find_node_for_processable (*dest_port);
                src_node->connect_to (*dest_node);
              }
          },
          src_port_var, dest_port_var);
      }
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
  z_debug ("validating for {} to {}", src.get_label (), dest.get_label ());

  ProjectGraphBuilder builder (project);
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

  return valid;
}
