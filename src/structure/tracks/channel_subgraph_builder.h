// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph.h"
#include "structure/tracks/channel.h"

namespace zrythm::structure::tracks
{
/**
 * @brief A helper class to add nodes and standard connections for a channel to
 * a DSP graph.
 */
class ChannelSubgraphBuilder
{
public:
  static void add_nodes (dsp::graph::Graph &graph, Channel &ch);

  /**
   * @brief Adds connections for the nodes already in the graph.
   *
   * This requires add_nodes() to be called beforehand.
   *
   * @note @p track_processor_output must be added to the graph before calling
   * this.
   *
   * @param graph
   * @param ch
   * @param track_processor_output Track processor output to connect to the
   * channel's input (or the first plugin).
   */
  static void add_connections (
    dsp::graph::Graph     &graph,
    Channel               &ch,
    dsp::PortUuidReference track_processor_output);

  /**
   * @brief Adds a connection to the graph for the given ports.
   *
   * @note Assumes the ports are already added to the graph.
   *
   * @param graph
   * @param src_ref
   * @param dest_ref
   */
  template <dsp::FinalPortSubclass T>
  static void add_connection_for_ports (
    dsp::graph::Graph &graph,
    const T           &src_port,
    const T           &dest_port)
  {
    auto * src = graph.get_nodes ().find_node_for_processable (src_port);
    auto * dest = graph.get_nodes ().find_node_for_processable (dest_port);
    assert (src);
    assert (dest);
    src->connect_to (*dest);
  }

  /**
   * @brief Connects ports of the same type in the given source and destination
   * ranges.
   *
   * @return Whether any connections were made.
   */
  static bool connect_like_ports (
    dsp::graph::Graph                          &graph,
    const RangeOf<dsp::PortUuidReference> auto &src_refs,
    const RangeOf<dsp::PortUuidReference> auto &dest_refs)
  {
    using ObjectView = utils::UuidIdentifiableObjectView<dsp::PortRegistry>;
    const auto object_getter = [] (auto &&port_ref) {
      return port_ref.get_object ();
    };
    const auto src_output_ports =
      src_refs | std::views::transform (object_getter);
    const auto dest_input_ports =
      dest_refs | std::views::transform (object_getter);

    auto src_midi_out_ports =
      src_output_ports
      | std::views::filter (ObjectView::type_projection<dsp::MidiPort>)
      | std::views::transform (ObjectView::type_transformation<dsp::MidiPort>);
    auto dest_midi_in_ports =
      dest_input_ports
      | std::views::filter (ObjectView::type_projection<dsp::MidiPort>)
      | std::views::transform (ObjectView::type_transformation<dsp::MidiPort>);
    auto src_audio_out_ports =
      src_output_ports
      | std::views::filter (ObjectView::type_projection<dsp::AudioPort>)
      | std::views::transform (ObjectView::type_transformation<dsp::AudioPort>);
    auto dest_audio_in_ports =
      dest_input_ports
      | std::views::filter (ObjectView::type_projection<dsp::AudioPort>)
      | std::views::transform (ObjectView::type_transformation<dsp::AudioPort>);
    auto src_cv_out_ports =
      src_output_ports
      | std::views::filter (ObjectView::type_projection<dsp::CVPort>)
      | std::views::transform (ObjectView::type_transformation<dsp::CVPort>);
    auto dest_cv_in_ports =
      dest_input_ports
      | std::views::filter (ObjectView::type_projection<dsp::CVPort>)
      | std::views::transform (ObjectView::type_transformation<dsp::CVPort>);

    bool       connections_made{};
    const auto make_conns = [&connections_made, &graph] (const auto &ports) {
      const auto &[src_port, dest_port] = ports;
      add_connection_for_ports (graph, *src_port, *dest_port);
      connections_made = true;
    };

    std::ranges::for_each (
      std::views::zip (src_midi_out_ports, dest_midi_in_ports), make_conns);
    std::ranges::for_each (
      std::views::zip (src_audio_out_ports, dest_audio_in_ports), make_conns);
    std::ranges::for_each (
      std::views::zip (src_cv_out_ports, dest_cv_in_ports), make_conns);

    return connections_made;
  }
};
} // namespace zrythm::structure::tracks
