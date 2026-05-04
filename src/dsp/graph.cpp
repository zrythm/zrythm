// SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <unordered_map>

#include "dsp/graph.h"
#include "utils/logger.h"

namespace zrythm::dsp::graph
{

bool
Graph::is_valid () const
{
  // Topological sort using Kahn's algorithm (non-destructive).
  // Track remaining refcounts locally instead of modifying the graph.
  std::unordered_map<GraphNode *, int> remaining;
  remaining.reserve (setup_nodes_.graph_nodes_.size ());
  for (const auto &node : setup_nodes_.graph_nodes_)
    {
      remaining.emplace (node.get (), node->init_refcount_);
    }

  std::vector<std::reference_wrapper<GraphNode>> queue;
  queue.reserve (setup_nodes_.trigger_nodes_.size ());
  for (const auto trigger : setup_nodes_.trigger_nodes_)
    {
      queue.push_back (trigger);
    }

  while (!queue.empty ())
    {
      auto current = queue.back ();
      queue.pop_back ();

      for (const auto &child : current.get ().feeds ())
        {
          if (auto it = remaining.find (&child.get ()); it != remaining.end ())
            {
              --it->second;
              if (it->second == 0)
                {
                  queue.push_back (child);
                }
            }
        }
    }

  return std::ranges::all_of (remaining, [] (const auto &entry) {
    return entry.second == 0;
  });
}

void
Graph::print () const
{
  z_info ("==printing graph");

  for (const auto &node : setup_nodes_.graph_nodes_)
    {
      z_info ("{}", node->print_node_to_str ());
    }

  z_info (
    "num trigger nodes {} | num terminal nodes {}",
    setup_nodes_.trigger_nodes_.size (), setup_nodes_.terminal_nodes_.size ());
  z_info ("==finish printing graph");
}

GraphNode *
Graph::add_node_for_processable (IProcessable &node)
{
  if (
    auto * node_ptr = setup_nodes_.find_node_for_processable (node);
    node_ptr != nullptr)
    {
      // don't allow duplicates
      return node_ptr;
    }
  setup_nodes_.graph_nodes_.emplace_back (
    std::make_unique<GraphNode> (setup_nodes_.graph_nodes_.size (), node));
  return setup_nodes_.graph_nodes_.back ().get ();
}

} // namespace zrythm::dsp
