// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph_builder.h"

using namespace zrythm;

class Project;

namespace zrythm::dsp
{
class Port;
};

class ProjectGraphBuilder final : public dsp::graph::IGraphBuilder
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
  static bool can_ports_be_connected (
    Project         &project,
    const dsp::Port &src,
    const dsp::Port &dest);

private:
  void build_graph_impl (dsp::graph::Graph &graph) override;

private:
  Project &project_;

  /**
   * @brief Whether to drop any ports that don't connect anywhere.
   */
  bool drop_unnecessary_ports_{};
};
