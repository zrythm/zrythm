// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph.h"
#include "dsp/port_all.h"
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
    dsp::PortRegistry     &port_registry,
    Channel               &ch,
    dsp::PortUuidReference track_processor_output);
};
} // namespace zrythm::structure::tracks
