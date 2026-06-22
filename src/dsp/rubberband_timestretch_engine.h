// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/timestretch_engine.h"
#include "utils/units.h"

namespace zrythm::dsp
{

/**
 * @brief RubberBand v4 time-stretch engine (offline, pitch-preserving).
 *
 * Uses the RubberBand C++ API in Offline mode with the R3 (Finer) engine and
 * setKeyFrameMap() seeded from the TimeWarpMap anchors, producing
 * high-quality variable-ratio stretching that follows tempo curves including
 * linear ramps. Exact output duration is guaranteed by offline
 * padding/delay compensation plus setExpectedInputDuration().
 *
 * Only @ref StretchOptions::PitchMode::Preserve is supported; a future
 * resampler-based engine will handle Repitch mode.
 */
class RubberBandTimeStretchEngine final : public ITimeStretchEngine
{
public:
  /**
   * @param sample_rate The sample rate the engine will process audio at.
   */
  explicit RubberBandTimeStretchEngine (units::sample_rate_t sample_rate);
  ~RubberBandTimeStretchEngine () override;

  [[nodiscard]] std::string_view id () const override;
  [[nodiscard]] bool supports (StretchOptions::PitchMode mode) const override;

private:
  [[nodiscard]] utils::audio::AudioBuffer stretch_impl (
    const utils::audio::AudioBuffer &input,
    const TimeWarpMap               &warp,
    const StretchOptions            &options) [[clang::blocking]] override;

  units::sample_rate_t sample_rate_;
};

} // namespace zrythm::dsp
