// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/channel_subgraph_builder.h"

namespace zrythm::structure::tracks
{
void
ChannelSubgraphBuilder::add_nodes (dsp::graph::Graph &graph, Channel &ch)
{
  // prefader & post-fader
  if (ch.is_audio ())
    {
      dsp::ProcessorGraphBuilder::add_nodes (graph, ch.get_audio_pre_fader ());
      dsp::ProcessorGraphBuilder::add_nodes (graph, ch.get_audio_post_fader ());
    }
  else if (ch.is_midi ())
    {
      dsp::ProcessorGraphBuilder::add_nodes (graph, ch.get_midi_pre_fader ());
      dsp::ProcessorGraphBuilder::add_nodes (graph, ch.get_midi_post_fader ());
    }

  // fader
  {
    dsp::ProcessorGraphBuilder::add_nodes (graph, *ch.fader ());
  }

  // plugins
  std::vector<plugins::PluginPtrVariant> plugins;
  ch.get_plugins (plugins);
  for (
    auto * pl :
    plugins | std::views::transform (&plugins::plugin_ptr_variant_to_base))
    {
      dsp::ProcessorGraphBuilder::add_nodes (graph, *pl);
    }

  // sends
  for (const auto &send : ch.pre_fader_sends ())
    {
      dsp::ProcessorGraphBuilder::add_nodes (graph, *send);
    }
  for (const auto &send : ch.post_fader_sends ())
    {
      dsp::ProcessorGraphBuilder::add_nodes (graph, *send);
    }
}

void
ChannelSubgraphBuilder::add_connections (
  dsp::graph::Graph           &graph,
  Channel                     &ch,
  const dsp::PortUuidReference track_processor_output)
{
  if (
    graph.get_nodes ().find_node_for_processable (
      *track_processor_output.get_object_base ())
    == nullptr)
    {
      throw std::invalid_argument (
        "Track processor outputs must be added to the graph before calling this");
    }

  const auto connect_ports =
    [&] (
      const dsp::FinalPortSubclass auto &src,
      const dsp::FinalPortSubclass auto &dest) {
      add_connection_for_ports (graph, src, dest);
    };

  auto *                                fader = ch.fader ();
  std::optional<dsp::PortUuidReference> processor_midi_out_ref;
  std::optional<dsp::PortUuidReference> processor_audio_out_ref;
  const auto track_proc_out_var = track_processor_output.get_object ();
  if (std::holds_alternative<dsp::AudioPort *> (track_proc_out_var))
    {
      processor_audio_out_ref = track_processor_output;
    }
  else
    {
      processor_midi_out_ref = track_processor_output;
    }
  const auto channel_output_type =
    ch.fader ()->is_midi () ? dsp::PortType::Midi : dsp::PortType::Audio;

  std::vector<zrythm::plugins::PluginPtrVariant> plugins;
  ch.get_plugins (plugins);
  plugins::Plugin * last_plugin = nullptr;
  for (
    auto * const pl :
    plugins | std::views::transform (&plugins::plugin_ptr_variant_to_base))
    {
      dsp::ProcessorGraphBuilder::add_connections (graph, *pl);
      if (last_plugin == nullptr)
        {
          // connect processor outputs to plugin inputputs
          std::array<dsp::PortUuidReference, 1> processor_outputs{
            track_processor_output
          };
          bool connection_made = connect_like_ports (
            graph, processor_outputs, pl->get_input_ports ());

          // if no connection was made (plugin had no matching inputs), connect
          // the track processor outputs directly to the plugin processor
          if (!connection_made)
            {
              graph.get_nodes ()
                .find_node_for_processable (
                  *track_processor_output.get_object_base ())
                ->connect_to (
                  *graph.get_nodes ().find_node_for_processable (*pl));
            }
        }
      else // else if last processed plugin exists
        {
          // connect last plugin to this plugin
          const auto connect_plugins =
            [&] (plugins::Plugin &src, plugins::Plugin &dest) {
              // Get audio ports
              auto src_audio_outs =
                dsp::PortSpan{ src.get_output_ports () }
                  .get_elements_by_type<dsp::AudioPort> ();
              auto dest_audio_ins =
                dsp::PortSpan{ dest.get_input_ports () }
                  .get_elements_by_type<dsp::AudioPort> ();

              const size_t num_src_outs = std::ranges::distance (src_audio_outs);
              const size_t num_dest_ins = std::ranges::distance (dest_audio_ins);

              // Handle audio connections
              if (num_src_outs > 0 && num_dest_ins > 0)
                {
                  if (num_src_outs == 1 && num_dest_ins == 1)
                    {
                      // 1:1 connection
                      connect_ports (
                        *src_audio_outs.front (), *dest_audio_ins.front ());
                    }
                  else if (num_src_outs == 1)
                    {
                      // 1:N connection - mono to stereo/multi
                      for (auto * in_port : dest_audio_ins)
                        {
                          connect_ports (*src_audio_outs.front (), *in_port);
                        }
                    }
                  else if (num_dest_ins == 1)
                    {
                      // N:1 connection - stereo/multi to mono
                      connect_ports (
                        *src_audio_outs.front (), *dest_audio_ins.front ());
                    }
                  else
                    {
                      // N:N connection - connect min(N,M) ports
                      for (
                        const auto &[src_audio_out, dest_audio_in] :
                        std::views::zip (src_audio_outs, dest_audio_ins))
                        {
                          connect_ports (*src_audio_out, *dest_audio_in);
                        }
                    }
                }

              // Handle MIDI connections
              auto src_midi_outs =
                dsp::PortSpan{ src.get_output_ports () }
                  .get_elements_by_type<dsp::MidiPort> ();
              auto dest_midi_ins =
                dsp::PortSpan{ dest.get_input_ports () }
                  .get_elements_by_type<dsp::MidiPort> ();

              if (!src_midi_outs.empty () && !dest_midi_ins.empty ())
                {
                  // Connect first MIDI out to all MIDI ins
                  auto * midi_out = src_midi_outs.front ();
                  for (auto * midi_in : dest_midi_ins)
                    {
                      connect_ports (*midi_out, *midi_in);
                    }
                }
            };
          connect_plugins (*last_plugin, *pl);
        }
      last_plugin = pl;
    }

  if (plugins.empty ())
    {
      // connect processor outs to channel prefader
      if (channel_output_type == dsp::PortType::Audio)
        {
          if (processor_audio_out_ref.has_value ())
            {
              auto &prefader = ch.get_audio_pre_fader ();
              connect_ports (
                *processor_audio_out_ref->get_object_as<dsp::AudioPort> (),
                prefader.get_audio_in_port ());
            }
        }
      else if (channel_output_type == dsp::PortType::Midi)
        {
          if (processor_midi_out_ref.has_value ())
            {
              auto &prefader = ch.get_midi_pre_fader ();
              connect_ports (
                *processor_midi_out_ref->get_object_as<dsp::MidiPort> (),
                prefader.get_midi_in_port (0));
            }
        }
    }
  else // else if there is at least 1 plugin
    {
      // connect plugin outputs to channel prefader
      auto * last_pl = plugins::plugin_ptr_variant_to_base (plugins.back ());
      if (channel_output_type == dsp::PortType::Audio)
        {
          auto prefader_audio_ins =
            dsp::PortSpan{ ch.get_audio_pre_fader ().get_input_ports () }
              .get_elements_by_type<dsp::AudioPort> ();
          auto pl_audio_outs =
            dsp::PortSpan{ last_pl->get_output_ports () }
              .get_elements_by_type<dsp::AudioPort> ();

          bool connections_made{};
          for (
            const auto &[pl_audio_out, prefader_audio_in] :
            std::views::zip (pl_audio_outs, prefader_audio_ins))
            {
              connect_ports (*pl_audio_out, *prefader_audio_in);
              connections_made = true;
            }

          // if no compatible ports were connected, connect the plugin
          // outputs directly to the prefader processor
          if (!connections_made)
            {
              for (const auto &pl_audio_out : pl_audio_outs)
                {
                  graph.get_nodes ()
                    .find_node_for_processable (ch.get_midi_pre_fader ())
                    ->connect_to (*graph.get_nodes ().find_node_for_processable (
                      *pl_audio_out));
                }
            }
        }
      else if (channel_output_type == dsp::PortType::Midi)
        {
          auto prefader_midi_ins =
            dsp::PortSpan{ ch.get_midi_pre_fader ().get_input_ports () }
              .get_elements_by_type<dsp::MidiPort> ();
          auto pl_midi_outs =
            dsp::PortSpan{ last_pl->get_output_ports () }
              .get_elements_by_type<dsp::MidiPort> ();

          bool connections_made{};
          for (
            const auto &[pl_midi_out, prefader_midi_in] :
            std::views::zip (pl_midi_outs, prefader_midi_ins))
            {
              connect_ports (*pl_midi_out, *prefader_midi_in);
              connections_made = true;
            }

          // if no compatible ports were connected, connect the plugin
          // outputs directly to the prefader processor
          if (!connections_made)
            {
              for (const auto &pl_midi_out : pl_midi_outs)
                {
                  graph.get_nodes ()
                    .find_node_for_processable (ch.get_midi_pre_fader ())
                    ->connect_to (*graph.get_nodes ().find_node_for_processable (
                      *pl_midi_out));
                }
            }
        }
    }

  // connect the prefader & postfader
  if (channel_output_type == dsp::PortType::Audio)
    {
      dsp::ProcessorGraphBuilder::add_connections (
        graph, ch.get_audio_pre_fader ());
      dsp::ProcessorGraphBuilder::add_connections (
        graph, ch.get_audio_post_fader ());
    }
  else if (channel_output_type == dsp::PortType::Midi)
    {
      dsp::ProcessorGraphBuilder::add_connections (
        graph, ch.get_midi_pre_fader ());
      dsp::ProcessorGraphBuilder::add_connections (
        graph, ch.get_midi_post_fader ());
    }

  // connect the fader
  dsp::ProcessorGraphBuilder::add_connections (graph, *fader);

  // connect the prefader output to the fader input
  if (channel_output_type == dsp::PortType::Audio)
    {
      auto       &prefader = ch.get_audio_pre_fader ();
      const auto &fader_in = fader->get_stereo_in_port ();
      connect_ports (prefader.get_audio_out_port (), fader_in);
    }
  else if (channel_output_type == dsp::PortType::Midi)
    {
      auto &prefader = ch.get_midi_pre_fader ();
      connect_ports (prefader.get_midi_out_port (0), fader->get_midi_in_port ());
    }

  // connect sends
  for (const auto &send : ch.pre_fader_sends ())
    {
      dsp::ProcessorGraphBuilder::add_connections (graph, *send);

      if (send->is_midi ())
        {
          // connect prefader output to send input
          connect_ports (
            ch.get_midi_pre_fader ().get_midi_out_port (0),
            send->get_midi_in_port ());

          // connect send output to destination port (if set)
          if (auto dest = send->destination_port ())
            {
              connect_ports (
                send->get_midi_out_port (),
                *dest->get_object_as<dsp::MidiPort> ());
            }
        }
      else if (send->is_audio ())
        {
          // connect prefader output to send input
          connect_ports (
            ch.get_audio_pre_fader ().get_audio_out_port (),
            send->get_stereo_in_port ());

          // connect send output to destination port (if set)
          if (auto dest = send->destination_port ())
            {
              connect_ports (
                send->get_stereo_out_port (),
                *dest->get_object_as<dsp::AudioPort> ());
            }
        }
    }
  for (const auto &send : ch.post_fader_sends ())
    {
      dsp::ProcessorGraphBuilder::add_connections (graph, *send);

      if (send->is_midi ())
        {
          // connect fader output to send input
          connect_ports (fader->get_midi_out_port (), send->get_midi_in_port ());

          // connect send output to destination port (if set)
          if (auto dest = send->destination_port ())
            {
              connect_ports (
                send->get_midi_out_port (),
                *dest->get_object_as<dsp::MidiPort> ());
            }
        }
      else if (send->is_audio ())
        {
          // connect fader output to send input
          connect_ports (
            fader->get_stereo_out_port (), send->get_stereo_in_port ());

          // connect send output to destination port (if set)
          if (auto dest = send->destination_port ())
            {
              connect_ports (
                send->get_stereo_out_port (),
                *dest->get_object_as<dsp::AudioPort> ());
            }
        }
    }

  // connect fader out to channel post-fader inputs
  if (channel_output_type == dsp::PortType::Midi)
    {
      connect_ports (
        fader->get_midi_out_port (),
        ch.get_midi_post_fader ().get_midi_in_port (0));
    }
  else if (channel_output_type == dsp::PortType::Audio)
    {
      connect_ports (
        fader->get_stereo_out_port (),
        ch.get_audio_post_fader ().get_audio_in_port ());
    }
}
}
