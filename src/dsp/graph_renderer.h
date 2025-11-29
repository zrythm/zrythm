// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph_node.h"
#include "utils/audio.h"
#include "utils/units.h"

#include <QPromise>

namespace zrythm::dsp
{
class GraphRenderer
{
public:
  using BitDepth = utils::audio::BitDepth;
  using SampleRange = std::pair<units::sample_t, units::sample_t>;

  struct RenderOptions
  {
    BitDepth             bit_depth_;
    bool                 dither_;
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
   */
  static QFuture<juce::AudioSampleBuffer> render_run_async (
    RenderOptions                options,
    graph::GraphNodeCollection &&nodes,
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
    SampleRange                        range);
};
}
