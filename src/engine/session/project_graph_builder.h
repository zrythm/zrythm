// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph_builder.h"
#include "dsp/port.h"

using namespace zrythm;

class Project;

class ProjectGraphBuilder final : public dsp::graph::IGraphBuilder
{
public:
  ProjectGraphBuilder (Project &project) : project_ (&project) { }

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
  Project * project_{};
};
