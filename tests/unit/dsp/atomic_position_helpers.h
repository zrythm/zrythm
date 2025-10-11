// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/atomic_position.h"
#include "dsp/tempo_map.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{
static inline auto
basic_conversion_providers ()
{
  // Use custom conversion providers that support negative positions
  // 120 BPM = 960 ticks per beat, 0.5 seconds per beat
  return std::make_unique<
    AtomicPosition::
      TimeConversionFunctions> (AtomicPosition::TimeConversionFunctions{
    .tick_to_seconds =
      [] (units::precise_tick_t ticks) {
        return (ticks / units::ticks (960.0)) * units::seconds (0.5);
      },
    .seconds_to_tick =
      [] (units::precise_second_t seconds) {
        return seconds / units::seconds (0.5) * units::ticks (960.0);
      },
    .tick_to_samples =
      [] (units::precise_tick_t ticks) {
        return (ticks / units::ticks (960.0)) * units::seconds (0.5)
               * units::sample_rate (44100.0);
      },
    .samples_to_tick =
      [] (units::precise_sample_t samples) {
        return samples / units::sample_rate (44100.0) / units::seconds (0.5)
               * units::ticks (960.0);
      },
  });
}
}
