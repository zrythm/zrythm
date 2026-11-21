// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "graph_pruner.h"

namespace zrythm::dsp::graph
{

bool
GraphPruner::prune_graph_to_terminals (
  Graph                                                &graph,
  const std::vector<std::reference_wrapper<GraphNode>> &terminal_nodes)
{
  if (terminal_nodes.empty ())
    {
      return false;
    }

  // Mark all nodes that should be kept
  std::unordered_set<GraphNode *> marked_nodes;
  for (const auto &terminal : terminal_nodes)
    {
      mark_upstream_nodes (terminal.get (), marked_nodes);
    }

  // Remove unmarked nodes
  graph.erase_nodes_if ([&marked_nodes] (const auto &node) {
    return !marked_nodes.contains (node.get ());
  });

  return true;
}

void
GraphPruner::mark_upstream_nodes (
  GraphNode                       &node,
  std::unordered_set<GraphNode *> &marked_nodes)
{
  if (marked_nodes.contains (&node))
    {
      return;
    }

  marked_nodes.insert (&node);

  // Mark all parent nodes
  for (const auto &parent : node.depends ())
    {
      mark_upstream_nodes (parent.get (), marked_nodes);
    }
}
}
