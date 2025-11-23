// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph.h"
#include "dsp/graph_export.h"
#include "engine/session/project_graph_builder.h"
#include "gui/backend/backend/project.h"
#include "structure/tracks/channel_subgraph_builder.h"

using namespace zrythm;

// Extracted to a separate function because MSVC ICEs if used in lambda
template <typename TrackT>
static void
process_track_connections (
  TrackT *                         tr,
  dsp::graph::Graph               &graph,
  ProjectGraphBuilder             &self,
  const dsp::ITransport           &transport,
  const Project                   &project,
  dsp::graph::GraphNode *          initial_processor_node,
  engine::device_io::AudioEngine * engine,
  dsp::Fader *                     monitor_fader)
{
  /* connect the track processor */
  auto *       track_processor = tr->get_track_processor ();
  auto * const track_processor_node =
    graph.get_nodes ().find_node_for_processable (*track_processor);
  assert (track_processor_node);
  dsp::ProcessorGraphBuilder::add_connections (graph, *track_processor);

  // connect initial processor to track processor inputs (or track
  // processor itself if no inputs)
  if (track_processor->get_input_ports ().empty ())
    {
      initial_processor_node->connect_to (*track_processor_node);
    }
  else
    {
      for (const auto &tp_in_port_ref : track_processor->get_input_ports ())
        {
          std::visit (
            [&] (auto &&tp_in_port) {
              using PortT = base_type<decltype (tp_in_port)>;
              // MIDI ports go via MIDI panic
              if constexpr (std::is_same_v<PortT, dsp::MidiPort>)
                {
                  graph.get_nodes ()
                    .find_node_for_processable (
                      *engine->midi_panic_processor_->get_output_ports ()
                         .front ()
                         .get_object_as<dsp::MidiPort> ())
                    ->connect_to (*graph.get_nodes ().find_node_for_processable (
                      *tp_in_port));
                }
              // other ports go directly via initial processor
              else
                {
                  initial_processor_node->connect_to (
                    *graph.get_nodes ().find_node_for_processable (*tp_in_port));
                }
            },
            tp_in_port_ref.get_object ());
        }
    }

  if constexpr (std::is_same_v<TrackT, structure::tracks::ModulatorTrack>)
    {
      for (
        const auto &pl_obj :
        tr->modulators ()->plugins ()
          | std::views::transform (&plugins::PluginUuidReference::get_object))
        {
          std::visit (
            [&] (auto &&pl) {
              dsp::ProcessorGraphBuilder::add_connections (graph, *pl);
              for (
                const auto &pl_port_obj :
                pl->get_input_ports ()
                  | std::views::transform (&dsp::PortUuidReference::get_object))
                {
                  std::visit (
                    [&] (auto &&pl_port) {
                      auto port_node =
                        graph.get_nodes ().find_node_for_processable (*pl_port);
                      assert (port_node != nullptr);
                      track_processor_node->connect_to (*port_node);
                    },
                    pl_port_obj);
                }
            },
            pl_obj);
        }

      /* connect the modulator macro processors */
      for (const auto &mmp : tr->get_modulator_macro_processors ())
        {
          dsp::ProcessorGraphBuilder::add_connections (graph, *mmp);

          // special case - track processor connected directly to
          // the mmp input(s)
          auto * const cv_in_node = graph.get_nodes ().find_node_for_processable (
            mmp->get_cv_in_port ());
          track_processor_node->connect_to (*cv_in_node);
        }
    } // if modulator track
}

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
  auto * tracklist = project_.tracklist ();
  auto * monitor_fader = project.controlRoom ()->monitorFader ();
  // auto *      hw_in_processor = engine->hw_in_processor_.get ();
  auto * transport = project.getTransport ();
  auto  &metronome = *project.metronome ();

  const auto add_node_for_processable = [&] (auto &processable) {
    return graph.add_node_for_processable (processable);
  };

  const auto connect_ports = [&] (auto * src, auto * dest) {
    dsp::add_connection_for_ports (graph, src, dest);
  };

  // add engine monitor output
  {
    auto &monitor_out = engine->get_monitor_out_port ();
    add_node_for_processable (monitor_out);
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
  dsp::ProcessorGraphBuilder::add_nodes (graph, metronome);

  /* add the monitor fader */
  dsp::ProcessorGraphBuilder::add_nodes (graph, *monitor_fader);

  /* add the initial processor */
  auto * initial_processor_node = graph.add_initial_processor ();

  // add midi panic processor
  dsp::ProcessorGraphBuilder::add_nodes (graph, *engine->midi_panic_processor_);

  /* add the hardware input processor */
  // add_node_for_processable (*hw_in_processor);

  /* add each track */
  for (const auto &cur_tr : tracklist->collection ()->get_track_span ())
    {
      std::visit (
        [&] (auto &&tr) {
          z_return_if_fail (tr);
          using TrackT = base_type<decltype (tr)>;

          if constexpr (structure::tracks::ProcessableTrack<TrackT>)
            {
              /* add the track processor */
              dsp::ProcessorGraphBuilder::add_nodes (
                graph, *tr->get_track_processor ());
            }

          /* handle modulator track */
          if constexpr (
            std::is_same_v<structure::tracks::ModulatorTrack, TrackT>)
            {
              /* add plugins */
              std::vector<plugins::PluginPtrVariant> plugins;
              tr->modulators ()->get_plugins (plugins);
              for (const auto &pl_var : plugins)
                {
                  std::visit (
                    [&] (auto &&pl) {
                      dsp::ProcessorGraphBuilder::add_nodes (graph, *pl);
                    },
                    pl_var);
                }

              /* add macro processors */
              for (const auto &mp : tr->get_modulator_macro_processors ())
                {
                  dsp::ProcessorGraphBuilder::add_nodes (graph, *mp);
                }
            }

          if (auto * channel = tr->channel ())
            {
              structure::tracks::ChannelSubgraphBuilder::add_nodes (
                graph, *channel);
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
    auto  &monitor_in = monitor_fader->get_stereo_in_port ();
    auto * monitor_in_node =
      graph.get_nodes ().find_node_for_processable (monitor_in);
    auto * metronome_out = metronome.get_output_audio_port_non_rt ();
    auto * metronome_out_node =
      graph.get_nodes ().find_node_for_processable (*metronome_out);
    metronome_out_node->connect_to (*monitor_in_node);
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
    const auto &mf_out = monitor_fader->get_output_ports ().front ();
    const auto &monitor_out = engine->get_monitor_out_port ();
    auto *      mf_out_node = graph.get_nodes ().find_node_for_processable (
      *mf_out.get_object_as<dsp::AudioPort> ());
    auto * monitor_out_node =
      graph.get_nodes ().find_node_for_processable (monitor_out);
    mf_out_node->connect_to (*monitor_out_node);
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

  /* connect tracks */
  const auto * master_track = tracklist->singletonTracks ()->masterTrack ();
  for (const auto &cur_tr : tracklist->collection ()->get_track_span ())
    {
      std::visit (
        [&] (auto &tr) {
          using TrackT = base_type<decltype (tr)>;

          // only processable tracks are part of the graph (essentially every
          // track besides MarkerTrack)
          if constexpr (structure::tracks::ProcessableTrack<TrackT>)
            {
              {
                process_track_connections (
                  tr, graph, *this, *transport, project, initial_processor_node,
                  engine.get (), monitor_fader);
              }

              // connect the channel
              if (auto * ch = tr->channel ())
                {
                  structure::tracks::ChannelSubgraphBuilder::add_connections (
                    graph, tr->get_port_registry (), *ch,
                    tr->get_track_processor ()->get_output_ports ().front ());

                  // connect to target track
                  auto route_target =
                    tracklist->get_track_route_target (tr->get_uuid ());
                  if (route_target.has_value ())
                    {
                      std::visit (
                        [&] (const auto &output_track) {
                          if (
                            output_track->get_input_signal_type ()
                            == dsp::PortType::Audio)
                            {
                              connect_ports (
                                &ch->get_audio_post_fader ()
                                   .get_audio_out_port (),
                                &output_track->get_track_processor ()
                                   ->get_stereo_in_port ());
                            }
                          else if (
                            output_track->get_input_signal_type ()
                            == dsp::PortType::Midi)
                            {
                              connect_ports (
                                &ch->get_midi_post_fader ().get_midi_out_port (0),
                                &output_track->get_track_processor ()
                                   ->get_midi_in_port ());
                            }
                        },
                        route_target.value ().get_object ());
                    }

                  // Connect master track output to monitor fader input
                  if constexpr (
                    std::is_same_v<TrackT, structure::tracks::MasterTrack>)
                    {
                      auto &monitor_fader_in =
                        monitor_fader->get_stereo_in_port ();
                      connect_ports (
                        &ch->get_audio_post_fader ().get_audio_out_port (),
                        &monitor_fader_in);
                    }
                  // Connect all track outputs to master processor:
                  // 1. track faders are checked for
                  // solo/mute/listen status regardless of whether they are
                  // actually part of the final pruned graph
                  // 2. outputs are used in meters
                  else
                    {
                      auto * ch_out_port =
                        graph.get_nodes ().find_node_for_processable (*(
                          ch->fader ()->is_audio ()
                            ? static_cast<dsp::graph::IProcessable *> (
                                ch->audioOutPort ())
                            : static_cast<dsp::graph::IProcessable *> (
                                ch->getMidiOut ())));
                      assert (ch_out_port != nullptr);
                      auto * master_track_processor =
                        graph.get_nodes ().find_node_for_processable (
                          *master_track->get_track_processor ());
                      assert (master_track_processor != nullptr);
                      ch_out_port->connect_to (*master_track_processor);
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
