// SPDX-FileCopyrightText: © 2019-2021, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include "dsp/graph_node.h"

namespace zrythm::dsp::graph
{

/**
 * @brief A DSP graph under construction.
 *
 * Nodes and edges are added during construction; is_valid() checks the
 * topology built so far for cycles. finalize() consumes the graph and
 * produces the immutable FinalizedGraph used for processing.
 */
class Graph final
{
public:
  /**
   * Creates a new node, adds it to the graph and returns it.
   *
   * @note This method does not allow duplicates. If a duplicate is attempted to
   * be added, this method will return the existing node without adding a new
   * node.
   */
  GraphNode * add_node_for_processable (IProcessable &node);

  /**
   * @brief Adds an externally constructed node and returns it.
   */
  GraphNode * add_node (std::unique_ptr<GraphNode> node);

  /**
   * @brief Creates, adds and returns the initial processor node.
   */
  GraphNode * add_initial_processor ();

  /**
   * @brief Connects @p from to @p to (@p from feeds @p to).
   *
   * If the edge already exists it is not duplicated; a passed
   * configuration still replaces any configuration previously stored for
   * the edge.
   *
   * @param config Configuration carried by the edge; std::nullopt means the
   * edge carries no configuration and consumers apply their defaults.
   */
  void connect (
    GraphNode                      &from,
    GraphNode                      &to,
    std::optional<ConnectionConfig> config = std::nullopt);

  /**
   * @brief Removes the edge from @p from to @p to, if any.
   *
   * Severs both endpoints of the edge and drops the configuration it
   * carried.
   *
   * @return Whether an edge was removed.
   */
  bool disconnect (GraphNode &from, GraphNode &to);

  /**
   * @brief Erases the nodes matching @p predicate.
   *
   * Edges between erased and remaining nodes (and the connection
   * configurations they carry) are severed first.
   *
   * @return Number of nodes erased.
   */
  size_t erase_nodes_if (auto &&predicate)
  {
    std::unordered_set<GraphNode *> to_erase;
    for (const auto &node : nodes_)
      {
        if (predicate (node))
          {
            to_erase.insert (node.get ());
          }
      }
    for (const auto * erased : to_erase)
      {
        for (const auto &feed : erased->feeds ())
          {
            feed.get ().remove_depend (*erased);
          }
        for (const auto &depend : erased->depends ())
          {
            depend.get ().remove_feed (*erased);
          }
      }
    return std::erase_if (nodes_, [&to_erase] (const auto &node) {
      return to_erase.contains (node.get ());
    });
  }

  /**
   * @brief Checks the topology built so far for cycles.
   *
   * Holds for the graph in its current state; no finalization required.
   *
   * @return Whether valid.
   */
  bool is_valid () const;

  /**
   * @brief Returns the node for @p processable, or null if there is none.
   */
  GraphNode * find_node_for_processable (const IProcessable &processable) const;

  /**
   * @brief All nodes added so far.
   */
  const std::vector<std::unique_ptr<GraphNode>> &nodes () const
  {
    return nodes_;
  }

  /**
   * @brief Computes the derived trigger/terminal node sets and returns the
   * finalized collection.
   *
   * The graph is consumed: it owns no nodes afterwards.
   */
  FinalizedGraph finalize () &&;

private:
  std::unique_ptr<InitialProcessor>       initial_processor_;
  std::vector<std::unique_ptr<GraphNode>> nodes_;
};

} // namespace zrythm::dsp::graph
