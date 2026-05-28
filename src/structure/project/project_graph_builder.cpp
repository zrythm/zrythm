// SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/fader.h"
#include "dsp/graph.h"
#include "dsp/metronome.h"
#include "structure/project/project.h"
#include "structure/project/project_graph_builder.h"
#include "structure/tracks/channel_subgraph_builder.h"
#include "utils/registry_utils.h"
#include "utils/variant_helpers.h"

namespace zrythm::structure::project
{

// Extracted to a separate function because MSVC ICEs if used in lambda
static void
process_track_connections (
  structure::tracks::Track * tr,
  dsp::graph::Graph         &graph,
  ProjectGraphBuilder       &self,
  const dsp::ITransport     &transport,
  const Project             &project,
  dsp::graph::GraphNode *    initial_processor_node,
  dsp::MidiPanicProcessor   &midi_panic_processor,
  dsp::Fader *               monitor_fader)
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
      for (
        const auto &tp_in_port :
        track_processor->get_input_ports ()
          | std::views::transform (&utils::TypedUuidReference<dsp::Port>::get))
        {
          // MIDI input port goes via MIDI panic
          if (
            tp_in_port->get_symbol ().view ()
            == tracks::TrackProcessor::midi_in_symbol)
            {
              graph.get_nodes ()
                .find_node_for_processable (
                  *midi_panic_processor.get_output_ports ()
                     .front ()
                     .get_object_as<dsp::MidiPort> ())
                ->connect_to (
                  *graph.get_nodes ().find_node_for_processable (*tp_in_port));
            }
          // other ports go directly via initial processor
          else
            {
              initial_processor_node->connect_to (
                *graph.get_nodes ().find_node_for_processable (*tp_in_port));
            }
        }
    }

  if (auto * mod_track = qobject_cast<structure::tracks::ModulatorTrack *> (tr))
    {
      std::vector<plugins::PluginUuidReference> plugins;
      mod_track->modulators ()->get_plugins (plugins);
      for (const auto &pl_ref : plugins)
        {
          auto * pl = pl_ref.get ();
          dsp::ProcessorGraphBuilder::add_connections (graph, *pl);
          for (const auto &pl_port_ref : pl->get_input_ports ())
            {
              auto port_node = graph.get_nodes ().find_node_for_processable (
                *pl_port_ref.get ());
              assert (port_node != nullptr);
              track_processor_node->connect_to (*port_node);
            }
        }

      /* connect the modulator macro processors */
      for (const auto &mmp : mod_track->get_modulator_macro_processors ())
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
  const auto &engine = project_->audio_engine_;
  // auto *      sample_processor = engine->sample_processor_.get ();
  auto * tracklist = project_->tracklist ();
  auto * transport = project->getTransport ();
  auto * midi_panic_processor = engine->midi_panic_processor ();

  const auto add_node_for_processable = [&] (auto &processable) {
    return graph.add_node_for_processable (processable);
  };

  const auto connect_ports =
    [&] (
      const dsp::FinalPortSubclass auto &src,
      const dsp::FinalPortSubclass auto &dest) {
      tracks::ChannelSubgraphBuilder::add_connection_for_ports (
        graph, src, dest);
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
  dsp::ProcessorGraphBuilder::add_nodes (graph, *metronome_);

  /* add the monitor fader */
  dsp::ProcessorGraphBuilder::add_nodes (graph, *monitor_fader_);

  /* add the initial processor */
  auto * initial_processor_node = graph.add_initial_processor ();

  // add midi panic processor
  dsp::ProcessorGraphBuilder::add_nodes (graph, *midi_panic_processor);

  /* add the audio input processor */
  if (auto * aip = engine->audio_input_processor ())
    {
      dsp::ProcessorGraphBuilder::add_nodes (graph, *aip);
    }

  /* add the MIDI input processors */
  for (const auto &mip : engine->midi_input_processors () | std::views::values)
    {
      dsp::ProcessorGraphBuilder::add_nodes (graph, *mip);
    }

  /* add each track */
  for (const auto &cur_tr : tracklist->collection ()->tracks ())
    {
      auto * tr = cur_tr.get ();

      if (auto * tp = tr->get_track_processor ())
        {
          /* add the track processor */
          dsp::ProcessorGraphBuilder::add_nodes (graph, *tp);
        }

      /* handle modulator track */
      if (
        auto * mod_track = qobject_cast<structure::tracks::ModulatorTrack *> (tr))
        {
          /* add plugins */
          std::vector<plugins::PluginUuidReference> plugins;
          mod_track->modulators ()->get_plugins (plugins);
          for (const auto &pl_ref : plugins)
            {
              dsp::ProcessorGraphBuilder::add_nodes (graph, *pl_ref.get ());
            }

          /* add macro processors */
          for (const auto &mp : mod_track->get_modulator_macro_processors ())
            {
              dsp::ProcessorGraphBuilder::add_nodes (graph, *mp);
            }
        }

      if (auto * channel = tr->channel ())
        {
          structure::tracks::ChannelSubgraphBuilder::add_nodes (graph, *channel);
        }
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
    dsp::ProcessorGraphBuilder::add_connections (graph, *metronome_);
    auto  &monitor_in = monitor_fader_->get_stereo_in_port ();
    auto * monitor_in_node =
      graph.get_nodes ().find_node_for_processable (monitor_in);
    auto * metronome_out = metronome_->get_output_audio_port_non_rt ();
    auto * metronome_out_node =
      graph.get_nodes ().find_node_for_processable (*metronome_out);
    metronome_out_node->connect_to (*monitor_in_node);
  }

  // connect midi panic processor
  {
    dsp::ProcessorGraphBuilder::add_connections (graph, *midi_panic_processor);
    auto * midi_panic_processor_node =
      graph.get_nodes ().find_node_for_processable (*midi_panic_processor);
    initial_processor_node->connect_to (*midi_panic_processor_node);
  }

  /* connect the monitor fader */
  dsp::ProcessorGraphBuilder::add_connections (graph, *monitor_fader_);

// TODO: connect the sample processor output to the monitor fader output so we
// hear the samples
#if 0
  {
    const auto &sp_fader_outs = sample_processor->fader_->get_output_ports ();
    const auto &monitor_fader_outs = monitor_fader_->get_output_ports ();
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
    const auto &mf_out = monitor_fader_->get_output_ports ().front ();
    const auto &monitor_out = engine->get_monitor_out_port ();
    auto *      mf_out_node = graph.get_nodes ().find_node_for_processable (
      *mf_out.get_object_as<dsp::AudioPort> ());
    auto * monitor_out_node =
      graph.get_nodes ().find_node_for_processable (monitor_out);
    mf_out_node->connect_to (*monitor_out_node);
  }

  /* connect the audio input processor */
  if (auto * aip = engine->audio_input_processor ())
    {
      dsp::ProcessorGraphBuilder::add_connections (graph, *aip);
      auto * aip_node = graph.get_nodes ().find_node_for_processable (*aip);
      z_return_if_fail (aip_node);
      initial_processor_node->connect_to (*aip_node);
    }

  /* connect the MIDI input processors */
  for (const auto &mip : engine->midi_input_processors () | std::views::values)
    {
      dsp::ProcessorGraphBuilder::add_connections (graph, *mip);
      auto  &mip_out_port = mip->get_output_port ();
      auto * mip_out_node =
        graph.get_nodes ().find_node_for_processable (mip_out_port);
      z_return_if_fail (mip_out_node);
      initial_processor_node->connect_to (*mip_out_node);
    }

  /* connect tracks */
  const auto * master_track = tracklist->singletonTracks ()->masterTrack ();
  for (const auto &cur_tr : tracklist->collection ()->tracks ())
    {
      auto * tr = cur_tr.get ();

      // only processable tracks are part of the graph (essentially every
      // track besides MarkerTrack)
      if (tr->get_track_processor () != nullptr)
        {
          process_track_connections (
            tr, graph, *this, *transport, *project, initial_processor_node,
            *midi_panic_processor, monitor_fader_);

          // connect audio input processor to this track, if configured
          const auto &sel_provider = project->audio_input_selection_provider ();
          if (
            tr->input_signal_type () == dsp::PortType::Audio && sel_provider
            && (engine->audio_input_processor () != nullptr))
            {
              if (auto * sel = sel_provider (tr->get_uuid ()))
                {
                  if (!sel->deviceName ().isEmpty ())
                    {
                      if (sel->deviceName () == engine->device_name ())
                        {
                          auto * src_port =
                            engine->audio_input_processor ()->find_output_port (
                              sel->firstChannel (), sel->stereo ());
                          if (src_port != nullptr)
                            {
                              auto * src_node =
                                graph.get_nodes ().find_node_for_processable (
                                  *src_port);
                              auto * dst_node =
                                graph.get_nodes ().find_node_for_processable (
                                  tr->get_track_processor ()
                                    ->get_stereo_in_port ());
                              if ((src_node != nullptr) && (dst_node != nullptr))
                                {
                                  src_node->connect_to (*dst_node);
                                }
                              else
                                {
                                  z_warning (
                                    "Failed to find graph node for audio input connection (src={}, dst={})",
                                    src_node != nullptr, dst_node != nullptr);
                                }
                            }
                          else
                            {
                              z_warning (
                                "AudioInputProcessor has no output port for channel {} (stereo: {})",
                                sel->firstChannel (), sel->stereo ());
                            }
                        }
                      else
                        {
                          z_debug (
                            "Audio input selection device '{}' does not match current device '{}' for track {}",
                            sel->deviceName (), engine->device_name (),
                            tr->get_uuid ());
                        }
                    }
                }
            }

          // connect MIDI input processor to this track, if configured
          const auto &midi_sel_provider =
            project->midi_input_selection_provider ();
          if (
            tr->input_signal_type () == dsp::PortType::Midi && midi_sel_provider)
            {
              if (auto * sel = midi_sel_provider (tr->get_uuid ()))
                {
                  if (!sel->deviceIdentifier ().isEmpty ())
                    {
                      const auto sel_id = utils::Utf8String::from_qstring (
                        sel->deviceIdentifier ());
                      auto it = engine->midi_input_processors ().find (sel_id);
                      if (it != engine->midi_input_processors ().end ())
                        {
                          auto &src_port = it->second->get_output_port ();
                          auto &dst_port =
                            tr->get_track_processor ()->get_hw_midi_in_port ();
                          auto * src_node =
                            graph.get_nodes ().find_node_for_processable (
                              src_port);
                          auto * dst_node =
                            graph.get_nodes ().find_node_for_processable (
                              dst_port);
                          if ((src_node != nullptr) && (dst_node != nullptr))
                            {
                              src_node->connect_to (*dst_node);
                            }
                        }
                      else
                        {
                          z_debug (
                            "MIDI input processor not found for device '{}' (track {})",
                            sel_id, tr->get_uuid ());
                        }
                    }
                }
            }

          // connect the channel
          if (auto * ch = tr->channel ())
            {
              structure::tracks::ChannelSubgraphBuilder::add_connections (
                graph, *ch,
                tr->get_track_processor ()->get_output_ports ().front ());

              // connect to target track
              auto route_target =
                tracklist->get_track_route_target (tr->get_uuid ());
              if (route_target.has_value ())
                {
                  auto * output_track = route_target.value ().get ();
                  if (output_track->input_signal_type () == dsp::PortType::Audio)
                    {
                      connect_ports (
                        ch->get_audio_post_fader ().get_audio_out_port (),
                        output_track->get_track_processor ()
                          ->get_stereo_in_port ());
                    }
                  else if (
                    output_track->input_signal_type () == dsp::PortType::Midi)
                    {
                      connect_ports (
                        ch->get_midi_post_fader ().get_midi_out_port (0),
                        output_track->get_track_processor ()
                          ->get_midi_in_port ());
                    }
                }

              // Connect master track output to monitor fader input
              if (qobject_cast<structure::tracks::MasterTrack *> (tr) != nullptr)
                {
                  auto &monitor_fader_in = monitor_fader_->get_stereo_in_port ();
                  connect_ports (
                    ch->get_audio_post_fader ().get_audio_out_port (),
                    monitor_fader_in);
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
    }

  // add additional custom connections from the PortConnectionsManager
  {
    const auto &mgr = *project->port_connections_manager_;
    for (const auto &conn : mgr.get_connections ())
      {
        auto &src_port_base =
          utils::get_typed<dsp::Port> (project->get_registry (), conn->src_id_);
        auto &dest_port_base = utils::get_typed<dsp::Port> (
          project->get_registry (), conn->dest_id_);
        auto src_port_var =
          convert_to_variant_qobj<dsp::PortPtrVariant> (&src_port_base);
        auto dest_port_var =
          convert_to_variant_qobj<dsp::PortPtrVariant> (&dest_port_base);
        std::visit (
          [&] (auto &&src_port, auto &&dest_port) {
            using SourcePortT = utils::base_type<decltype (src_port)>;
            using DestPortT = utils::base_type<decltype (dest_port)>;
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

  ProjectGraphBuilder builder (
    project, project.metronome (), project.monitor_fader ());
  dsp::graph::Graph graph;
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
    convert_to_variant_qobj<dsp::PortPtrVariant> (&src),
    convert_to_variant_qobj<dsp::PortPtrVariant> (&dest));
  if (!success)
    {
      z_info ("failed to connect {} to {}", src.get_label (), dest.get_label ());
      return false;
    }

  bool valid = graph.is_valid ();

  z_debug ("valid {}", valid);

  return valid;
}
}
