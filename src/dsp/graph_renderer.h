// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

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

  auto render (graph::GraphNodeCollection &&nodes, SampleRange range)
    -> juce::AudioSampleBuffer;

private:
  RenderOptions         options_;
  graph::GraphScheduler graph_scheduler_;
};
}
