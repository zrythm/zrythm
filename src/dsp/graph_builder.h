// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph.h"

namespace zrythm::dsp
{

/**
 * @brief Interface for building a graph of nodes.
 *
 * Implementations of this interface are responsible for constructing the
 * graph of nodes that represent the audio processing pipeline.
 */
class IGraphBuilder
{
public:
  virtual ~IGraphBuilder () = default;

  /**
   * @brief Populates the graph.
   *
   * @param graph The graph to populate.
   */
  void build_graph (Graph &graph)
  {
    build_graph_impl (graph);
    graph.finalize_nodes ();
  };

protected:
  /**
   * @brief Actual logic to be implemented by subclasses.
   */
  virtual void build_graph_impl (Graph &graph) = 0;
};

} // namespace zrythm::dsp
