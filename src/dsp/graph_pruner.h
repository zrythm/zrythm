// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <unordered_set>

#include "dsp/graph.h"

namespace zrythm::dsp::graph
{
/**
 * @brief Helper class for pruning a graph.
 */
class GraphPruner
{
public:
  /**
   * @brief Prunes an existing graph to only include nodes that lead to
   * specified terminals.
   *
   * This is a utility method that can be called on any graph after it's built.
   * It performs a backward traversal from the specified terminal nodes and
   * removes all nodes that are not in the dependency chain.
   *
   * @param graph The graph to prune (will be modified in-place)
   * @param terminal_nodes The terminal nodes to preserve in pruned graph
   * @return Whether prune operation was successful
   */
  static bool prune_graph_to_terminals (
    Graph                                                &graph,
    const std::vector<std::reference_wrapper<GraphNode>> &terminal_nodes);

private:
  /**
   * @brief Helper to mark upstream nodes during prune operations.
   *
   * Uses a temporary marking approach that doesn't modify GraphNode.
   */
  static void mark_upstream_nodes (
    GraphNode                       &node,
    std::unordered_set<GraphNode *> &marked_nodes);
};
}
