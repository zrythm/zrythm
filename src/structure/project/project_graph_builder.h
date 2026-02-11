// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph_builder.h"
#include "dsp/port.h"

namespace zrythm::dsp
{
class Metronome;
class Fader;
}

namespace zrythm::structure::project
{

class Project;

class ProjectGraphBuilder final : public dsp::graph::IGraphBuilder
{
public:
  ProjectGraphBuilder (
    Project        &project,
    dsp::Metronome &metronome,
    dsp::Fader     &monitor_fader)
      : project_ (&project), metronome_ (&metronome),
        monitor_fader_ (&monitor_fader)
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
  Project *        project_{};
  dsp::Metronome * metronome_{};
  dsp::Fader *     monitor_fader_{};
};
}
