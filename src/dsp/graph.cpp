// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph.h"

namespace zrythm::dsp::graph
{

bool
Graph::is_valid () const
{
  std::vector<std::reference_wrapper<GraphNode>> triggers;
  triggers.reserve (setup_nodes_.trigger_nodes_.size ());
  for (const auto trigger : setup_nodes_.trigger_nodes_)
    {
      triggers.push_back (trigger);
    }

  while (!triggers.empty ())
    {
      const auto trigger = triggers.back ();
      triggers.pop_back ();

      for (const auto child : trigger.get ().childnodes_)
        {
          trigger.get ().childnodes_.pop_back ();
          child.get ().init_refcount_--;
          if (child.get ().init_refcount_ == 0)
            {
              triggers.push_back (child);
            }
        }
    }

  return std::ranges::none_of (setup_nodes_.graph_nodes_, [] (const auto &node) {
    return !node->childnodes_.empty () || node->init_refcount_ > 0;
  });
}

void
Graph::print () const
{
  z_info ("==printing graph");

  for (const auto &node : setup_nodes_.graph_nodes_)
    {
      node->print_node ();
    }

  z_info (
    "num trigger nodes {} | num terminal nodes {}",
    setup_nodes_.trigger_nodes_.size (), setup_nodes_.terminal_nodes_.size ());
  z_info ("==finish printing graph");
}

GraphNode *
Graph::add_node_for_processable (
  IProcessable          &node,
  const dsp::ITransport &transport)
{
  setup_nodes_.graph_nodes_.emplace_back (
    std::make_unique<GraphNode> (
      setup_nodes_.graph_nodes_.size (), transport, node));
  return setup_nodes_.graph_nodes_.back ().get ();
}

} // namespace zrythm::dsp
