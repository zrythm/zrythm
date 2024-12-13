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
   * @brief Function to be called after the graph is built and before
   * re-chaining (if re-chaining requested).
   *
   */
  using PostBuildCallback = std::function<void ()>;

  /**
   * @brief Builds the graph.
   *
   * @param graph The graph to build.
   * @param rechain Whether to rechain (prepare the graph for processing by
   * moving its setup nodes to active nodes).
   */
  virtual void build_graph (
    Graph                           &graph,
    bool                             rechain,
    std::optional<PostBuildCallback> post_build_cb) = 0;
};

class ProjectGraphBuilder final : public IGraphBuilder
{
public:
  ProjectGraphBuilder (Project &project, bool drop_unnecessary_ports)
      : project_ (project), drop_unnecessary_ports_ (drop_unnecessary_ports)
  {
  }

  void build_graph (
    Graph                           &graph,
    bool                             rechain,
    std::optional<PostBuildCallback> post_build_cb) override;

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
  Project &project_;

  /**
   * @brief Whether to drop any ports that don't connect anywhere.
   */
  bool drop_unnecessary_ports_{};
};
