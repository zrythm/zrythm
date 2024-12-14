// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright (C) 2017, 2019 Robin Gareus <robin@gareus.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---
 */

#ifndef ZRYTHM_DSP_GRAPH_H
#define ZRYTHM_DSP_GRAPH_H

#include "dsp/graph_node.h"

namespace zrythm::dsp
{

/**
 * @brief The Graph class represents a graph of DSP nodes.
 *
 * The Graph class manages the lifecycle of DSP nodes and their connections,
 * allowing for efficient parallel processing of the graph. It provides
 * functionality to start and terminate the processing, add and remove nodes,
 * and manage the overall graph structure.
 */
class Graph final
{
  friend class IGraphBuilder;
  using GraphNode = dsp::GraphNode;

public:
  void print () const;

  /**
   * Creates a new node, adds it to the graph and returns it.
   */
  GraphNode * add_node_for_processable (
    dsp::IProcessable     &node,
    const dsp::ITransport &transport);

  GraphNode * add_initial_processor (const dsp::ITransport &transport)
  {
    setup_nodes_.initial_processor_ = std::make_unique<dsp::InitialProcessor> ();
    return add_node_for_processable (
      *setup_nodes_.initial_processor_, transport);
  };

  /**
   * @brief Checks for cycles in the graph.
   *
   * @return Whether valid.
   */
  bool is_valid () const;

  /**
   * @brief Steals the nodes in the graph.
   *
   * After this function is called, the graph will be empty.
   */
  auto &&steal_nodes () { return std::move (setup_nodes_); }

  auto &get_nodes () { return setup_nodes_; }

private:
  /**
   * @brief To be called when done adding nodes to the graph.
   *
   * This will update initial and terminal nodes.
   */
  void finish_adding_nodes ();

private:
  /**
   * @brief Nodes in this graph.
   *
   * These are "setup nodes", in that they are not used during actual
   * processing. They just hold the initial (setup) topology of the graph that
   * can be used to generate a "live" collection.
   */
  dsp::GraphNodeCollection setup_nodes_;
};

} // namespace zrythm::dsp

#endif
