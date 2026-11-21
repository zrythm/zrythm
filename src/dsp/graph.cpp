// SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>
#include <unordered_map>

#include "dsp/graph.h"
#include "utils/logger.h"

namespace zrythm::dsp::graph
{

bool
Graph::is_valid () const
{
  // Topological sort using Kahn's algorithm (non-destructive).
  // The queue is seeded from a fresh scan of the current refcounts, so the
  // result holds for the topology as built so far.
  std::unordered_map<GraphNode *, int> remaining;
  remaining.reserve (nodes_.size ());
  std::vector<std::reference_wrapper<GraphNode>> queue;
  queue.reserve (nodes_.size ());
  for (const auto &node : nodes_)
    {
      remaining.emplace (node.get (), node->init_refcount_);
      if (node->init_refcount_ == 0)
        {
          queue.push_back (*node);
        }
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

GraphNode *
Graph::add_node_for_processable (IProcessable &processable)
{
  if (
    auto * node_ptr = find_node_for_processable (processable);
    node_ptr != nullptr)
    {
      // don't allow duplicates
      return node_ptr;
    }
  nodes_.emplace_back (
    std::make_unique<GraphNode> (nodes_.size (), processable));
  return nodes_.back ().get ();
}

GraphNode *
Graph::add_node (std::unique_ptr<GraphNode> node)
{
  assert (node != nullptr);
  nodes_.emplace_back (std::move (node));
  return nodes_.back ().get ();
}

GraphNode *
Graph::add_initial_processor ()
{
  initial_processor_ = std::make_unique<InitialProcessor> ();
  return add_node_for_processable (*initial_processor_);
}

void
Graph::connect (
  GraphNode                      &from,
  GraphNode                      &to,
  std::optional<ConnectionConfig> config)
{
  from.connect_to (to, std::move (config));
}

bool
Graph::disconnect (GraphNode &from, GraphNode &to)
{
  const auto feed_removed = from.remove_feed (to);
  const auto depend_removed = to.remove_depend (from);
  assert (feed_removed == depend_removed);
  return feed_removed;
}

GraphNode *
Graph::find_node_for_processable (const IProcessable &processable) const
{
  auto it = std::ranges::find_if (nodes_, [&] (const auto &node) {
    return std::addressof (node->get_processable ())
           == std::addressof (processable);
  });
  if (it != nodes_.end ())
    {
      return it->get ();
    }
  return nullptr;
}

FinalizedGraph
Graph::finalize () &&
{
  assert (is_valid ());
  FinalizedGraph collection;
  collection.graph_nodes_ = std::move (nodes_);
  collection.initial_processor_ = std::move (initial_processor_);
  collection.set_initial_and_terminal_nodes ();
  return collection;
}

} // namespace zrythm::dsp::graph
