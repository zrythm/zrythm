// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
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

  auto &nodes = graph.get_nodes ().graph_nodes_;

  // Mark all nodes that should be kept
  std::unordered_set<GraphNode *> marked_nodes;
  for (const auto &terminal : terminal_nodes)
    {
      mark_upstream_nodes (terminal.get (), marked_nodes);
    }

  // Remove unmarked nodes
  const auto predicate = [&] (const auto &node) {
    return !marked_nodes.contains (node.get ());
  };
  auto filter_view = std::views::filter (nodes, predicate);
  for (const auto &node_to_erase : filter_view)
    {
      for (const auto &feed : node_to_erase->feeds ())
        {
          feed.get ().remove_depend (*node_to_erase);
        }
      for (const auto &depend : node_to_erase->depends ())
        {
          depend.get ().remove_feed (*node_to_erase);
        }
    }
  std::erase_if (nodes, predicate);

  // Rebuild terminal and trigger node lists
  graph.finalize_nodes ();

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
