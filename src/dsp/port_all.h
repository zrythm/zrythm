// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_port.h"
#include "dsp/cv_port.h"
#include "dsp/graph.h"
#include "dsp/midi_port.h"

namespace zrythm::dsp
{

/**
 * @brief Adds a connection to the graph for the given ports.
 *
 * @note Assumes the ports are already added to the graph.
 *
 * @param graph
 * @param src_ref
 * @param dest_ref
 */
inline void
add_connection_for_ports (
  dsp::graph::Graph            &graph,
  const dsp::PortUuidReference &src_ref,
  const dsp::PortUuidReference &dest_ref)
{
  std::visit (
    [&] (auto &&src_port, auto &&dest_port) {
      auto * src = graph.get_nodes ().find_node_for_processable (*src_port);
      auto * dest = graph.get_nodes ().find_node_for_processable (*dest_port);
      assert (src);
      assert (dest);
      src->connect_to (*dest);
    },
    src_ref.get_object (), dest_ref.get_object ());
}
} // namespace zrythm::dsp
