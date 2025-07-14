// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph_node.h"

namespace zrythm::dsp::graph
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
  friend class GraphExport;

public:
  void print () const;

  /**
   * Creates a new node, adds it to the graph and returns it.
   *
   * @note This method does not allow duplicates. If a duplicate is attempted to
   * be added, this method will return the existing node without adding a new
   * node.
   */
  GraphNode *
  add_node_for_processable (IProcessable &node, const dsp::ITransport &transport);

  GraphNode * add_initial_processor (const dsp::ITransport &transport)
  {
    setup_nodes_.initial_processor_ = std::make_unique<InitialProcessor> ();
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

  void finalize_nodes () { setup_nodes_.finalize_nodes (); }

private:
  /**
   * @brief Nodes in this graph.
   *
   * These are "setup nodes", in that they are not used during actual
   * processing. They just hold the initial (setup) topology of the graph that
   * can be used to generate a "live" collection.
   */
  GraphNodeCollection setup_nodes_;
};

} // namespace zrythm::dsp::graph
