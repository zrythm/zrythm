// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <stop_token>

#include "dsp/graph_scheduler.h"
#include "utils/audio.h"
#include "utils/units.h"

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

  GraphRenderer (RenderOptions options);

  /**
   * @brief Renders the graph for the given range and returns the resulting
   * audio, or nullopt if cancelled.
   *
   * @param nodes
   * @param range
   * @return The resulting audio from the terminal AudioPort node, or nullopt if
   * cancelled.
   */
  [[nodiscard]] auto render (
    graph::GraphNodeCollection &&nodes,
    SampleRange                  range,
    std::stop_token token = {}) -> std::optional<juce::AudioSampleBuffer>;

private:
  RenderOptions options_;
};
}
