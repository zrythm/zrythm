// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/dsp/graph.h"

class Project;
class Port;

/**
 * @brief Interface for building a graph of nodes.
 *
 * Implementations of this interface are responsible for constructing the
 * graph of nodes that represent the audio processing pipeline.
 */
class IGraphBuilder
{
public:
  using GraphNode = Graph::GraphNode;

  virtual ~IGraphBuilder () = default;

  /**
   * @brief Populates the graph.
   *
   * @param graph The graph to populate.
   */
  void build_graph (Graph &graph)
  {
    build_graph_impl (graph);
    graph.finish_adding_nodes ();
  };

protected:
  /**
   * @brief Actual logic to be implemented by subclasses.
   */
  virtual void build_graph_impl (Graph &graph) = 0;
};

class ProjectGraphBuilder final : public IGraphBuilder
{
public:
  ProjectGraphBuilder (Project &project, bool drop_unnecessary_ports)
      : project_ (project), drop_unnecessary_ports_ (drop_unnecessary_ports)
  {
  }

  /**
   * Adds a new connection for the given src and dest ports and validates the
   * graph.
   *
   * @return Whether the ports can be connected (if the connection will
   * be valid and won't break the acyclicity of the graph).
   */
  static bool
  can_ports_be_connected (Project &project, const Port &src, const Port &dest);

private:
  void build_graph_impl (Graph &graph) override;

private:
  Project &project_;

  /**
   * @brief Whether to drop any ports that don't connect anywhere.
   */
  bool drop_unnecessary_ports_{};
};
