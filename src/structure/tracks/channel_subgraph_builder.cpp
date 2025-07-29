// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_all.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/channel_subgraph_builder.h"

namespace zrythm::structure::tracks
{
void
ChannelSubgraphBuilder::
  add_nodes (dsp::graph::Graph &graph, dsp::ITransport &transport, Channel &ch)
{
  // prefader
  if (ch.audio_prefader_)
    {
      dsp::ProcessorGraphBuilder::add_nodes (
        graph, transport, *ch.audio_prefader_);
    }
  else if (ch.midi_prefader_)
    {
      dsp::ProcessorGraphBuilder::add_nodes (
        graph, transport, *ch.midi_prefader_);
    }

  // fader
  {
    dsp::ProcessorGraphBuilder::add_nodes (graph, transport, *ch.fader ());
  }

  // plugins
  std::vector<zrythm::gui::old_dsp::plugins::Plugin *> plugins;
  ch.get_plugins (plugins);
  for (auto * pl : plugins)
    {
      if (pl == nullptr)
        continue;

      dsp::ProcessorGraphBuilder::add_nodes (graph, transport, *pl);
    }

  // sends
  for (auto &send : ch.sends_)
    {
      dsp::ProcessorGraphBuilder::add_nodes (graph, transport, *send);
    }

  // channel outputs
  if (ch.midi_out_id_.has_value ())
    {
      graph.add_node_for_processable (ch.get_midi_out_port (), transport);
    }
  else if (ch.stereo_out_left_id_.has_value ())
    {
      graph.add_node_for_processable (
        ch.get_stereo_out_ports ().first, transport);
      graph.add_node_for_processable (
        ch.get_stereo_out_ports ().second, transport);
    }
}

void
ChannelSubgraphBuilder::add_connections (
  dsp::graph::Graph                      &graph,
  Channel                                &ch,
  std::span<const dsp::PortUuidReference> track_processor_outputs)
{
  const auto connect_ports =
    [&] (const dsp::Port::Uuid src_id, const dsp::Port::Uuid &dest_id) {
      dsp::add_connection_for_ports (
        graph, { src_id, ch.port_registry_ }, { dest_id, ch.port_registry_ });
    };

  auto                                              &fader = ch.fader_;
  std::optional<dsp::PortUuidReference>              processor_midi_out_ref;
  std::optional<std::vector<dsp::PortUuidReference>> processor_audio_out_refs;
  if (track_processor_outputs.size () == 1)
    {
      processor_midi_out_ref = track_processor_outputs.front ();
    }
  else
    {
      processor_audio_out_refs =
        std::ranges::to<std::vector> (track_processor_outputs);
    }
  const auto input_type =
    processor_midi_out_ref.has_value ()
      ? dsp::PortType::Midi
      : dsp::PortType::Audio;
  const auto output_type =
    ch.fader ()->is_midi () ? dsp::PortType::Midi : dsp::PortType::Audio;

  std::vector<zrythm::gui::old_dsp::plugins::Plugin *> plugins;
  ch.get_plugins (plugins);
  Plugin * last_plugin = nullptr;
  for (auto * const pl : plugins)
    {
      if (pl != nullptr)
        {
          dsp::ProcessorGraphBuilder::add_connections (graph, *pl);
          if (last_plugin == nullptr)
            {
              // connect processor out to plugin input
              size_t last_index, num_ports_to_connect, i;
              auto   pl_in_ports = pl->get_input_port_span ();
              if (input_type == dsp::PortType::Midi)
                {
                  for (
                    const auto * in_port :
                    pl_in_ports.get_elements_by_type<dsp::MidiPort> ())
                    {
                      connect_ports (
                        processor_midi_out_ref->id (), in_port->get_uuid ());
                    }
                }
              else if (input_type == dsp::PortType::Audio)
                {
                  auto num_pl_audio_ins = std::ranges::distance (
                    pl_in_ports.get_elements_by_type<dsp::AudioPort> ());

                  num_ports_to_connect = 0;
                  if (num_pl_audio_ins == 1)
                    {
                      num_ports_to_connect = 1;
                    }
                  else if (num_pl_audio_ins > 1)
                    {
                      num_ports_to_connect = 2;
                    }
                  last_index = 0;
                  for (i = 0; i < num_ports_to_connect; i++)
                    {
                      for (; last_index < pl->in_ports_.size (); last_index++)
                        {
                          const auto in_port_var = pl_in_ports[last_index];
                          if (std::holds_alternative<dsp::AudioPort *> (
                                in_port_var))
                            {
                              auto * in_port =
                                std::get<dsp::AudioPort *> (in_port_var);
                              connect_ports (
                                processor_audio_out_refs->at (i).id (),
                                in_port->get_uuid ());
                              last_index++;
                              break;
                            }
                        }
                    }
                }
            }
          else
            {
              // connect last plugin to this plugin
              const auto connect_plugins = [&] (Plugin &src, Plugin &dest) {
                // Get audio ports
                auto src_audio_outs =
                  src.get_output_port_span ()
                    .get_elements_by_type<dsp::AudioPort> ();
                auto dest_audio_ins =
                  dest.get_input_port_span ()
                    .get_elements_by_type<dsp::AudioPort> ();

                const size_t num_src_outs =
                  std::ranges::distance (src_audio_outs);
                const size_t num_dest_ins =
                  std::ranges::distance (dest_audio_ins);

                // Handle audio connections
                if (num_src_outs > 0 && num_dest_ins > 0)
                  {
                    if (num_src_outs == 1 && num_dest_ins == 1)
                      {
                        // 1:1 connection
                        connect_ports (
                          src_audio_outs.front ()->get_uuid (),
                          dest_audio_ins.front ()->get_uuid ());
                      }
                    else if (num_src_outs == 1)
                      {
                        // 1:N connection - mono to stereo/multi
                        for (auto * in_port : dest_audio_ins)
                          {
                            connect_ports (
                              src_audio_outs.front ()->get_uuid (),
                              in_port->get_uuid ());
                          }
                      }
                    else if (num_dest_ins == 1)
                      {
                        // N:1 connection - stereo/multi to mono
                        connect_ports (
                          src_audio_outs.front ()->get_uuid (),
                          dest_audio_ins.front ()->get_uuid ());
                      }
                    else
                      {
                        // N:N connection - connect min(N,M) ports
                        for (
                          const auto &[src_audio_out, dest_audio_in] :
                          std::views::zip (src_audio_outs, dest_audio_ins))
                          {
                            connect_ports (
                              src_audio_out->get_uuid (),
                              dest_audio_in->get_uuid ());
                          }
                      }
                  }

                // Handle MIDI connections
                auto src_midi_outs =
                  src.get_output_port_span ()
                    .get_elements_by_type<dsp::MidiPort> ();
                auto dest_midi_ins =
                  dest.get_input_port_span ()
                    .get_elements_by_type<dsp::MidiPort> ();

                if (!src_midi_outs.empty () && !dest_midi_ins.empty ())
                  {
                    // Connect first MIDI out to all MIDI ins
                    auto * midi_out = src_midi_outs.front ();
                    for (auto * midi_in : dest_midi_ins)
                      {
                        connect_ports (
                          midi_out->get_uuid (), midi_in->get_uuid ());
                      }
                  }
              };
              connect_plugins (*last_plugin, *pl);
            }
        }
    }
  if (last_plugin == nullptr)
    {
      // connect processor outs to channel prefader
      if (output_type == dsp::PortType::Audio)
        {
          if (processor_audio_out_refs.has_value ())
            {
              auto &prefader = ch.get_audio_pre_fader ();
              for (
                const auto &[processor_audio_out_ref, prefader_audio_in_ref] :
                std::views::zip (
                  processor_audio_out_refs.value (), prefader.get_input_ports ()))
                {
                  connect_ports (
                    processor_audio_out_ref.id (), prefader_audio_in_ref.id ());
                }
            }
        }
      else if (output_type == dsp::PortType::Midi)
        {
          if (processor_midi_out_ref.has_value ())
            {
              auto &prefader = ch.get_midi_pre_fader ();
              connect_ports (
                processor_midi_out_ref->id (),
                prefader.get_midi_in_port (0).get_uuid ());
            }
        }
    }

  /* connect the prefader */
  if (output_type == dsp::PortType::Audio)
    {
      dsp::ProcessorGraphBuilder::add_connections (
        graph, ch.get_audio_pre_fader ());
    }
  else if (output_type == dsp::PortType::Midi)
    {
      dsp::ProcessorGraphBuilder::add_connections (
        graph, ch.get_midi_pre_fader ());
    }

  // connect the fader
  dsp::ProcessorGraphBuilder::add_connections (graph, *fader);

  // connect the prefader output to the fader input
  if (output_type == dsp::PortType::Audio)
    {
      auto       &prefader = ch.get_audio_pre_fader ();
      const auto &prefader_outs = prefader.get_output_ports ();
      const auto  fader_ins = fader->get_stereo_in_ports ();
      connect_ports (prefader_outs.at (0).id (), fader_ins.first.get_uuid ());
      connect_ports (prefader_outs.at (1).id (), fader_ins.second.get_uuid ());
    }
  else if (output_type == dsp::PortType::Midi)
    {
      auto &prefader = ch.get_midi_pre_fader ();
      connect_ports (
        prefader.get_midi_out_port (0).get_uuid (),
        fader->get_input_ports ().front ().id ());
    }

  // connect sends
  for (auto &send : ch.sends_)
    {
      dsp::ProcessorGraphBuilder::add_connections (graph, *send);

      if (send->is_midi ())
        {
          // connect (pre)fader output to send input
          if (send->is_prefader ())
            {
              connect_ports (
                ch.get_midi_pre_fader ().get_midi_out_port (0).get_uuid (),
                send->get_midi_in_port ().get_uuid ());
            }
          else
            {
              connect_ports (
                fader->get_output_ports ().front ().id (),
                send->get_midi_in_port ().get_uuid ());
            }
        }
      else if (send->is_audio ())
        {
          // connect (pre)fader output to send input
          if (send->is_prefader ())
            {
              connect_ports (
                ch.get_audio_pre_fader ().get_audio_out_port (0).get_uuid (),
                send->get_stereo_in_ports ().first.get_uuid ());
              connect_ports (
                ch.get_audio_pre_fader ().get_audio_out_port (1).get_uuid (),
                send->get_stereo_in_ports ().second.get_uuid ());
            }
          else
            {
              connect_ports (
                fader->get_output_ports ().at (0).id (),
                send->get_stereo_in_ports ().first.get_uuid ());
              connect_ports (
                fader->get_output_ports ().at (1).id (),
                send->get_stereo_in_ports ().second.get_uuid ());
            }
        }
    }

  // connect fader out to channel out
  if (output_type == dsp::PortType::Midi)
    {
      connect_ports (
        fader->get_output_ports ().front ().id (),
        ch.get_midi_out_port ().get_uuid ());
    }
  else if (output_type == dsp::PortType::Audio)
    {
      connect_ports (
        fader->get_output_ports ().at (0).id (),
        ch.get_stereo_out_ports ().first.get_uuid ());
      connect_ports (
        fader->get_output_ports ().at (1).id (),
        ch.get_stereo_out_ports ().second.get_uuid ());
    }
}
}
