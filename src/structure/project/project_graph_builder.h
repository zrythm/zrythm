// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>

#include "dsp/graph_builder.h"
#include "dsp/port.h"
#include "utils/refusal.h"

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
   * Returns why the given ports cannot be wired together in the
   * processing graph, or no value when they can.
   *
   * Refuses a port that has no node in the processing graph, and a
   * wiring that would break the graph's acyclicity.
   */
  static std::optional<utils::RefusalReason> port_wiring_refusal_reason (
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
