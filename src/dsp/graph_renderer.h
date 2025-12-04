// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph_node.h"
#include "utils/units.h"

#include <QPromise>

#include <juce_wrapper.h>

namespace zrythm::dsp
{
class GraphRenderer
{
public:
  using SampleRange = std::pair<units::sample_t, units::sample_t>;
  using RunOnMainThread = std::function<void (std::function<void ()>)>;

  struct RenderOptions
  {
    units::sample_rate_t sample_rate_;
    units::sample_t      block_length_;
    unsigned int         num_threads_ =
      std::max (5u, std::thread::hardware_concurrency ()) - 4;
  };

  /**
   * @brief Executes render() asynchronously and returns a QFuture to control
   * the task.
   *
   * @param options
   * @param nodes
   * @param range
   * @param run_on_main_thread Used to invoke logic that needs to run on the
   * main thread (like graph node prepare_for_processing, etc.).
   */
  static QFuture<juce::AudioSampleBuffer> render_async (
    RenderOptions                options,
    graph::GraphNodeCollection &&nodes,
    RunOnMainThread              run_on_main_thread,
    SampleRange                  range);

private:
  /**
   * @brief Renders the graph for the given range.
   *
   * @param nodes
   * @param range
   */
  static void render (
    QPromise<juce::AudioSampleBuffer> &promise,
    RenderOptions                      options,
    graph::GraphNodeCollection       &&nodes,
    RunOnMainThread                    run_on_main_thread,
    SampleRange                        range);
};
}
